// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Examples/HorizonChatExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "Models/HorizonChatWire.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if PLATFORM_ANDROID || PLATFORM_IOS
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif
namespace HorizonChatJournal
{
/** Mobile UE's system-wide mutex is deliberately unimplemented and fatal: use its POSIX equivalent. */
class FExclusiveLock
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    int Descriptor=-1;
#else
    TUniquePtr<FSystemWideCriticalSection> Mutex;
#endif
public:
    explicit FExclusiveLock(const FString& Name)
    {
#if PLATFORM_ANDROID || PLATFORM_IOS
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("SaveGames");IFileManager::Get().MakeDirectory(*Directory,true);const FString File=Directory/(Name+TEXT(".lock"));
        Descriptor=open(TCHAR_TO_UTF8(*File),O_CREAT|O_WRONLY,0600);if(Descriptor>=0&&flock(Descriptor,LOCK_EX|LOCK_NB)!=0){close(Descriptor);Descriptor=-1;}
#else
        Mutex=MakeUnique<FSystemWideCriticalSection>(Name);
#endif
    }
    bool IsValid() const
    {
#if PLATFORM_ANDROID || PLATFORM_IOS
        return Descriptor>=0;
#else
        return Mutex&&Mutex->IsValid();
#endif
    }
    ~FExclusiveLock()
    {
#if PLATFORM_ANDROID || PLATFORM_IOS
        if(Descriptor>=0){flock(Descriptor,LOCK_UN);close(Descriptor);}
#endif
    }
    FExclusiveLock(const FExclusiveLock&)=delete;
    FExclusiveLock& operator=(const FExclusiveLock&)=delete;
};
inline FString Project(const FString& ApiKey){return UHorizonExampleIdentity::Project(ApiKey);}
inline FString Name(const FString& P,const FString& U){return TEXT("horizon-chat-")+Project(P+TEXT("|")+U);}
inline FString Directory(){return FPaths::ProjectSavedDir()/TEXT("SaveGames");}
inline FString Path(const FString& Name,int32 Generation){return Directory()/(Name+LexToString(Generation)+TEXT(".chat"));}
inline FString Hash(const FString& S){return Project(S);}
inline void Add(FString& S,const FString& V){S+=LexToString(V.Len())+TEXT(":")+V;}
inline FString OriginalHash(const UHorizonChatOriginal* P){FString S;for(const auto& V:{P->Project,P->UserId,P->Action,P->OperationId,P->IssuedAt,P->ChannelId,P->MessageId,P->TextBase64})Add(S,V);return Hash(S);}
inline FString Signature(const UHorizonChatOriginal* P)
{
    FString S=OriginalHash(P);for(const auto& V:{P->State,P->Receipt.OperationId,P->Receipt.IssuedAt,P->Receipt.ChannelId,P->Receipt.MessageId,P->Receipt.CreatedAt,P->Receipt.ExpiresAt,P->ReportId,P->ReportStatus,LexToString(P->SendSerial),LexToString(P->StorageGeneration)})Add(S,V);return Hash(S);
}
inline FString Encode(const FString& S){const auto B=HorizonChatWire::Utf8(S);return FBase64::Encode(reinterpret_cast<const uint8*>(B.data()),int32(B.size()));}
inline bool Decode(const FString& Encoded,FString& Out)
{
    std::vector<uint8_t> B;if(!HorizonSharedContract::DecodeBase64(HorizonChatWire::Utf8(Encoded),B,2000))return false;const FUTF8ToTCHAR C(reinterpret_cast<const char*>(B.data()),int32(B.size()));Out=FString(C.Length(),C.Get());return Encode(Out)==Encoded&&HorizonChatWire::Text(Out);
}
inline bool Valid(const UHorizonChatOriginal* P,const FString& Project,const FString& User)
{
    FString Text;if(!P||P->Project!=Project||P->UserId!=User||!HorizonChatWire::Id(User)||!HorizonChatWire::Id(P->OperationId)||!HorizonChatWire::Time(P->IssuedAt)||!HorizonChatWire::Id(P->ChannelId)||P->SendSerial<0||P->SendSerial>=HorizonMatchesContract::MaxSafe||!Decode(P->TextBase64,Text)||(P->State!=TEXT("pending")&&P->State!=TEXT("acknowledged")&&P->State!=TEXT("expired")))return false;
    if(P->Action==TEXT("report")){if(!HorizonChatWire::Id(P->MessageId)||!HorizonChatWire::Text(Text,240,960))return false;if(P->State==TEXT("acknowledged")&&(!HorizonChatWire::Id(P->ReportId)||(P->ReportStatus!=TEXT("open")&&P->ReportStatus!=TEXT("dismissed")&&P->ReportStatus!=TEXT("actioned")&&P->ReportStatus!=TEXT("removed"))))return false;}
    else if(P->Action!=TEXT("send")||!P->MessageId.IsEmpty())return false;
    if(P->State==TEXT("acknowledged")&&P->Action==TEXT("send")){const auto& R=P->Receipt;if(R.OperationId!=P->OperationId||R.IssuedAt!=P->IssuedAt||R.ChannelId!=P->ChannelId||!HorizonChatWire::Id(R.MessageId)||!HorizonChatWire::Time(R.CreatedAt)||!HorizonChatWire::Time(R.ExpiresAt)||R.CreatedAt>=R.ExpiresAt)return false;}return true;
}
inline UHorizonChatOriginal* Copy(const UHorizonChatOriginal* P)
{
    auto* C=NewObject<UHorizonChatOriginal>();C->Project=P->Project;C->UserId=P->UserId;C->Action=P->Action;C->OperationId=P->OperationId;C->IssuedAt=P->IssuedAt;C->ChannelId=P->ChannelId;C->MessageId=P->MessageId;C->TextBase64=P->TextBase64;C->State=P->State;C->Receipt=P->Receipt;C->ReportId=P->ReportId;C->ReportStatus=P->ReportStatus;C->SendSerial=P->SendSerial;C->StorageGeneration=P->StorageGeneration;C->StorageHash=P->StorageHash;return C;
}
/** The caller MUST hold this scope's OS-wide mutex through read-modify-write. Any damaged generation blocks writes. */
inline UHorizonChatOriginal* Load(const FString& Name,bool& Damaged)
{
    Damaged=false;UHorizonChatOriginal* Latest=nullptr;int64 Previous=-1;
    for(int32 I=0;I<2;++I)
    {
        const FString File=Path(Name,I);const int64 Size=IFileManager::Get().FileSize(*File);if(Size==-1)continue;if(Size<=0||Size>32768){Damaged=true;return nullptr;}
        // Never hand damaged bytes to Unreal's object/FName deserializer: it can assert before returning null.
        FString Envelope;if(!FFileHelper::LoadFileToString(Envelope,*File)){Damaged=true;return nullptr;}
        const FString Prefix=TEXT("horizon-chat-journal:1\n");
        if(!Envelope.StartsWith(Prefix,ESearchCase::CaseSensitive)||Envelope.Len()<Prefix.Len()+66||Envelope[Prefix.Len()+64]!='\n'){Damaged=true;return nullptr;}
        const FString Digest=Envelope.Mid(Prefix.Len(),64),Encoded=Envelope.Mid(Prefix.Len()+65);
        if(!HorizonChatWire::Exact(Digest,Hash(Encoded))){Damaged=true;return nullptr;}
        std::vector<uint8_t> Decoded;if(!HorizonSharedContract::DecodeBase64(HorizonChatWire::Utf8(Encoded),Decoded,16384)||Decoded.empty()){Damaged=true;return nullptr;}
        TArray<uint8> B;B.Append(Decoded.data(),int32(Decoded.size()));
        auto* P=Cast<UHorizonChatOriginal>(UGameplayStatics::LoadGameFromMemory(B));
        if(!P||P->StorageGeneration<1||P->StorageGeneration>=HorizonMatchesContract::MaxSafe||int32(P->StorageGeneration%2)!=I||Signature(P)!=P->StorageHash||!Valid(P,P->Project,P->UserId)){Damaged=true;return nullptr;}
        if(Latest){if(FMath::Abs(P->StorageGeneration-Previous)!=1){Damaged=true;return nullptr;}}Previous=P->StorageGeneration;if(!Latest||P->StorageGeneration>Latest->StorageGeneration)Latest=P;
    }return Latest;
}
inline bool Save(const FString& Name,const UHorizonChatOriginal* Original)
{
    bool Damaged;auto* Latest=Load(Name,Damaged);if(Damaged||!Valid(Original,Original->Project,Original->UserId))return false;auto* P=Copy(Original);P->StorageGeneration=Latest?Latest->StorageGeneration+1:1;if(P->StorageGeneration>=HorizonMatchesContract::MaxSafe)return false;P->StorageHash=Signature(P);
    IFileManager::Get().MakeDirectory(*Directory(),true);TArray<uint8> Bytes;if(!UGameplayStatics::SaveGameToMemory(P,Bytes)||Bytes.Num()>16384)return false;const FString Destination=Path(Name,int32(P->StorageGeneration%2)),Temporary=Destination+TEXT(".tmp");
    const FString Encoded=FBase64::Encode(Bytes),Envelope=TEXT("horizon-chat-journal:1\n")+Hash(Encoded)+TEXT("\n")+Encoded;
    if(!FFileHelper::SaveStringToFile(Envelope,*Temporary,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)){IFileManager::Get().Delete(*Temporary);return false;}
    const bool Moved=IFileManager::Get().Move(*Destination,*Temporary,true,true,false,true);if(!Moved){IFileManager::Get().Delete(*Temporary);return false;}auto* Verified=Load(Name,Damaged);return !Damaged&&Verified&&Verified->StorageGeneration==P->StorageGeneration&&Verified->StorageHash==P->StorageHash;
}
inline bool Acknowledge(const FString& Name,const UHorizonChatOriginal* Original,const FHorizonChatReceipt& Receipt,const FString& ReportId=TEXT(""),const FString& ReportStatus=TEXT(""))
{
    FExclusiveLock Lock(Name);if(!Lock.IsValid())return false;bool Damaged;auto* P=Load(Name,Damaged);if(Damaged||!P||OriginalHash(P)!=OriginalHash(Original))return false;
    if(P->State==TEXT("acknowledged"))return P->Receipt.OperationId==Receipt.OperationId&&P->Receipt.MessageId==Receipt.MessageId&&P->ReportId==ReportId;
    if(P->Action==TEXT("send")&&(Receipt.OperationId!=P->OperationId||Receipt.IssuedAt!=P->IssuedAt||Receipt.ChannelId!=P->ChannelId))return false;P->Receipt=Receipt;P->ReportId=ReportId;P->ReportStatus=ReportStatus;P->State=TEXT("acknowledged");return Save(Name,P);
}
}
