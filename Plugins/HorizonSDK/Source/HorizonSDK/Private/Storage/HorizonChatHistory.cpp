// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Storage/HorizonChatHistory.h"
#include "Models/HorizonChatWire.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "HAL/FileManager.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if PLATFORM_ANDROID || PLATFORM_IOS
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif
namespace HorizonChatHistoryStorage
{
constexpr int64 MaxRow=16384;
FString Hash(const FString& S){const auto B=HorizonChatWire::Utf8(S);return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(B.data()),B.size()).c_str());}
class FLock
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    int Descriptor=-1;
#else
    TUniquePtr<FSystemWideCriticalSection> Mutex;
#endif
public:
    explicit FLock(const FString& Directory)
    {
#if PLATFORM_ANDROID || PLATFORM_IOS
        Descriptor=open(TCHAR_TO_UTF8(*(Directory/TEXT(".lock"))),O_CREAT|O_WRONLY,0600);if(Descriptor>=0&&flock(Descriptor,LOCK_EX|LOCK_NB)!=0){close(Descriptor);Descriptor=-1;}
#else
        Mutex=MakeUnique<FSystemWideCriticalSection>(TEXT("horizon-chat-history-")+Hash(Directory));
#endif
    }
    bool Valid() const
    {
#if PLATFORM_ANDROID || PLATFORM_IOS
        return Descriptor>=0;
#else
        return Mutex&&Mutex->IsValid();
#endif
    }
    ~FLock(){
#if PLATFORM_ANDROID || PLATFORM_IOS
        if(Descriptor>=0){flock(Descriptor,LOCK_UN);close(Descriptor);}
#endif
    }
};
bool Valid(const FHorizonChatMessage& M,const FString& C){return HorizonChatWire::Id(C)&&M.ChannelId==C&&HorizonChatWire::Id(M.Id)&&HorizonChatWire::Id(M.AuthorId)&&HorizonChatWire::Text(M.Text)&&HorizonChatWire::Time(M.CreatedAt)&&HorizonChatWire::Time(M.ExpiresAt);}
FString Encode(const FHorizonChatMessage& M)
{
    auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("version"),1);J->SetStringField(TEXT("id"),M.Id);J->SetStringField(TEXT("channelId"),M.ChannelId);J->SetStringField(TEXT("authorId"),M.AuthorId);J->SetStringField(TEXT("text"),M.Text);J->SetStringField(TEXT("createdAt"),M.CreatedAt);J->SetStringField(TEXT("expiresAt"),M.ExpiresAt);FString S;auto W=TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&S);FJsonSerializer::Serialize(J,W);return S;
}
bool Decode(const FString& Raw,const FString& C,FHorizonChatMessage& M)
{
    TSharedPtr<FJsonObject> J;double V;if(!HorizonChatWire::Parse(Raw,J)||J->Values.Num()!=7||!J->TryGetNumberField(TEXT("version"),V)||V!=1||!J->TryGetStringField(TEXT("id"),M.Id)||!J->TryGetStringField(TEXT("channelId"),M.ChannelId)||!J->TryGetStringField(TEXT("authorId"),M.AuthorId)||!J->TryGetStringField(TEXT("text"),M.Text)||!J->TryGetStringField(TEXT("createdAt"),M.CreatedAt)||!J->TryGetStringField(TEXT("expiresAt"),M.ExpiresAt))return false;return Valid(M,C)&&Encode(M)==Raw;
}
bool AppendBytes(const FString& Path,const FString& Value)
{
    const auto Bytes=HorizonChatWire::Utf8(Value);TUniquePtr<IFileHandle> F(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Path,true,false));return F&&F->Write(reinterpret_cast<const uint8*>(Bytes.data()),Bytes.size())&&F->Flush(true);
}
bool ReadBytes(IFileHandle& F,int64 Start,int64 Size,FString& Out)
{
    if(Size<0||Size>MaxRow+1||!F.Seek(Start))return false;TArray<uint8> Bytes;Bytes.SetNumUninitialized(Size);if(!F.Read(Bytes.GetData(),Size))return false;FUTF8ToTCHAR Chars(reinterpret_cast<const char*>(Bytes.GetData()),Bytes.Num());Out=FString(Chars.Length(),Chars.Get());const auto Back=HorizonChatWire::Utf8(Out);return Back.size()==size_t(Bytes.Num())&&FMemory::Memcmp(Back.data(),Bytes.GetData(),Bytes.Num())==0;
}

}
FHorizonChatHistory::FHorizonChatHistory(const FString& Key,const FString& User,const FString& Base)
{
    Directory=(Base.IsEmpty()?FPaths::ProjectSavedDir()/TEXT("Horizon/Chat"):Base)/HorizonChatHistoryStorage::Hash(LexToString(Key.Len())+TEXT(":")+Key+LexToString(User.Len())+TEXT(":")+User);
}
FString FHorizonChatHistory::Path(const FString& C,const TCHAR* S) const{return Directory/(C+S);}
FString FHorizonChatHistory::Capture(const FString& C) const
{
    FString Combined;for(const auto& P:{Directory/TEXT("generation"),Path(C,TEXT(".generation"))}){FString S=TEXT("0");const int64 Size=IFileManager::Get().FileSize(*P);if(Size>=0&&(Size>64||!FFileHelper::LoadFileToString(S,*P)))return TEXT("invalid");Combined+=S+TEXT("/");}return HorizonChatHistoryStorage::Hash(Combined);
}
bool FHorizonChatHistory::Open(const FString& C,FIndex*& Out,FString& Error)
{
    using namespace HorizonChatHistoryStorage;const FString G=Capture(C);if(G==TEXT("invalid")){Error=TEXT("LOCAL_HISTORY_DAMAGED");return false;}auto& S=Indexes.FindOrAdd(C);if(S.Generation!=G){S={};S.Generation=G;}
    TUniquePtr<IFileHandle> Index(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Path(C,TEXT(".index"))));
    if(Index){if(Index->Size()<S.IndexEnd){S={};S.Generation=G;}while(S.IndexEnd<Index->Size()){TArray<uint8> Bytes;Bytes.SetNumUninitialized(FMath::Min(int64(200),Index->Size()-S.IndexEnd));if(!Index->Seek(S.IndexEnd)||!Index->Read(Bytes.GetData(),Bytes.Num())){Error=TEXT("LOCAL_HISTORY_IO");return false;}int32 Newline=Bytes.IndexOfByKey(uint8(10));if(Newline<0){Error=TEXT("LOCAL_HISTORY_DAMAGED");return false;}const FUTF8ToTCHAR Chars(reinterpret_cast<const char*>(Bytes.GetData()),Newline);const FString Line(Chars.Length(),Chars.Get());TArray<FString> Parts;Line.ParseIntoArray(Parts,TEXT(" "),false);int64 Offset,Length;if(Parts.Num()!=4||!HorizonChatWire::Id(Parts[0])||Parts[1].Len()!=64||!LexTryParseString(Offset,*Parts[2])||!LexTryParseString(Length,*Parts[3])||Offset!=S.End||Length<1||Length>MaxRow||S.Ids.Contains(Parts[0])){Error=TEXT("LOCAL_HISTORY_DAMAGED");return false;}S.Ids.Add(Parts[0],Parts[1]);S.End+=Length;S.IndexEnd+=Newline+1;}}
    TUniquePtr<IFileHandle> Log(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Path(C,TEXT(".jsonl"))));const int64 Size=Log?Log->Size():0;if(Size<S.End||Size-S.End>MaxRow){Error=TEXT("LOCAL_HISTORY_DAMAGED");return false;}
    if(Size>S.End){FString Raw;FHorizonChatMessage M;if(!ReadBytes(*Log,S.End,Size-S.End,Raw)||!Raw.RemoveFromEnd(TEXT("\n"))||!Decode(Raw,C,M)||S.Ids.Contains(M.Id)){Error=TEXT("LOCAL_HISTORY_DAMAGED");return false;}const FString Digest=Hash(Raw),Line=M.Id+TEXT(" ")+Digest+TEXT(" ")+LexToString(S.End)+TEXT(" ")+LexToString(Size-S.End)+TEXT("\n");if(!AppendBytes(Path(C,TEXT(".index")),Line)){Error=TEXT("LOCAL_HISTORY_IO");return false;}S.Ids.Add(M.Id,Digest);S.End=Size;S.IndexEnd=IFileManager::Get().FileSize(*Path(C,TEXT(".index")));}
    Out=&S;return true;
}
bool FHorizonChatHistory::Append(const FString& C,const FHorizonChatMessage& M,const FString& Captured,bool& Changed,FString& Error)
{
    using namespace HorizonChatHistoryStorage;Changed=false;if(!Valid(M,C)){Error=TEXT("LOCAL_HISTORY_INVALID");return false;}if(!IFileManager::Get().MakeDirectory(*Directory,true)){Error=TEXT("LOCAL_HISTORY_IO");return false;}FLock Lock(Directory);if(!Lock.Valid()){Error=TEXT("LOCAL_HISTORY_BUSY");return false;}if(Captured!=Capture(C)){Error=TEXT("LOCAL_HISTORY_STALE");return false;}FIndex* S;if(!Open(C,S,Error))return false;const FString Raw=Encode(M),Digest=Hash(Raw);if(const auto* Existing=S->Ids.Find(M.Id)){if(*Existing==Digest)return true;Error=TEXT("LOCAL_HISTORY_CONFLICT");return false;}
    const int64 Length=HorizonChatWire::Utf8(Raw).size()+1;if(Length>MaxRow){Error=TEXT("LOCAL_HISTORY_INVALID");return false;}const FString Line=M.Id+TEXT(" ")+Digest+TEXT(" ")+LexToString(S->End)+TEXT(" ")+LexToString(Length)+TEXT("\n");if(!AppendBytes(Path(C,TEXT(".jsonl")),Raw+TEXT("\n"))||!AppendBytes(Path(C,TEXT(".index")),Line)){Error=TEXT("LOCAL_HISTORY_IO");return false;}S->Ids.Add(M.Id,Digest);S->End+=Length;S->IndexEnd=IFileManager::Get().FileSize(*Path(C,TEXT(".index")));Changed=true;return true;
}
FHorizonChatHistoryPage FHorizonChatHistory::Read(const FString& C,const FString& Cursor,int32 Limit)
{
    using namespace HorizonChatHistoryStorage;FHorizonChatHistoryPage Page;if(!HorizonChatWire::Id(C)||Limit<1||Limit>100){Page.ErrorCode=TEXT("LOCAL_HISTORY_INVALID");return Page;}if(!IFileManager::Get().MakeDirectory(*Directory,true)){Page.ErrorCode=TEXT("LOCAL_HISTORY_IO");return Page;}FLock Lock(Directory);if(!Lock.Valid()){Page.ErrorCode=TEXT("LOCAL_HISTORY_BUSY");return Page;}FIndex* S;if(!Open(C,S,Page.ErrorCode))return Page;int64 End=S->End;const FString G=Capture(C);if(!Cursor.IsEmpty()){FString A,B;if(!Cursor.Split(TEXT(":"),&A,&B)||A!=G||!LexTryParseString(End,*B)||End<0||End>S->End){Page.ErrorCode=TEXT("LOCAL_HISTORY_STALE");return Page;}}
    TUniquePtr<IFileHandle> Log(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Path(C,TEXT(".jsonl"))));if(End>0&&!Log){Page.ErrorCode=TEXT("LOCAL_HISTORY_IO");return Page;}
    while(End>0&&Page.Messages.Num()<Limit){const int64 Start=FMath::Max(int64(0),End-MaxRow-1);TArray<uint8> Bytes;Bytes.SetNumUninitialized(End-Start);if(!Log->Seek(Start)||!Log->Read(Bytes.GetData(),Bytes.Num())||Bytes.Last()!=10){Page.ErrorCode=TEXT("LOCAL_HISTORY_DAMAGED");return Page;}int32 At=Bytes.Num()-2;while(At>=0&&Bytes[At]!=10)--At;if(At<0&&Start>0){Page.ErrorCode=TEXT("LOCAL_HISTORY_DAMAGED");return Page;}const FUTF8ToTCHAR Chars(reinterpret_cast<const char*>(Bytes.GetData()+At+1),Bytes.Num()-At-2);const FString Raw(Chars.Length(),Chars.Get());FHorizonChatMessage M;if(!Decode(Raw,C,M)||S->Ids.FindRef(M.Id)!=Hash(Raw)){Page.ErrorCode=TEXT("LOCAL_HISTORY_DAMAGED");return Page;}Page.Messages.Add(M);End=Start+At+1;}
    Page.bSuccess=true;if(End>0)Page.NextCursor=G+TEXT(":")+LexToString(End);return Page;
}
bool FHorizonChatHistory::Clear(const FString& C,FString& Error)
{
    using namespace HorizonChatHistoryStorage;if(!C.IsEmpty()&&!HorizonChatWire::Id(C)){Error=TEXT("LOCAL_HISTORY_INVALID");return false;}if(!IFileManager::Get().MakeDirectory(*Directory,true)){Error=TEXT("LOCAL_HISTORY_IO");return false;}FLock Lock(Directory);if(!Lock.Valid()){Error=TEXT("LOCAL_HISTORY_BUSY");return false;}const FString Marker=C.IsEmpty()?Directory/TEXT("generation"):Path(C,TEXT(".generation"));if(!FFileHelper::SaveStringToFile(FGuid::NewGuid().ToString(EGuidFormats::Digits),*Marker,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)){Error=TEXT("LOCAL_HISTORY_IO");return false;}TArray<FString> Names;IFileManager::Get().FindFiles(Names,*(Directory/TEXT("*")),true,false);for(const auto& Name:Names)if((C.IsEmpty()||Name.StartsWith(C+TEXT(".")))&&(Name.EndsWith(TEXT(".jsonl"))||Name.EndsWith(TEXT(".index"))))if(!IFileManager::Get().Delete(*(Directory/Name))){Error=TEXT("LOCAL_HISTORY_IO");return false;}Indexes.Empty();return true;
}
