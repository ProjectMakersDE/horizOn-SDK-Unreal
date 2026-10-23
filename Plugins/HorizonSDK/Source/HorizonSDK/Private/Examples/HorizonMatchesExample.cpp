// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Examples/HorizonMatchesExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonMatchesManager.h"
#include "Managers/HorizonRealtimeManager.h"
#include "Models/HorizonMatchesWire.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Base64.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
namespace HorizonMatchStorage
{
FString Project(UHorizonSubsystem* S){return UHorizonExampleIdentity::Project(S->GetHttpClient()->GetApiKey());}
FString Slot(const FString& P,const FString& U){return TEXT("horizon-match-original-")+UHorizonExampleIdentity::Project(P+TEXT("|")+U);}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Exact(const FString& A,const FString& B){return A.Len()==B.Len()&&(!A.Len()||FMemory::Memcmp(*A,*B,A.Len()*sizeof(TCHAR))==0);}
void Add(FString& Out,const FString& Value){Out+=LexToString(Value.Len())+TEXT(":")+Value;}
FString Hash(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(B.Get()),B.Length()).c_str());}
FString Prefix(const FString& Id){return TEXT("horizon-match:")+Id+TEXT(" ");}
void AddIds(FString& S,const TArray<FString>& Values){Add(S,LexToString(Values.Num()));for(const auto& V:Values)Add(S,V);}
FString OriginalHash(const UHorizonMatchOriginal* P)
{
    if(!P)return {};FString S;for(const FString& V:{P->Project,P->UserId,P->IntentId,P->Action,P->MatchId,LexToString(P->ExpectedRevision),P->Create.Name,LexToString(P->Create.Capacity),P->Create.Visibility,LexToString(P->Create.Strength),LexToString(P->Create.TurnSeconds),P->Create.TimeoutPolicy,LexToString(P->Create.bRanked),FBase64::Encode(P->Create.Data),FBase64::Encode(P->Data),LexToString(P->bFinish)})Add(S,V);AddIds(S,P->Create.InviteeIds);AddIds(S,P->Ids);return Hash(S);
}
FString SubmissionHash(const UHorizonMatchOriginal* P){return P?Hash(P->Project+TEXT("|")+P->UserId+TEXT("|")+P->MatchId+TEXT("|")+LexToString(P->ExpectedRevision)+TEXT("|")+P->Run.RunId+TEXT("|")+LexToString(P->Score)+TEXT("|")+FBase64::Encode(P->InputLog)):FString();}
FString Signature(const UHorizonMatchOriginal* P)
{
    if(!P)return {};FString S=OriginalHash(P);for(const FString& V:{P->OriginalHash,LexToString(P->SendSerial),LexToString(P->bConfirmed),P->ResolvedMatchId,P->Run.RunId,P->Run.Ticket,LexToString(P->Run.Seed),P->Run.LeaderboardKey,P->Run.IssuedAt,P->Run.ExpiresAt,LexToString(P->Run.ExpiresInSeconds),LexToString(P->bSubmitPrepared),LexToString(P->Score),FBase64::Encode(P->InputLog),P->SubmitHash,LexToString(P->bSubmitConfirmed),P->RejectionCode,P->TerminalCode})Add(S,V);return Hash(S);
}
UHorizonMatchOriginal* Copy(const UHorizonMatchOriginal* P)
{
    if(!P)return nullptr;auto* C=NewObject<UHorizonMatchOriginal>();C->Project=P->Project;C->UserId=P->UserId;C->IntentId=P->IntentId;C->Action=P->Action;C->MatchId=P->MatchId;C->ExpectedRevision=P->ExpectedRevision;C->Create=P->Create;C->CreateNameBase64=P->CreateNameBase64;C->Data=P->Data;C->Ids=P->Ids;C->bFinish=P->bFinish;C->SendSerial=P->SendSerial;C->OriginalHash=P->OriginalHash;C->bConfirmed=P->bConfirmed;C->ResolvedMatchId=P->ResolvedMatchId;C->Run=P->Run;C->RunBoardBase64=P->RunBoardBase64;C->bSubmitPrepared=P->bSubmitPrepared;C->Score=P->Score;C->InputLog=P->InputLog;C->SubmitHash=P->SubmitHash;C->bSubmitConfirmed=P->bSubmitConfirmed;C->StorageGeneration=P->StorageGeneration;C->StorageHash=P->StorageHash;C->RejectionCode=P->RejectionCode;C->TerminalCode=P->TerminalCode;return C;
}
FString Encode(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return FBase64::Encode(reinterpret_cast<const uint8*>(B.Get()),B.Length());}
bool Decode(const FString& Encoded,int32 Limit,FString& S)
{
    const FTCHARToUTF8 B(*Encoded,Encoded.Len()+1);std::vector<uint8_t> Bytes;if(!HorizonSharedContract::DecodeBase64(std::string(B.Get(),B.Length()),Bytes,Limit))return false;Bytes.push_back('x');const FUTF8ToTCHAR Chars(reinterpret_cast<const char*>(Bytes.data()),int32(Bytes.size()));const int32 Count=Chars.Length()-1;auto& Out=S.GetCharArray();Out.SetNumUninitialized(Count+1);if(Count)FMemory::Memcpy(Out.GetData(),Chars.Get(),Count*sizeof(TCHAR));Out[Count]=0;return HorizonMatchesWire::TextValue(S,Limit)&&Encode(S)==Encoded;
}
bool Exists(const FString& Slot){return UGameplayStatics::DoesSaveGameExist(Slot,0)||UGameplayStatics::DoesSaveGameExist(Slot+TEXT("-next"),0);}
UHorizonMatchOriginal* ReadGeneration(const FString& Slot)
{
    auto* P=UGameplayStatics::DoesSaveGameExist(Slot,0)?Cast<UHorizonMatchOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0)):nullptr;
    return P&&P->StorageGeneration>0&&P->StorageGeneration<HorizonMatchesContract::MaxSafe&&Decode(P->CreateNameBase64,120,P->Create.Name)&&Decode(P->RunBoardBase64,1024,P->Run.LeaderboardKey)&&P->StorageHash==Hash(Signature(P)+TEXT("|")+LexToString(P->StorageGeneration))?P:nullptr;
}
UHorizonMatchOriginal* Load(const FString& Slot)
{
    auto* A=ReadGeneration(Slot);auto* B=ReadGeneration(Slot+TEXT("-next"));return B&&(!A||B->StorageGeneration>A->StorageGeneration)?B:A;
}
bool Save(const UHorizonMatchOriginal* P,const FString& Slot)
{
    auto* A=ReadGeneration(Slot);auto* B=ReadGeneration(Slot+TEXT("-next"));auto* Latest=B&&(!A||B->StorageGeneration>A->StorageGeneration)?B:A;
    if(Exists(Slot)&&!Latest)return false;auto* C=Copy(P);if(!C)return false;C->StorageGeneration=Latest?Latest->StorageGeneration+1:1;if(C->StorageGeneration>=HorizonMatchesContract::MaxSafe)return false;
    C->StorageHash=Hash(Signature(C)+TEXT("|")+LexToString(C->StorageGeneration));const FString Expected=Signature(C);const int64 Generation=C->StorageGeneration;
    const FString Destination=Latest==A&&Latest?Slot+TEXT("-next"):Slot;C->CreateNameBase64=Encode(C->Create.Name);C->Create.Name.Reset();C->RunBoardBase64=Encode(C->Run.LeaderboardKey);C->Run.LeaderboardKey.Reset();
    if(!UGameplayStatics::SaveGameToSlot(C,Destination,0))return false;auto* Verified=ReadGeneration(Destination);return Verified&&Verified->StorageGeneration==Generation&&Signature(Verified)==Expected;
}
bool Delete(const FString& Slot)
{
    auto* A=ReadGeneration(Slot);auto* B=ReadGeneration(Slot+TEXT("-next"));const bool BLatest=B&&(!A||B->StorageGeneration>A->StorageGeneration);const FString First=BLatest?Slot:Slot+TEXT("-next"),Last=BLatest?Slot+TEXT("-next"):Slot;
    for(const auto& Name:{First,Last})if(UGameplayStatics::DoesSaveGameExist(Name,0)&&!UGameplayStatics::DeleteGameInSlot(Name,0))return false;return !Exists(Slot);
}
bool DefinitiveCreateRejection(const FString& Code)
{
    return Code==TEXT("STATE_SIZE_LIMIT_REACHED")||Code==TEXT("REVISION_CONFLICT")||Code==TEXT("ACTIVE_MATCH_LIMIT_REACHED")||Code==TEXT("HISTORY_LIMIT_REACHED")||Code==TEXT("INVITE_LIMIT_REACHED")||Code==TEXT("MATCH_RELATION_BLOCKED")||Code==TEXT("RANKED_NOT_CONFIGURED")||Code==TEXT("PLAYER_NOT_FOUND")||Code==TEXT("INVALID_REQUEST");
}
bool Ids(const TArray<FString>& Values,int32 Max,bool Sorted=false)
{
    if(Values.Num()>Max)return false;TSet<FString> Seen;FString Last;for(const auto& V:Values){if(!Uuid(V)||Seen.Contains(V.ToLower())||(Sorted&&!Last.IsEmpty()&&Last.Compare(V,ESearchCase::IgnoreCase)>=0))return false;Seen.Add(V.ToLower());Last=V;}return true;
}
bool Contains(const TArray<FString>& V,const FString& Id){return V.ContainsByPredicate([&](const FString& S){return S.Equals(Id,ESearchCase::IgnoreCase);});}
bool Valid(const UHorizonMatchOriginal* P,const FString& Project,const FString& User)
{
    if(!P||P->Project!=Project||P->UserId!=User||!Uuid(P->IntentId)||!HorizonMatchesContract::Revision(P->ExpectedRevision)||P->SendSerial<0||P->SendSerial>=HorizonMatchesContract::MaxSafe||P->OriginalHash!=OriginalHash(P)||P->Data.Num()>int32(HorizonMatchesContract::MaxStateBytes)||!Ids(P->Ids,8)||(!P->ResolvedMatchId.IsEmpty()&&!Uuid(P->ResolvedMatchId)))return false;
    const auto& I=P->Create;if(!HorizonMatchesWire::TextValue(I.Name,120)||I.Capacity<2||I.Capacity>8||(I.Visibility!=TEXT("private")&&I.Visibility!=TEXT("open"))||!HorizonMatchesContract::Safe(I.Strength)||I.TurnSeconds<60||I.TurnSeconds>604800||(I.TimeoutPolicy!=TEXT("skip")&&I.TimeoutPolicy!=TEXT("forfeit"))||I.Data.Num()>int32(HorizonMatchesContract::MaxStateBytes)||!Ids(I.InviteeIds,7)||I.InviteeIds.Num()>=I.Capacity||Contains(I.InviteeIds,User))return false;
    if(P->Action==TEXT("create")){if(!P->MatchId.IsEmpty()||P->SendSerial>1||!I.Name.StartsWith(Prefix(P->IntentId),ESearchCase::CaseSensitive)||(P->bConfirmed?!Uuid(P->ResolvedMatchId):!P->ResolvedMatchId.IsEmpty()))return false;}
    else if(!Uuid(P->MatchId)||(P->Action!=TEXT("join")&&P->Action!=TEXT("decline")&&P->Action!=TEXT("start")&&P->Action!=TEXT("turn")&&P->Action!=TEXT("timeout")&&P->Action!=TEXT("leave")&&P->Action!=TEXT("cancel")&&P->Action!=TEXT("result")&&P->Action!=TEXT("ranked")))return false;
    if(P->Action==TEXT("result")&&(P->ExpectedRevision==0||P->Data.Num()>256||!Ids(P->Ids,8,true)))return false;if(P->Action==TEXT("start")&&P->Ids.Num()<2)return false;if(P->Action==TEXT("ranked")&&P->ExpectedRevision==0)return false;
    if(P->Run.IsValid()&&(P->Action!=TEXT("ranked")||!P->bConfirmed||!Uuid(P->Run.RunId)))return false;
    if(P->bSubmitPrepared&&(P->Action!=TEXT("ranked")||!P->Run.IsValid()||!HorizonMatchesContract::Safe(P->Score)||P->InputLog.Num()>65536||P->SubmitHash!=SubmissionHash(P)))return false;if(P->bSubmitConfirmed&&!P->bSubmitPrepared)return false;return true;
}
bool CreateMatches(const UHorizonMatchOriginal* P,const FHorizonMatchView& M)
{
    return M.bPresent&&M.bHasData&&M.CreatorId.Equals(P->UserId,ESearchCase::IgnoreCase)&&Exact(M.Name,P->Create.Name)&&M.Capacity==P->Create.Capacity&&M.Visibility==P->Create.Visibility&&M.Strength==P->Create.Strength&&M.TurnSeconds==P->Create.TurnSeconds&&M.TimeoutPolicy==P->Create.TimeoutPolicy&&M.bRanked==P->Create.bRanked&&M.Data==P->Create.Data;
}
bool SameRun(const FHorizonValidatedRun& A,const FHorizonValidatedRun& B){return A.RunId==B.RunId&&A.Seed==B.Seed&&A.IssuedAt==B.IssuedAt&&A.ExpiresAt==B.ExpiresAt&&Exact(A.LeaderboardKey,B.LeaderboardKey);}
}
UHorizonSubsystem* AHorizonMatchesExample::Session() const{auto* GI=GetGameInstance();auto* S=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;return S&&S->Auth&&S->Auth->IsSignedIn()&&S->Matches?S:nullptr;}
bool AHorizonMatchesExample::Fresh() const{auto* S=Session();return S&&SnapshotContext==S->GetHttpClient()->GetRequestContextEpoch()&&SnapshotGeneration==S->Matches->GetCacheGeneration();}
FHorizonMyMatches AHorizonMatchesExample::GetMatches() const{return Fresh()?MyMatches:FHorizonMyMatches{};}
FHorizonMatchRead AHorizonMatchesExample::GetMatch() const{return Fresh()?Match:FHorizonMatchRead{};}
FHorizonMatchPage AHorizonMatchesExample::GetOpenMatches() const{return Fresh()?Open:FHorizonMatchPage{};}
bool AHorizonMatchesExample::AllowRead(const FString& Key,bool Continuation,uint64 Context)
{
    if(Context!=SnapshotContext)RefreshAfter.Empty();const double Now=FPlatformTime::Seconds();for(auto It=RefreshAfter.CreateIterator();It;++It)if(It.Value()<=Now)It.RemoveCurrent();if(Continuation)return true;if(const auto* Until=RefreshAfter.Find(Key))if(*Until>Now)return false;return RefreshAfter.Num()<256;
}
void AHorizonMatchesExample::MarkRead(const FString& Key,int32 Poll){RefreshAfter.Add(Key,FPlatformTime::Seconds()+Poll);}
void AHorizonMatchesExample::Refresh(const FString& Id)
{
    auto* S=Session();if(!S||Busy||(!Id.IsEmpty()&&!HorizonMatchStorage::Uuid(Id)))return;const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();const FString Key=Id.IsEmpty()?TEXT("ledger"):TEXT("match|")+Id;if(!AllowRead(Key,false,Context))return;
    const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);auto* Original=HorizonMatchStorage::Load(Slot);const FString Signature=HorizonMatchStorage::Valid(Original,P,U)?HorizonMatchStorage::Signature(Original):FString();
    const FHorizonMyMatches RetainedLedger=Fresh()?MyMatches:FHorizonMyMatches{};Busy=true;MyMatches={};Match={};Open={};OriginalAtRefresh.Reset();bRefreshedNotFound=false;LastErrorCode.Reset();
    if(Id.IsEmpty())
    {
        S->Matches->GetMatches(U,FOnMatchesComplete::CreateWeakLambda(this,[this,Context,Key,Signature](bool Ok,const FHorizonMyMatches& V,const FString& Code,const FString&){Busy=false;auto* Now=Session();if(!Ok||!Now||Now->GetHttpClient()->GetRequestContextEpoch()!=Context){LastErrorCode=Code;return;}MyMatches=V;SnapshotContext=Context;SnapshotGeneration=Now->Matches->GetCacheGeneration();OriginalAtRefresh=Signature;RefreshedMatchId.Reset();MarkRead(Key,V.PollAfterSeconds);}));return;
    }
    S->Matches->GetMatch(U,Id,FOnMatchComplete::CreateWeakLambda(this,[this,Context,Id,Key,Signature,RetainedLedger](bool Ok,const FHorizonMatchRead& R,const FString& Code,const FString&){Busy=false;auto* Now=Session();if(!Now||Now->GetHttpClient()->GetRequestContextEpoch()!=Context){LastErrorCode=Code;return;}if(!Ok){LastErrorCode=Code;if(Code==TEXT("MATCH_NOT_FOUND")&&RetainedLedger.bPresent){MyMatches=RetainedLedger;SnapshotContext=Context;SnapshotGeneration=Now->Matches->GetCacheGeneration();OriginalAtRefresh=Signature;RefreshedMatchId=Id;bRefreshedNotFound=true;MarkRead(Key,RetainedLedger.PollAfterSeconds);}return;}MyMatches=RetainedLedger;Match=R;SnapshotContext=Context;SnapshotGeneration=Now->Matches->GetCacheGeneration();OriginalAtRefresh=Signature;RefreshedMatchId=Id;MarkRead(Key,R.PollAfterSeconds);}));
}

void AHorizonMatchesExample::FindOpen(int64 Min,int64 Max,const FString& Cursor)
{
    auto* S=Session();if(!S||Busy)return;const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();const FString Key=TEXT("open|")+LexToString(Min)+TEXT("|")+LexToString(Max);if(!AllowRead(Key,!Cursor.IsEmpty(),Context))return;Busy=true;MyMatches={};Match={};Open={};OriginalAtRefresh.Reset();bRefreshedNotFound=false;LastErrorCode.Reset();
    S->Matches->FindOpenMatches(S->Auth->GetCurrentUser().UserId,Min,Max,FOnOpenMatchesComplete::CreateWeakLambda(this,[this,Context,Key](bool Ok,const FHorizonMatchPage& R,const FString& Code,const FString&){Busy=false;auto* Now=Session();if(!Ok||!Now||Now->GetHttpClient()->GetRequestContextEpoch()!=Context){LastErrorCode=Code;return;}Open=R;SnapshotContext=Context;SnapshotGeneration=Now->Matches->GetCacheGeneration();MarkRead(Key,R.PollAfterSeconds);}),20,Cursor);
}
bool AHorizonMatchesExample::WatchMatch(const FString& Id){auto* S=Session();return S&&S->Realtime&&HorizonMatchStorage::Uuid(Id)&&S->Realtime->Subscribe(TEXT("matches:")+Id);}
UHorizonMatchOriginal* AHorizonMatchesExample::Prepare(const FString& Action,const FString& Id,int64 Revision,const FHorizonMatchCreateInput& Create,const TArray<uint8>& Data,const TArray<FString>& Ids,bool Finish)
{
    auto* S=Session();if(!S||Busy||!Fresh())return nullptr;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);if(HorizonMatchStorage::Exists(Slot))return nullptr;
    if(Action==TEXT("create")){if(!MyMatches.bPresent||MyMatches.Revision!=Revision)return nullptr;}
    else
    {
        const bool Known=Match.bPresent&&Match.Match.Id.Equals(Id,ESearchCase::IgnoreCase);const auto* Public=Open.Matches.FindByPredicate([&](const FHorizonMatchSummary& M){return M.Id.Equals(Id,ESearchCase::IgnoreCase)&&M.Revision==Revision;});
        if(Action==TEXT("join")&&Public){}
        else if(!Known||((Action==TEXT("result")||Action==TEXT("ranked"))?(!Match.Match.bHasFinalStateRevision||Match.Match.FinalStateRevision!=Revision):Match.Match.Revision!=Revision))return nullptr;
        if(Action==TEXT("ranked")&&(!Known||Match.Match.Status!=TEXT("confirmed")||!Match.Match.bRanked))return nullptr;
    }
    auto* Original=NewObject<UHorizonMatchOriginal>();Original->Project=P;Original->UserId=U;Original->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);Original->Action=Action;Original->MatchId=Id;Original->ExpectedRevision=Revision;Original->Create=Create;Original->Data=Data;Original->Ids=Ids;Original->bFinish=Finish;if(Action==TEXT("create"))Original->Create.Name=HorizonMatchStorage::Prefix(Original->IntentId)+Create.Name;Original->OriginalHash=HorizonMatchStorage::OriginalHash(Original);
    if(!HorizonMatchStorage::Valid(Original,P,U)||!HorizonMatchStorage::Save(Original,Slot))return nullptr;return Original;
}
void AHorizonMatchesExample::CreateLobby(const FHorizonMatchCreateInput& Input){auto* P=Prepare(TEXT("create"),TEXT(""),GetMatches().Revision,Input,{},{},false);if(P)Send(P,HorizonMatchStorage::Slot(P->Project,P->UserId));}
void AHorizonMatchesExample::StartRankedFromPendingResult(const FString& Id)
{
    auto* S=Session();if(!S||Busy||!Fresh()||!MyMatches.bPresent)return;const auto* Source=MyMatches.PendingResults.FindByPredicate([&](const FHorizonMatchPendingResult& P){return P.MatchId.Equals(Id,ESearchCase::IgnoreCase);});if(!Source||!Source->bHasOwnRankedRunId)return;
    const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);if(HorizonMatchStorage::Exists(Slot))return;
    auto* Original=NewObject<UHorizonMatchOriginal>();Original->Project=P;Original->UserId=U;Original->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);Original->Action=TEXT("ranked");Original->MatchId=Source->MatchId;Original->ExpectedRevision=Source->FinalStateRevision;Original->OriginalHash=HorizonMatchStorage::OriginalHash(Original);
    if(HorizonMatchStorage::Valid(Original,P,U)&&HorizonMatchStorage::Save(Original,Slot))Send(Original,Slot);
}
void AHorizonMatchesExample::StartAction(const FString& Action,const FString& Id,int64 Revision,const TArray<uint8>& Data,const TArray<FString>& Ids,bool Finish){auto* P=Prepare(Action,Id,Revision,{},Data,Ids,Finish);if(P)Send(P,HorizonMatchStorage::Slot(P->Project,P->UserId));}
void AHorizonMatchesExample::Send(UHorizonMatchOriginal* P,const FString& Slot)
{
    auto* S=Session();if(!S||Busy||!HorizonMatchStorage::Valid(P,HorizonMatchStorage::Project(S),S->Auth->GetCurrentUser().UserId)||(P->bConfirmed&&P->Action!=TEXT("ranked"))||(P->Action==TEXT("create")&&P->SendSerial!=0))return;
    ++P->SendSerial;if(!HorizonMatchStorage::Save(P,Slot))return;Busy=true;bRefreshedNotFound=false;MyMatches={};Match={};Open={};RefreshAfter.Remove(TEXT("ledger"));RefreshAfter.Remove(TEXT("match|")+P->MatchId);const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();TStrongObjectPtr<UHorizonMatchOriginal> Original(HorizonMatchStorage::Copy(P));
    const auto Complete=[this,Slot,Original,Context](bool Ok,const FHorizonMatchRead& View,const FString& Resolved,const FHorizonValidatedRun& Run,const FString& Code)
    {
        Busy=false;auto* Current=Session();if(!Ok){LastErrorCode=Code;if(Current&&Current->GetHttpClient()->GetRequestContextEpoch()==Context){auto* Saved=HorizonMatchStorage::Load(Slot);if(Saved&&HorizonMatchStorage::Signature(Saved)==HorizonMatchStorage::Signature(Original.Get())){if(Saved->Action==TEXT("create")&&Saved->SendSerial==1&&HorizonMatchStorage::DefinitiveCreateRejection(Code))Saved->RejectionCode=Code;if(Saved->Action==TEXT("ranked")&&Saved->SendSerial==1&&!Saved->Run.IsValid()&&!Saved->bSubmitPrepared&&(Code==TEXT("RANKED_NOT_CONFIGURED")||Code==TEXT("FEATURE_NOT_SUPPORTED")))Saved->RejectionCode=Code;if(Saved->Action==TEXT("ranked")&&(Code==TEXT("RUN_EXPIRED")||Code==TEXT("RUN_ALREADY_USED")))Saved->TerminalCode=Code;HorizonMatchStorage::Save(Saved,Slot);}}UE_LOG(LogTemp,Warning,TEXT("[MatchesExample] Original retained; explicit reconciliation: %s"),*Code);return;}if(!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=Context)return;auto* Saved=HorizonMatchStorage::Load(Slot);if(!Saved||HorizonMatchStorage::Signature(Saved)!=HorizonMatchStorage::Signature(Original.Get()))return;
        if(Run.IsValid()&&Saved->Run.IsValid()&&!HorizonMatchStorage::SameRun(Saved->Run,Run)){if(Current->ValidatedActions&&Current->ValidatedActions->GetCurrentRun().RunId==Run.RunId)Current->ValidatedActions->DiscardRun();LastErrorCode=TEXT("INVALID_RESPONSE");return;}Saved->bConfirmed=true;Saved->ResolvedMatchId=Resolved;if(Run.IsValid())Saved->Run=Run;if(!HorizonMatchStorage::Save(Saved,Slot))return;
        Match=View;SnapshotContext=Context;SnapshotGeneration=Current->Matches->GetCacheGeneration();LastErrorCode.Reset();if(Saved->Action!=TEXT("ranked"))HorizonMatchStorage::Delete(Slot);
    };
    const auto D=FOnMatchComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonMatchRead& R,const FString& Code,const FString&){Complete(Ok,R,R.Match.Id,{},Code);});auto* M=S->Matches;
    if(P->Action==TEXT("create"))M->CreateMatch(P->UserId,P->ExpectedRevision,P->Create,FOnMatchCreatedComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonMatchCreated& R,const FString& Code,const FString&){FHorizonMatchRead V;V.bPresent=R.bPresent;V.Match=R.Match;V.PollAfterSeconds=R.PollAfterSeconds;V.ServerTime=R.ServerTime;Complete(Ok,V,R.Match.Id,{},Code);}));
    else if(P->Action==TEXT("join"))M->JoinMatch(P->UserId,P->MatchId,P->ExpectedRevision,D);
    else if(P->Action==TEXT("decline"))M->DeclineInvite(P->UserId,P->MatchId,P->ExpectedRevision,D);
    else if(P->Action==TEXT("start"))M->StartMatch(P->UserId,P->MatchId,P->ExpectedRevision,P->Ids,D);
    else if(P->Action==TEXT("turn"))M->SubmitTurn(P->UserId,P->MatchId,P->ExpectedRevision,P->Data,P->bFinish,D);
    else if(P->Action==TEXT("timeout"))M->AdvanceTimeout(P->UserId,P->MatchId,P->ExpectedRevision,D);
    else if(P->Action==TEXT("leave"))M->LeaveMatch(P->UserId,P->MatchId,P->ExpectedRevision,D);
    else if(P->Action==TEXT("cancel"))M->CancelMatch(P->UserId,P->MatchId,P->ExpectedRevision,D);
    else if(P->Action==TEXT("result"))M->ReportResult(P->UserId,P->MatchId,P->ExpectedRevision,P->Data,P->Ids,FOnMatchReportedComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonMatchReported& R,const FString& Code,const FString&){FHorizonMatchRead V;V.bPresent=R.bPresent;V.Match=R.Match;V.PollAfterSeconds=R.PollAfterSeconds;V.ServerTime=R.ServerTime;Complete(Ok,V,R.Match.Id,{},Code);}));
    else M->StartRankedRun(P->UserId,P->MatchId,P->ExpectedRevision,FOnValidatedRunStarted::CreateWeakLambda(this,[Complete,Original](bool Ok,const FHorizonValidatedRun& R,const FString& Code,const FString&){Complete(Ok,{},Original->MatchId,R,Code);}));
}
void AHorizonMatchesExample::RetryOriginal()
{
    auto* S=Session();if(!S||Busy)return;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);auto* Original=HorizonMatchStorage::Load(Slot);if(!HorizonMatchStorage::Valid(Original,P,U))return;
    if(Original->bSubmitConfirmed||(Original->bConfirmed&&Original->Action!=TEXT("ranked"))){HorizonMatchStorage::Delete(Slot);return;}
    if(Original->Action==TEXT("create")&&Original->SendSerial!=0){Refresh();return;}Send(Original,Slot);
}
bool AHorizonMatchesExample::ConfirmRecoveredCreate(const FString& Id)
{
    auto* S=Session();if(!S||Busy||!Fresh()||!MyMatches.bPresent||!Match.bPresent||!Match.Match.Id.Equals(Id,ESearchCase::IgnoreCase))return false;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);auto* Original=HorizonMatchStorage::Load(Slot);
    if(!HorizonMatchStorage::Valid(Original,P,U)||Original->Action!=TEXT("create")||Original->SendSerial!=1||OriginalAtRefresh!=HorizonMatchStorage::Signature(Original)||!HorizonMatchStorage::CreateMatches(Original,Match.Match))return false;
    int32 Matches=0;for(const auto& R:MyMatches.Active)if(HorizonMatchStorage::Exact(R.Name,Original->Create.Name)&&R.CreatorId.Equals(U,ESearchCase::IgnoreCase))++Matches;for(const auto& R:MyMatches.Recent)if(HorizonMatchStorage::Exact(R.Name,Original->Create.Name)&&R.CreatorId.Equals(U,ESearchCase::IgnoreCase))++Matches;if(Matches!=1)return false;
    Original->bConfirmed=true;Original->ResolvedMatchId=Id;if(!HorizonMatchStorage::Save(Original,Slot))return false;return HorizonMatchStorage::Delete(Slot);
}
bool AHorizonMatchesExample::AcknowledgeAfterRefresh()
{
    auto* S=Session();if(!S||Busy||!Fresh())return false;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);auto* Original=HorizonMatchStorage::Load(Slot);
    if(!HorizonMatchStorage::Valid(Original,P,U)||OriginalAtRefresh!=HorizonMatchStorage::Signature(Original))return false;
    if(Original->Action==TEXT("create"))
    {
        if(!MyMatches.bPresent||!RefreshedMatchId.IsEmpty()||!HorizonMatchStorage::DefinitiveCreateRejection(Original->RejectionCode))return false;
        for(const auto* Rows:{&MyMatches.Active,&MyMatches.Recent})for(const auto& M:*Rows)if(HorizonMatchStorage::Exact(M.Name,Original->Create.Name))return false;
    }
    else
    {
        if(RefreshedMatchId.IsEmpty()||!Original->MatchId.Equals(RefreshedMatchId,ESearchCase::IgnoreCase))return false;
        if(!Match.bPresent){if(!bRefreshedNotFound||!MyMatches.bPresent)return false;for(const auto* Rows:{&MyMatches.Active,&MyMatches.Invites,&MyMatches.Recent})for(const auto& M:*Rows)if(M.Id.Equals(RefreshedMatchId,ESearchCase::IgnoreCase))return false;OriginalAtRefresh.Reset();return HorizonMatchStorage::Delete(Slot);}
        if(Original->Action==TEXT("ranked")&&(Original->TerminalCode!=TEXT("RUN_EXPIRED")&&Original->TerminalCode!=TEXT("RUN_ALREADY_USED"))&&!(Original->SendSerial==1&&!Original->Run.IsValid()&&!Original->bSubmitPrepared&&(Original->RejectionCode==TEXT("RANKED_NOT_CONFIGURED")||Original->RejectionCode==TEXT("FEATURE_NOT_SUPPORTED"))))return false;
    }
    OriginalAtRefresh.Reset();return HorizonMatchStorage::Delete(Slot);
}
void AHorizonMatchesExample::SubmitRankedResult(int64 Score,const TArray<uint8>& Log)
{
    auto* S=Session();if(!S||Busy||!S->ValidatedActions||!HorizonMatchesContract::Safe(Score)||Log.Num()>65536)return;S->Matches->GetCacheGeneration();const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonMatchStorage::Project(S),Slot=HorizonMatchStorage::Slot(P,U);auto* Original=HorizonMatchStorage::Load(Slot);
    if(!HorizonMatchStorage::Valid(Original,P,U)||Original->Action!=TEXT("ranked")||!Original->bConfirmed||!Original->Run.IsValid()||S->ValidatedActions->GetCurrentRun().RunId!=Original->Run.RunId)return;
    if(Original->bSubmitPrepared){if(Original->Score!=Score||Original->InputLog!=Log)return;}else {Original->bSubmitPrepared=true;Original->Score=Score;Original->InputLog=Log;Original->SubmitHash=HorizonMatchStorage::SubmissionHash(Original);if(!HorizonMatchStorage::Save(Original,Slot))return;}
    TStrongObjectPtr<UHorizonMatchOriginal> Capture(HorizonMatchStorage::Copy(Original));const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();Busy=true;
    S->ValidatedActions->SubmitValidated(Original->Score,Original->InputLog,TEXT("match_result_demo"),TEXT(""),{},FOnValidatedSubmitComplete::CreateWeakLambda(this,[this,Slot,Capture,Context](bool Ok,const FHorizonValidatedSubmitResult& Receipt,const FString& Code,const FString&)
    {
        Busy=false;auto* Now=Session();if(!Ok){LastErrorCode=Code;return;}if(!Receipt.bAccepted||Receipt.RunId!=Capture->Run.RunId||!HorizonMatchStorage::Exact(Receipt.LeaderboardKey,Capture->Run.LeaderboardKey)){LastErrorCode=TEXT("INVALID_RESPONSE");return;}if(!Now||Now->GetHttpClient()->GetRequestContextEpoch()!=Context)return;auto* Saved=HorizonMatchStorage::Load(Slot);if(!Saved||HorizonMatchStorage::Signature(Saved)!=HorizonMatchStorage::Signature(Capture.Get()))return;Saved->bSubmitConfirmed=true;if(!HorizonMatchStorage::Save(Saved,Slot))return;HorizonMatchStorage::Delete(Slot);LastErrorCode.Reset();
    }));
}
