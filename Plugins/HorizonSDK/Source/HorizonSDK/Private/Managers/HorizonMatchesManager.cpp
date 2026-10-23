// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonMatchesManager.h"
#include "Managers/HorizonRunStartLease.h"
#include "Models/HorizonMatchesWire.h"
#include "Http/HorizonLosslessJson.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Base64.h"
#include "Managers/HorizonRealtimeManager.h"
namespace HorizonMatchesRequest
{
const FString Root=TEXT("/api/v1/app/matches");
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Revision(int64 N){return HorizonMatchesContract::Revision(N);}
bool Exact(const FString& A,const FString& B){return A.Len()==B.Len()&&(!A.Len()||FMemory::Memcmp(*A,*B,A.Len()*sizeof(TCHAR))==0);}
bool Ids(const TArray<FString>& Values,int32 Max,bool Sorted=false){if(Values.Num()>Max)return false;TSet<FString> Seen;FString Last;for(const auto& V:Values){if(!Uuid(V)||Seen.Contains(V.ToLower())||(Sorted&&!Last.IsEmpty()&&Last.Compare(V,ESearchCase::IgnoreCase)>=0))return false;Seen.Add(V.ToLower());Last=V;}return true;}
bool Contains(const TArray<FString>& Values,const FString& Id){return Values.ContainsByPredicate([&](const FString& V){return V.Equals(Id,ESearchCase::IgnoreCase);});}
void IdArray(const TSharedPtr<FJsonObject>& B,const TCHAR* K,const TArray<FString>& Values){TArray<TSharedPtr<FJsonValue>> Rows;for(const auto& V:Values)Rows.Add(MakeShared<FJsonValueString>(V));B->SetArrayField(K,Rows);}
TSharedPtr<FJsonObject> Body(const FString& U){auto B=MakeShared<FJsonObject>();B->SetStringField(TEXT("userId"),U);return B;}
FString UserQuery(const FString& U){return TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U);}
FString Match(const FString& Id){return Root+TEXT("/")+Id;}
bool Visible(const FHorizonMatchView& M,const FString& U){return Contains(M.ParticipantIds,U)==M.bHasData;}
FString MatchKey(const FString& Id){return TEXT("match:")+Id.ToLower();}
TMap<FString,int64> Versions(const TArray<FHorizonMatchSummary>& Rows){TMap<FString,int64> Out;for(const auto& M:Rows)Out.Add(MatchKey(M.Id),M.Revision);return Out;}
TMap<FString,int64> Versions(const FHorizonMyMatches& M){auto Out=Versions(M.Active);Out.Append(Versions(M.Invites));Out.Append(Versions(M.Recent));Out.Add(TEXT("ledger"),M.Revision);return Out;}

template<typename D> bool Failed(const FHorizonNetworkResponse& R,const D& Done){if(R.bSuccess)return false;Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return true;}
template<typename D> void Invalid(const D& Done){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Valid immutable original inputs are required"));}
template<typename D> void Bad(const D& Done){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Malformed or mismatched match snapshot"));}
template<typename D> void Stale(const D& Done){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older authoritative match snapshot"));}
}
void UHorizonMatchesManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A,UHorizonValidatedActionsManager* V){HttpClient=H;AuthManager=A;ValidatedActions=V;ClearCache();}
void UHorizonMatchesManager::ClearSnapshots(){CurrentMatches={};CurrentLimits={};CurrentOpen={};CurrentMatch={};CurrentCreated={};CurrentReported={};}
void UHorizonMatchesManager::Sync(){const FString I=HttpClient&&AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 C=HttpClient?HttpClient->GetRequestContextEpoch():0;if(I!=Identity||C!=Context){if(!AdoptedRunId.IsEmpty()&&ValidatedActions&&ValidatedActions->GetCurrentRun().RunId==AdoptedRunId)ValidatedActions->DiscardRun();AdoptedRunId.Reset();AdoptedMatchId.Reset();AdoptedFinalRevision=-1;AdoptedOriginalRun={};SourceRuns.Empty();Identity=I;Context=C;++Epoch;++Ticket;Busy=false;Scope.Reset();RevisionFloors.Empty();ServerMillis=-1;ClearSnapshots();}}
void UHorizonMatchesManager::ClearCache(){Sync();++Epoch;++Ticket;Busy=false;ClearSnapshots();}
uint64 UHorizonMatchesManager::GetCacheGeneration(){Sync();return Epoch;}
FHorizonNetworkResponse UHorizonMatchesManager::Error(const FString& C){FHorizonNetworkResponse R;R.ServerErrorCode=C;R.ErrorMessage=C;return R;}
bool UHorizonMatchesManager::Accept(const FString& Time,const TMap<FString,int64>& Versions)
{
    FDateTime T;if(!FDateTime::ParseIso8601(*Time,T))return false;
    const int64 Millis=T.ToUnixTimestamp()*1000+T.GetMillisecond();if(Millis<ServerMillis)return false;
    for(const auto& V:Versions){const auto* Known=RevisionFloors.Find(V.Key);if(Known&&V.Value<*Known)return false;}
    ServerMillis=Millis;for(const auto& V:Versions)RevisionFloors.Add(V.Key,V.Value);return true;
}

void UHorizonMatchesManager::Call(const FString& U,const FString& Route,const FString& Verb,const TSharedPtr<FJsonObject>& Body,int64 Revision,FOnHttpResponse Done)
{
    Sync();FString Failure;if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)Failure=TEXT("SESSION_REQUIRED");else if(U!=AuthManager->GetCurrentUser().UserId)Failure=TEXT("SESSION_FORBIDDEN");else if(!HorizonMatchesRequest::Uuid(U))Failure=TEXT("INVALID_REQUEST");else if(Busy)Failure=TEXT("REQUEST_IN_PROGRESS");if(!Failure.IsEmpty()){Done.ExecuteIfBound(Error(Failure));return;}
    FString Json;if(Body&&!HorizonLosslessJson::Object(Body.ToSharedRef(),Json)){Done.ExecuteIfBound(Error(TEXT("INVALID_REQUEST")));return;}
    const FString Input=Verb+TEXT("|")+Route+TEXT("|")+LexToString(Revision)+TEXT("|")+Json;const FTCHARToUTF8 Bytes(*Input,Input.Len()+1);const FString NewScope=UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length()).c_str());if(NewScope!=Scope)Scope=NewScope;++Epoch;ClearSnapshots();Busy=true;const uint64 E=Epoch,T=++Ticket,C=Context;TWeakObjectPtr<UHorizonMatchesManager> Weak(this);
    const auto Complete=FOnHttpResponse::CreateLambda([Weak,E,T,C,NewScope,Done](const FHorizonNetworkResponse& Response)
    {
        auto* S=Weak.Get();if(!S)return;S->Sync();if(S->Epoch!=E||S->Ticket!=T||S->Context!=C||S->Scope!=NewScope){Done.ExecuteIfBound(Error(TEXT("STALE_RESPONSE")));return;}S->Busy=false;
        if(!Response.bSuccess){Done.ExecuteIfBound(Response);return;}FHorizonNetworkResponse R=Response;if(R.RawJsonBody.IsEmpty()||!HorizonMatchesWire::Parse(R.RawJsonBody,R.JsonData)){Done.ExecuteIfBound(Error(TEXT("INVALID_RESPONSE")));return;}Done.ExecuteIfBound(R);
    });
    if(Verb==TEXT("GET"))HttpClient->Get(Route,true,Complete);else if(Revision<0)HttpClient->JsonOnce(Body.ToSharedRef(),Route,false,true,Complete);else HttpClient->RevisionJsonOnce(Body,Route,Verb,Revision,Complete);
}
FHorizonMyMatches UHorizonMatchesManager::GetCurrentMatches(){Sync();return CurrentMatches;}
FHorizonMatchLimitsView UHorizonMatchesManager::GetCurrentLimits(){Sync();return CurrentLimits;}
FHorizonMatchPage UHorizonMatchesManager::GetCurrentOpenMatches(){Sync();return CurrentOpen;}
FHorizonMatchRead UHorizonMatchesManager::GetCurrentMatch(){Sync();return CurrentMatch;}
FHorizonMatchCreated UHorizonMatchesManager::GetCurrentCreated(){Sync();return CurrentCreated;}
FHorizonMatchReported UHorizonMatchesManager::GetCurrentReported(){Sync();return CurrentReported;}
bool UHorizonMatchesManager::SourceMatches(const FString& Id,int64 Final,const FString& Run) const
{
    const auto* Known=SourceRuns.Find(Id.ToLower()+TEXT("|")+LexToString(Final));return !Known||Known->Equals(Run,ESearchCase::IgnoreCase);
}
void UHorizonMatchesManager::RememberSource(const FString& Id,int64 Final,const FString& Run)
{
    const FString Key=Id.ToLower()+TEXT("|")+LexToString(Final);if(SourceRuns.Contains(Key))return;SourceRuns.Add(Key,Run);
}
void UHorizonMatchesManager::GetMatches(const FString& U,FOnMatchesComplete Done)
{
    Call(U,HorizonMatchesRequest::Root+HorizonMatchesRequest::UserQuery(U),TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,U,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMyMatches V;if(!FHorizonMyMatches::TryFromJson(R.JsonData,V)||V.UserId!=U){HorizonMatchesRequest::Bad(Done);return;}for(const auto& P:V.PendingResults)if(P.bHasOwnRankedRunId&&!SourceMatches(P.MatchId,P.FinalStateRevision,P.OwnRankedRunId)){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,HorizonMatchesRequest::Versions(V))){HorizonMatchesRequest::Stale(Done);return;}for(const auto& P:V.PendingResults)if(P.bHasOwnRankedRunId)RememberSource(P.MatchId,P.FinalStateRevision,P.OwnRankedRunId);CurrentMatches=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonMatchesManager::GetLimits(const FString& U,FOnMatchLimitsComplete Done)
{
    Call(U,HorizonMatchesRequest::Root+TEXT("/limits")+HorizonMatchesRequest::UserQuery(U),TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMatchLimitsView V;if(!FHorizonMatchLimitsView::TryFromJson(R.JsonData,V)){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,{{TEXT("config"),V.Config.Revision}})){HorizonMatchesRequest::Stale(Done);return;}CurrentLimits=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonMatchesManager::FindOpenMatches(const FString& U,int64 Min,int64 Max,FOnOpenMatchesComplete Done,int32 Limit,const FString& Cursor)
{
    const FTCHARToUTF8 C(*Cursor,Cursor.Len()+1);if(!HorizonMatchesContract::Safe(Min)||!HorizonMatchesContract::Safe(Max)||Min>Max||Limit<1||Limit>50||(!Cursor.IsEmpty()&&!HorizonMatchesContract::Cursor(std::string(C.Get(),C.Length())))){HorizonMatchesRequest::Invalid(Done);return;}
    FString Route=HorizonMatchesRequest::Root+TEXT("/open")+HorizonMatchesRequest::UserQuery(U)+TEXT("&minStrength=")+LexToString(Min)+TEXT("&maxStrength=")+LexToString(Max)+TEXT("&limit=")+LexToString(Limit);if(!Cursor.IsEmpty())Route+=TEXT("&cursor=")+FGenericPlatformHttp::UrlEncode(Cursor);
    Call(U,Route,TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,U,Min,Max,Limit,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMatchPage V;if(!FHorizonMatchPage::TryFromJson(R.JsonData,V)||V.Matches.Num()>Limit){HorizonMatchesRequest::Bad(Done);return;}for(const auto& M:V.Matches)if(M.Status!=TEXT("lobby")||M.Visibility!=TEXT("open")||M.Strength<Min||M.Strength>Max||M.ParticipantIds.Num()>=M.Capacity||HorizonMatchesRequest::Contains(M.ParticipantIds,U)){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,HorizonMatchesRequest::Versions(V.Matches))){HorizonMatchesRequest::Stale(Done);return;}CurrentOpen=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonMatchesManager::GetMatch(const FString& U,const FString& Id,FOnMatchComplete Done)
{
    if(!HorizonMatchesRequest::Uuid(Id)){HorizonMatchesRequest::Invalid(Done);return;}
    Call(U,HorizonMatchesRequest::Match(Id)+HorizonMatchesRequest::UserQuery(U),TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,U,Id,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMatchRead V;if(!FHorizonMatchRead::TryFromJson(R.JsonData,V)||!V.Match.Id.Equals(Id,ESearchCase::IgnoreCase)||!HorizonMatchesRequest::Visible(V.Match,U)||(!HorizonMatchesRequest::Contains(V.Match.ParticipantIds,U)&&!HorizonMatchesRequest::Contains(V.Match.InvitedIds,U))){HorizonMatchesRequest::Bad(Done);return;}if(V.Match.bHasOwnRankedRunId&&(!V.Match.bHasFinalStateRevision||!SourceMatches(Id,V.Match.FinalStateRevision,V.Match.OwnRankedRunId))){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,{{HorizonMatchesRequest::MatchKey(V.Match.Id),V.Match.Revision}})){HorizonMatchesRequest::Stale(Done);return;}if(V.Match.bHasOwnRankedRunId)RememberSource(Id,V.Match.FinalStateRevision,V.Match.OwnRankedRunId);CurrentMatch=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonMatchesManager::CreateMatch(const FString& U,int64 Revision,const FHorizonMatchCreateInput& I,FOnMatchCreatedComplete Done)
{
    if(!HorizonMatchesRequest::Revision(Revision)||!HorizonMatchesWire::TextValue(I.Name,120,true)||I.Capacity<2||I.Capacity>8||(I.Visibility!=TEXT("open")&&I.Visibility!=TEXT("private"))||!HorizonMatchesContract::Safe(I.Strength)||I.TurnSeconds<60||I.TurnSeconds>604800||(I.TimeoutPolicy!=TEXT("skip")&&I.TimeoutPolicy!=TEXT("forfeit"))||I.Data.Num()>int32(HorizonMatchesContract::MaxStateBytes)||!HorizonMatchesRequest::Ids(I.InviteeIds,7)||I.InviteeIds.Num()>=I.Capacity||HorizonMatchesRequest::Contains(I.InviteeIds,U)){HorizonMatchesRequest::Invalid(Done);return;}
    auto B=HorizonMatchesRequest::Body(U);B->SetStringField(TEXT("name"),I.Name);B->SetNumberField(TEXT("capacity"),I.Capacity);B->SetStringField(TEXT("visibility"),I.Visibility);B->SetField(TEXT("strength"),MakeShared<FJsonValueNumberString>(LexToString(I.Strength)));B->SetNumberField(TEXT("turnSeconds"),I.TurnSeconds);B->SetStringField(TEXT("timeoutPolicy"),I.TimeoutPolicy);B->SetBoolField(TEXT("ranked"),I.bRanked);B->SetStringField(TEXT("dataBase64"),FBase64::Encode(I.Data));HorizonMatchesRequest::IdArray(B,TEXT("inviteeIds"),I.InviteeIds);
    Call(U,HorizonMatchesRequest::Root,TEXT("POST"),B,Revision,FOnHttpResponse::CreateWeakLambda(this,[this,U,Revision,I,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMatchCreated V;if(!FHorizonMatchCreated::TryFromJson(R.JsonData,V)||V.PlayerRevision!=Revision+1||V.Match.Revision!=1||!V.Match.CreatorId.Equals(U,ESearchCase::IgnoreCase)||!V.Match.bHasData||V.Match.Data!=I.Data||!HorizonMatchesRequest::Exact(V.Match.Name,I.Name)||V.Match.Capacity!=I.Capacity||V.Match.Visibility!=I.Visibility||V.Match.Strength!=I.Strength||V.Match.TurnSeconds!=I.TurnSeconds||V.Match.TimeoutPolicy!=I.TimeoutPolicy||V.Match.bRanked!=I.bRanked){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,{{TEXT("ledger"),V.PlayerRevision},{HorizonMatchesRequest::MatchKey(V.Match.Id),V.Match.Revision}})){HorizonMatchesRequest::Stale(Done);return;}CurrentCreated=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonMatchesManager::Action(const FString& U,const FString& Id,int64 Revision,const FString& ActionName,const TSharedPtr<FJsonObject>& B,FOnMatchComplete Done)
{
    if(!HorizonMatchesRequest::Uuid(Id)||!HorizonMatchesRequest::Revision(Revision)){HorizonMatchesRequest::Invalid(Done);return;}
    Call(U,HorizonMatchesRequest::Match(Id)+TEXT("/")+ActionName,TEXT("POST"),B,Revision,FOnHttpResponse::CreateWeakLambda(this,[this,U,Id,Revision,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMatchRead V;if(!FHorizonMatchRead::TryFromJson(R.JsonData,V)||!V.Match.Id.Equals(Id,ESearchCase::IgnoreCase)||V.Match.Revision!=Revision+1||!HorizonMatchesRequest::Visible(V.Match,U)){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,{{HorizonMatchesRequest::MatchKey(V.Match.Id),V.Match.Revision}})){HorizonMatchesRequest::Stale(Done);return;}CurrentMatch=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonMatchesManager::JoinMatch(const FString& U,const FString& Id,int64 R,FOnMatchComplete D){Action(U,Id,R,TEXT("join"),HorizonMatchesRequest::Body(U),D);}
void UHorizonMatchesManager::DeclineInvite(const FString& U,const FString& Id,int64 R,FOnMatchComplete D){Action(U,Id,R,TEXT("decline"),HorizonMatchesRequest::Body(U),D);}
void UHorizonMatchesManager::AdvanceTimeout(const FString& U,const FString& Id,int64 R,FOnMatchComplete D){Action(U,Id,R,TEXT("timeout"),HorizonMatchesRequest::Body(U),D);}
void UHorizonMatchesManager::LeaveMatch(const FString& U,const FString& Id,int64 R,FOnMatchComplete D){Action(U,Id,R,TEXT("leave"),HorizonMatchesRequest::Body(U),D);}
void UHorizonMatchesManager::CancelMatch(const FString& U,const FString& Id,int64 R,FOnMatchComplete D){Action(U,Id,R,TEXT("cancel"),HorizonMatchesRequest::Body(U),D);}
void UHorizonMatchesManager::StartMatch(const FString& U,const FString& Id,int64 R,const TArray<FString>& Order,FOnMatchComplete D){if(Order.Num()<2||!HorizonMatchesRequest::Ids(Order,8)){HorizonMatchesRequest::Invalid(D);return;}auto B=HorizonMatchesRequest::Body(U);HorizonMatchesRequest::IdArray(B,TEXT("turnOrder"),Order);Action(U,Id,R,TEXT("start"),B,D);}
void UHorizonMatchesManager::SubmitTurn(const FString& U,const FString& Id,int64 R,const TArray<uint8>& Data,bool Finish,FOnMatchComplete D){if(Data.Num()>int32(HorizonMatchesContract::MaxStateBytes)){HorizonMatchesRequest::Invalid(D);return;}auto B=HorizonMatchesRequest::Body(U);B->SetStringField(TEXT("dataBase64"),FBase64::Encode(Data));B->SetBoolField(TEXT("finish"),Finish);Action(U,Id,R,TEXT("turn"),B,D);}
void UHorizonMatchesManager::ReportResult(const FString& U,const FString& Id,int64 Final,const TArray<uint8>& Result,const TArray<FString>& Winners,FOnMatchReportedComplete Done)
{
    if(!HorizonMatchesRequest::Uuid(Id)||!HorizonMatchesRequest::Revision(Final)||Final==0||Result.Num()>256||!HorizonMatchesRequest::Ids(Winners,8,true)){HorizonMatchesRequest::Invalid(Done);return;}auto B=HorizonMatchesRequest::Body(U);B->SetField(TEXT("finalStateRevision"),MakeShared<FJsonValueNumberString>(LexToString(Final)));B->SetStringField(TEXT("resultBase64"),FBase64::Encode(Result));HorizonMatchesRequest::IdArray(B,TEXT("winnerIds"),Winners);
    Call(U,HorizonMatchesRequest::Match(Id)+TEXT("/result"),TEXT("POST"),B,-1,FOnHttpResponse::CreateWeakLambda(this,[this,U,Id,Final,Result,Winners,Done](const FHorizonNetworkResponse& R){if(HorizonMatchesRequest::Failed(R,Done))return;FHorizonMatchReported V;if(!FHorizonMatchReported::TryFromJson(R.JsonData,V)||!V.Match.Id.Equals(Id,ESearchCase::IgnoreCase)||!V.Match.bHasData||!V.Match.bHasFinalStateRevision||V.Match.FinalStateRevision!=Final){HorizonMatchesRequest::Bad(Done);return;}const auto* Own=V.Match.Reports.FindByPredicate([&](const FHorizonMatchReport& X){return X.UserId.Equals(U,ESearchCase::IgnoreCase);});if(!Own||Own->Result!=Result||Own->WinnerIds.Num()!=Winners.Num()){HorizonMatchesRequest::Bad(Done);return;}for(int32 N=0;N<Winners.Num();++N)if(!Own->WinnerIds[N].Equals(Winners[N],ESearchCase::IgnoreCase)){HorizonMatchesRequest::Bad(Done);return;}if(!Accept(V.ServerTime,{{HorizonMatchesRequest::MatchKey(V.Match.Id),V.Match.Revision}})){HorizonMatchesRequest::Stale(Done);return;}CurrentReported=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
namespace HorizonMatchesRequest
{
bool ReadRun(const TSharedPtr<FJsonObject>& J,FHorizonValidatedRun& R)
{
    auto Text=[&](const TCHAR* K,FString& S,int32 Max,bool Empty=false){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::String&&V->TryGetString(S)&&HorizonMatchesWire::TextValue(S,Max,!Empty);};
    auto Number=[&](const TCHAR* K,int64& Out){auto V=J?J->TryGetField(K):nullptr;FString S;if(!V||V->Type!=EJson::Number||!V->TryGetString(S))return false;const FTCHARToUTF8 B(*S,S.Len()+1);long long N;if(!HorizonItemsContract::Integer(std::string(B.Get(),B.Length()),N))return false;Out=N;return true;};
    int64 Seed,Seconds;FDateTime Issued,Expires;
    if(!Text(TEXT("runId"),R.RunId,36)||!Uuid(R.RunId)||!Text(TEXT("ticket"),R.Ticket,32768)||!Number(TEXT("seed"),Seed)||Seed<0||Seed>2147483646||!Number(TEXT("expiresInSeconds"),Seconds)||Seconds<1||Seconds>MAX_int32||!Text(TEXT("issuedAt"),R.IssuedAt,40)||R.IssuedAt.Len()!=FCString::Strlen(*R.IssuedAt)||!FDateTime::ParseIso8601(*R.IssuedAt,Issued)||!Text(TEXT("expiresAt"),R.ExpiresAt,40)||R.ExpiresAt.Len()!=FCString::Strlen(*R.ExpiresAt)||!FDateTime::ParseIso8601(*R.ExpiresAt,Expires)||Expires<=Issued)return false;
    for(TCHAR C:R.Ticket)if(C<33||C>126)return false;
    auto Board=J->TryGetField(TEXT("leaderboardKey"));if(!Board||(!Board->IsNull()&&!Text(TEXT("leaderboardKey"),R.LeaderboardKey,1024)))return false;R.Seed=int32(Seed);R.ExpiresInSeconds=int32(Seconds);return true;
}
}
void UHorizonMatchesManager::StartRankedRun(const FString& U,const FString& Id,int64 Final,FOnValidatedRunStarted Done)
{
    if(!HorizonMatchesRequest::Uuid(Id)||!HorizonMatchesRequest::Revision(Final)||Final==0){HorizonMatchesRequest::Invalid(Done);return;}Sync();FString ExpectedRun=AdoptedMatchId.Equals(Id,ESearchCase::IgnoreCase)&&AdoptedFinalRevision==Final?AdoptedRunId:SourceRuns.FindRef(Id.ToLower()+TEXT("|")+LexToString(Final));
    if(ExpectedRun.IsEmpty()&&CurrentMatch.bPresent&&CurrentMatch.Match.Id.Equals(Id,ESearchCase::IgnoreCase)&&CurrentMatch.Match.bHasFinalStateRevision&&CurrentMatch.Match.FinalStateRevision==Final&&CurrentMatch.Match.bHasOwnRankedRunId)ExpectedRun=CurrentMatch.Match.OwnRankedRunId;
    if(ValidatedActions&&ValidatedActions->HasActiveRun()&&(ExpectedRun.IsEmpty()||ValidatedActions->GetCurrentRun().RunId!=ExpectedRun)){Done.ExecuteIfBound(false,{},TEXT("RUN_IN_PROGRESS"),TEXT("An unrelated original run is active"));return;}
    const auto Lease=FHorizonRunStartLease::Acquire(ValidatedActions);if(!Lease){Done.ExecuteIfBound(false,{},TEXT("RUN_IN_PROGRESS"),TEXT("Another run start is in flight"));return;}
    const uint64 RequestContext=Context;
    auto B=HorizonMatchesRequest::Body(U);B->SetField(TEXT("finalStateRevision"),MakeShared<FJsonValueNumberString>(LexToString(Final)));
    Call(U,HorizonMatchesRequest::Match(Id)+TEXT("/ranked-run"),TEXT("POST"),B,-1,FOnHttpResponse::CreateWeakLambda(this,[this,U,Id,Final,Done,Lease,RequestContext,ExpectedRun](const FHorizonNetworkResponse& R)
    {
        struct FRelease { TSharedPtr<FHorizonRunStartLease> Lease; ~FRelease(){Lease->Release();} } Release{Lease};
        if(!R.bSuccess){const FString Code=R.GetErrorCodeString();if((Code==TEXT("RUN_ALREADY_USED")||Code==TEXT("RUN_EXPIRED"))&&AdoptedMatchId.Equals(Id,ESearchCase::IgnoreCase)&&AdoptedFinalRevision==Final&&ValidatedActions&&ValidatedActions->GetCurrentRun().RunId==AdoptedRunId)ValidatedActions->DiscardRun();Done.ExecuteIfBound(false,{},Code,R.ErrorMessage);return;}
        FHorizonValidatedRun Run;if(!HorizonMatchesRequest::ReadRun(R.JsonData,Run)){HorizonMatchesRequest::Bad(Done);return;}
        if(AdoptedMatchId.Equals(Id,ESearchCase::IgnoreCase)&&AdoptedFinalRevision==Final&&AdoptedOriginalRun.IsValid()&&(Run.RunId!=AdoptedOriginalRun.RunId||Run.Seed!=AdoptedOriginalRun.Seed||Run.IssuedAt!=AdoptedOriginalRun.IssuedAt||Run.ExpiresAt!=AdoptedOriginalRun.ExpiresAt||!HorizonMatchesRequest::Exact(Run.LeaderboardKey,AdoptedOriginalRun.LeaderboardKey))){HorizonMatchesRequest::Bad(Done);return;}
        if(!ExpectedRun.IsEmpty()&&!Run.RunId.Equals(ExpectedRun,ESearchCase::IgnoreCase)){HorizonMatchesRequest::Bad(Done);return;}if(!ValidatedActions||HttpClient->GetRequestContextEpoch()!=RequestContext||!ValidatedActions->AdoptMatchRun(U,Run,RequestContext,Lease->Id)){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Original run context changed"));return;}
        AdoptedOriginalRun=Run;AdoptedRunId=Run.RunId;AdoptedMatchId=Id;AdoptedFinalRevision=Final;Done.ExecuteIfBound(true,Run,TEXT(""),TEXT(""));
    }));
}

void UHorizonMatchesManager::SetRealtime(UHorizonRealtimeManager* R)
{
    if(Realtime){Realtime->OnEvent.RemoveDynamic(this,&UHorizonMatchesManager::OnRealtimeEvent);Realtime->OnResyncRequired.RemoveDynamic(this,&UHorizonMatchesManager::OnRealtimeTopic);}Realtime=R;
    if(R){R->OnEvent.AddUniqueDynamic(this,&UHorizonMatchesManager::OnRealtimeEvent);R->OnResyncRequired.AddUniqueDynamic(this,&UHorizonMatchesManager::OnRealtimeTopic);}
}
void UHorizonMatchesManager::OnRealtimeTopic(const FString& Topic)
{
    if(Topic.StartsWith(TEXT("matches:"),ESearchCase::CaseSensitive)&&HorizonMatchesRequest::Uuid(Topic.Mid(8)))ClearCache();
}
void UHorizonMatchesManager::OnRealtimeEvent(const FString& EventJson)
{
    TSharedPtr<FJsonObject> J;FString Type,Topic,Version;if(!HorizonMatchesWire::Parse(EventJson,J)||!J->TryGetStringField(TEXT("type"),Type)||Type!=TEXT("event")||!J->TryGetStringField(TEXT("topic"),Topic))return;
    const auto V=J->TryGetField(TEXT("version"));if(V&&V->Type==EJson::Number&&V->TryGetString(Version)&&Version==TEXT("1"))OnRealtimeTopic(Topic);
}
