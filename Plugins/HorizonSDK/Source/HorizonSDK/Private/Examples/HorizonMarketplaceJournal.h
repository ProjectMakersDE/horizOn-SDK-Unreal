// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Models/HorizonMarketplaceWire.h"
#include "Http/HorizonLosslessJson.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if PLATFORM_ANDROID || PLATFORM_IOS
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif
namespace HorizonMarketplaceJournal
{
inline FString Hash(const FString& S){const auto B=HorizonMarketplaceWire::Utf8(S);return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(B.data()),B.size()).c_str());}
inline FString Name(const FString& Project,const FString& User){return TEXT("horizon-marketplace-")+Hash(Project+TEXT("|")+User);}
struct FOriginal
{
    FString Project,User,ConfirmedListing;
    FHorizonMarketplaceIntent Intent;
    bool Valid() const{return HorizonMarketplaceWire::Id(Project)&&HorizonMarketplaceWire::Id(User)&&Intent.IsValid()&&Intent.User()==User&&(ConfirmedListing.IsEmpty()||HorizonMarketplaceWire::Id(ConfirmedListing)&&(Intent.Kind==TEXT("create")||Intent.ListingId==ConfirmedListing));}
};
class FLease
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    int Descriptor=-1;
#else
    TUniquePtr<FSystemWideCriticalSection> Mutex;
#endif
    FString File,Raw;
    static bool Write(const FString& Path,const FString& Content)
    {
        auto& Platform=FPlatformFileManager::Get().GetPlatformFile();if(Platform.FileExists(*Path))return false;
        const auto Bytes=HorizonMarketplaceWire::Utf8(Content);TUniquePtr<IFileHandle> Handle(Platform.OpenWrite(*Path,false,false));
        if(!Handle||!Handle->Write(reinterpret_cast<const uint8*>(Bytes.data()),Bytes.size())||!Handle->Flush(true))return false;Handle.Reset();FString Read;return FFileHelper::LoadFileToString(Read,*Path)&&Read==Content;
    }
    static FString Encode(const FOriginal& P)
    {
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("project"),P.Project);J->SetStringField(TEXT("user"),P.User);J->SetStringField(TEXT("kind"),P.Intent.Kind);J->SetStringField(TEXT("listing"),P.Intent.ListingId);J->SetStringField(TEXT("original"),P.Intent.Json);J->SetStringField(TEXT("confirmedListing"),P.ConfirmedListing);FString Body;if(!HorizonLosslessJson::Object(J,Body))return {};return TEXT("horizon-marketplace:1\n")+Hash(Body)+TEXT("\n")+Body;
    }
    static bool Decode(const FString& Raw,FOriginal& P)
    {
        using namespace HorizonMarketplaceWire;const FString Prefix=TEXT("horizon-marketplace:1\n");if(!Raw.StartsWith(Prefix,ESearchCase::CaseSensitive)||Raw.Len()<Prefix.Len()+65||Raw[Prefix.Len()+64]!='\n')return false;
        const FString Digest=Raw.Mid(Prefix.Len(),64),Body=Raw.Mid(Prefix.Len()+65);TSharedPtr<FJsonObject> J;
        return Digest==Hash(Body)&&Parse(Body,J)&&J->Values.Num()==6&&Text(J,TEXT("project"),P.Project)&&Text(J,TEXT("user"),P.User)&&Text(J,TEXT("kind"),P.Intent.Kind)&&Text(J,TEXT("listing"),P.Intent.ListingId)&&Text(J,TEXT("original"),P.Intent.Json)&&Text(J,TEXT("confirmedListing"),P.ConfirmedListing)&&P.Valid();
    }
public:
    FOriginal Original;
    bool Locked=false;
    explicit FLease(const FString& Project,const FString& User)
    {
        if(!HorizonMarketplaceWire::Id(Project)||!HorizonMarketplaceWire::Id(User))return;
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("SaveGames"),Scope=Name(Project,User);IFileManager::Get().MakeDirectory(*Directory,true);File=Directory/(Scope+TEXT(".market"));
#if PLATFORM_ANDROID || PLATFORM_IOS
        Descriptor=open(TCHAR_TO_UTF8(*(File+TEXT(".lock"))),O_CREAT|O_WRONLY,0600);if(Descriptor>=0&&flock(Descriptor,LOCK_EX|LOCK_NB)!=0){close(Descriptor);Descriptor=-1;}Locked=Descriptor>=0;
#else
        Mutex=MakeUnique<FSystemWideCriticalSection>(Scope);Locked=Mutex&&Mutex->IsValid();
#endif
        Original.Project=Project;Original.User=User;
    }
    ~FLease()
    {
#if PLATFORM_ANDROID || PLATFORM_IOS
        if(Descriptor>=0){flock(Descriptor,LOCK_UN);close(Descriptor);}
#endif
    }
    bool HasPending() const{return IFileManager::Get().FileExists(*File)||IFileManager::Get().FileExists(*(File+TEXT(".next")));}
    bool Load()
    {
        if(!Locked||!FFileHelper::LoadFileToString(Raw,*File))return false;FOriginal P;if(!Decode(Raw,P)||P.Project!=Original.Project||P.User!=Original.User)return false;
        const FString Next=File+TEXT(".next");if(IFileManager::Get().FileExists(*Next))
        {
            FString Proposed;FOriginal N;if(!FFileHelper::LoadFileToString(Proposed,*Next)||!Decode(Proposed,N)||N.Project!=P.Project||N.User!=P.User||N.Intent.Kind!=P.Intent.Kind||N.Intent.ListingId!=P.Intent.ListingId||N.Intent.Json!=P.Intent.Json||N.ConfirmedListing.IsEmpty()||!P.ConfirmedListing.IsEmpty()&&P.ConfirmedListing!=N.ConfirmedListing||!IFileManager::Get().Move(*File,*Next,true,true,false,true))return false;Raw=Proposed;P=N;
        }
        Original=MoveTemp(P);return true;
    }
    bool Begin(const FHorizonMarketplaceIntent& I)
    {
        if(!Locked||HasPending())return false;Original.Intent=I;if(!Original.Valid())return false;Raw=Encode(Original);return !Raw.IsEmpty()&&Write(File,Raw)&&Load();
    }
    bool Confirm(const FHorizonMarketplaceReceipt& R)
    {
        if(!Locked||!Original.Intent.Matches(R))return false;const auto Saved=Original;if(!Load()||Original.Intent.Json!=Saved.Intent.Json||Original.Intent.ListingId!=Saved.Intent.ListingId||Original.Intent.Kind!=Saved.Intent.Kind)return false;
        if(!Original.ConfirmedListing.IsEmpty())return Original.ConfirmedListing==R.ListingId;Original.ConfirmedListing=R.ListingId;const FString Next=Encode(Original);
        if(!Write(File+TEXT(".next"),Next)||!IFileManager::Get().Move(*File,*(File+TEXT(".next")),true,true,false,true))return false;return Load()&&Raw==Next;
    }
    bool AcknowledgeAfterRefresh()
    {
        if(!Locked||Original.ConfirmedListing.IsEmpty())return false;const FString Saved=Raw;if(!Load()||Raw!=Saved)return false;return IFileManager::Get().Delete(*File,true,true)&&!HasPending();
    }
};
}
