#include "Models/HorizonMatches.h"
#include "Models/HorizonMatchesWire.h"
#include "Dom/JsonValue.h"
namespace HorizonMatchesJson
{
using HorizonMatchesWire::TextValue;
bool Text(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S,int32 Max,bool Nonblank=false){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::String&&V->TryGetString(S)&&TextValue(S,Max,Nonblank);}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Id(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S,36)&&Uuid(S);}
bool Number(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){auto V=J?J->TryGetField(K):nullptr;FString S;if(!V||V->Type!=EJson::Number||!V->TryGetString(S))return false;const FTCHARToUTF8 B(*S,S.Len()+1);long long Value;if(!HorizonItemsContract::Integer(std::string(B.Get(),B.Length()),Value))return false;N=Value;return true;}
bool Rev(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){return Number(J,K,N)&&N>=0;}
bool Small(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int32& N,int32 Max,int32 Min=1){int64 V;if(!Number(J,K,V)||V<Min||V>Max)return false;N=int32(V);return true;}
bool Bool(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& B){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Boolean&&V->TryGetBool(B);}
bool Null(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& Has){auto V=J?J->TryGetField(K):nullptr;if(!V)return false;Has=!V->IsNull();return true;}
bool Time(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){FDateTime T;return Text(J,K,S,40)&&S.Len()==FCString::Strlen(*S)&&FDateTime::ParseIso8601(*S,T);}
bool NullableTime(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& Has,FString& S){return Null(J,K,Has)&&(!Has||Time(J,K,S));}
bool NullableId(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& Has,FString& S){return Null(J,K,Has)&&(!Has||Id(J,K,S));}
bool Envelope(const TSharedPtr<FJsonObject>& J,int32& Poll,FString& Server){return Small(J,TEXT("pollAfterSeconds"),Poll,MAX_int32)&&Time(J,TEXT("serverTime"),Server);}
bool Ids(const TSharedPtr<FJsonObject>& J,const TCHAR* K,TArray<FString>& Out,int32 Maximum,bool Sorted=false)
{
    const TArray<TSharedPtr<FJsonValue>>* Values;if(!J||!J->TryGetArrayField(K,Values)||Values->Num()>Maximum)return false;TSet<FString> Seen;FString Last;
    for(auto V:*Values){FString S;if(!V||V->Type!=EJson::String||!V->TryGetString(S)||!Uuid(S)||Seen.Contains(S.ToLower())||(Sorted&&!Last.IsEmpty()&&Last.Compare(S,ESearchCase::IgnoreCase)>=0))return false;Seen.Add(S.ToLower());Last=S;Out.Add(S);}return true;
}
bool Contains(const TArray<FString>& Values,const FString& S){return Values.ContainsByPredicate([&](const FString& V){return V.Equals(S,ESearchCase::IgnoreCase);});}
bool Subset(const TArray<FString>& Small,const TArray<FString>& Large){for(const auto& V:Small)if(!Contains(Large,V))return false;return true;}
bool Bytes(const TSharedPtr<FJsonObject>& J,const TCHAR* K,TArray<uint8>& Data,int32 Max)
{
    FString S;if(!Text(J,K,S,((Max+2)/3)*4))return false;const FTCHARToUTF8 B(*S,S.Len()+1);std::vector<uint8_t> Out;if(!HorizonSharedContract::DecodeBase64(std::string(B.Get(),B.Length()),Out,Max))return false;if(!Out.empty())Data.Append(Out.data(),int32(Out.size()));return true;
}
bool ReadLimits(const TSharedPtr<FJsonObject>& J,FHorizonMatchLimits& L){const TSharedPtr<FJsonObject>* O;return J&&J->TryGetObjectField(TEXT("limits"),O)&&Small(*O,TEXT("activeMatches"),L.ActiveMatches,MAX_int32)&&Small(*O,TEXT("stateBytes"),L.StateBytes,int32(HorizonMatchesContract::MaxStateBytes))&&Small(*O,TEXT("retainedMatches"),L.RetainedMatches,MAX_int32,0)&&Small(*O,TEXT("pendingInvites"),L.PendingInvites,MAX_int32)&&Small(*O,TEXT("settlementMatches"),L.SettlementMatches,MAX_int32)&&Small(*O,TEXT("historyRetentionSeconds"),L.HistoryRetentionSeconds,MAX_int32,0)&&Small(*O,TEXT("rankedSettlementSeconds"),L.RankedSettlementSeconds,MAX_int32);}
bool Summary(const TSharedPtr<FJsonObject>& J,FHorizonMatchSummary& S)
{
    if(!Id(J,TEXT("id"),S.Id)||!Text(J,TEXT("name"),S.Name,120,true)||!Rev(J,TEXT("revision"),S.Revision)||!S.Revision||!Id(J,TEXT("creatorId"),S.CreatorId)||!Small(J,TEXT("capacity"),S.Capacity,8,2)||!Text(J,TEXT("visibility"),S.Visibility,7)||!Number(J,TEXT("strength"),S.Strength)||!Text(J,TEXT("status"),S.Status,11)||!Ids(J,TEXT("participantIds"),S.ParticipantIds,S.Capacity)||S.ParticipantIds.IsEmpty()||!Contains(S.ParticipantIds,S.CreatorId)||!Ids(J,TEXT("invitedIds"),S.InvitedIds,7)||!Ids(J,TEXT("forfeitedIds"),S.ForfeitedIds,8)||!Subset(S.ForfeitedIds,S.ParticipantIds)||!NullableId(J,TEXT("currentPlayerId"),S.bHasCurrentPlayerId,S.CurrentPlayerId)||!NullableTime(J,TEXT("turnDeadline"),S.bHasTurnDeadline,S.TurnDeadline)||!Time(J,TEXT("createdAt"),S.CreatedAt)||!NullableTime(J,TEXT("startedAt"),S.bHasStartedAt,S.StartedAt)||!NullableTime(J,TEXT("finishedAt"),S.bHasFinishedAt,S.FinishedAt)||!NullableTime(J,TEXT("purgeAt"),S.bHasPurgeAt,S.PurgeAt)||!Null(J,TEXT("finalStateRevision"),S.bHasFinalStateRevision)||(S.bHasFinalStateRevision&&(!Rev(J,TEXT("finalStateRevision"),S.FinalStateRevision)||S.FinalStateRevision==0||S.FinalStateRevision>S.Revision))||!Bool(J,TEXT("hasDisagreement"),S.bHasDisagreement)||!Bool(J,TEXT("ranked"),S.bRanked)||!Bool(J,TEXT("timeoutDue"),S.bTimeoutDue))return false;
    const FTCHARToUTF8 Status(*S.Status,S.Status.Len()+1);if(!HorizonMatchesContract::Status(std::string(Status.Get(),Status.Length()))||(S.Visibility!=TEXT("open")&&S.Visibility!=TEXT("private")))return false;
    for(const auto& V:S.InvitedIds)if(Contains(S.ParticipantIds,V))return false;
    if(S.Status==TEXT("active")){if(!S.bHasCurrentPlayerId||!S.bHasTurnDeadline||!Contains(S.ParticipantIds,S.CurrentPlayerId)||Contains(S.ForfeitedIds,S.CurrentPlayerId))return false;}else if(S.bHasCurrentPlayerId||S.bHasTurnDeadline)return false;
    if((S.Status==TEXT("reporting")||S.Status==TEXT("confirmed")||S.Status==TEXT("disputed"))&&!S.bHasFinalStateRevision)return false;S.bPresent=true;return true;
}
bool View(const TSharedPtr<FJsonObject>& J,FHorizonMatchView& V)
{
    if(!Summary(J,V)||!Ids(J,TEXT("turnOrder"),V.TurnOrder,8)||!Subset(V.TurnOrder,V.ParticipantIds)||!Small(J,TEXT("turnSeconds"),V.TurnSeconds,604800,60)||!Text(J,TEXT("timeoutPolicy"),V.TimeoutPolicy,7)||(V.TimeoutPolicy!=TEXT("skip")&&V.TimeoutPolicy!=TEXT("forfeit"))||!Null(J,TEXT("dataBase64"),V.bHasData)||(V.bHasData&&!Bytes(J,TEXT("dataBase64"),V.Data,int32(HorizonMatchesContract::MaxStateBytes)))||!NullableTime(J,TEXT("reportDeadline"),V.bHasReportDeadline,V.ReportDeadline)||!NullableTime(J,TEXT("absoluteDeadline"),V.bHasAbsoluteDeadline,V.AbsoluteDeadline)||!Null(J,TEXT("rankedLeaderboardKey"),V.bHasRankedLeaderboardKey)||(V.bHasRankedLeaderboardKey&&!Text(J,TEXT("rankedLeaderboardKey"),V.RankedLeaderboardKey,1024,true))||!NullableId(J,TEXT("ownRankedRunId"),V.bHasOwnRankedRunId,V.OwnRankedRunId)||!Bool(J,TEXT("notificationPending"),V.bNotificationPending))return false;
    const TArray<TSharedPtr<FJsonValue>>* Rows;if(!J->TryGetArrayField(TEXT("reports"),Rows)||Rows->Num()>V.ParticipantIds.Num())return false;TSet<FString> Seen;
    for(auto Row:*Rows){if(!Row||Row->Type!=EJson::Object)return false;const auto O=Row->AsObject();FHorizonMatchReport R;if(!Id(O,TEXT("userId"),R.UserId)||!Contains(V.ParticipantIds,R.UserId)||Seen.Contains(R.UserId.ToLower())||!Rev(O,TEXT("finalStateRevision"),R.FinalStateRevision)||!V.bHasFinalStateRevision||R.FinalStateRevision!=V.FinalStateRevision||!Bytes(O,TEXT("resultBase64"),R.Result,256)||!Ids(O,TEXT("winnerIds"),R.WinnerIds,8,true)||!Subset(R.WinnerIds,V.ParticipantIds)||!Time(O,TEXT("reportedAt"),R.ReportedAt))return false;Seen.Add(R.UserId.ToLower());V.Reports.Add(R);}
    if(!V.bHasData&&(!V.Reports.IsEmpty()||V.bHasOwnRankedRunId))return false;return true;
}
bool Rows(const TSharedPtr<FJsonObject>& J,const TCHAR* K,TArray<FHorizonMatchSummary>& Out,int32 Maximum,TSet<FString>& Seen)
{
    const TArray<TSharedPtr<FJsonValue>>* Values;if(!J||!J->TryGetArrayField(K,Values)||Values->Num()>Maximum)return false;
    for(auto V:*Values){FHorizonMatchSummary S;if(!V||V->Type!=EJson::Object||!Summary(V->AsObject(),S)||Seen.Contains(S.Id.ToLower()))return false;Seen.Add(S.Id.ToLower());Out.Add(S);}return true;
}
}
bool FHorizonMyMatches::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMyMatches& Out)
{
    using namespace HorizonMatchesJson;FHorizonMyMatches V;TSet<FString> Seen;if(!Id(J,TEXT("userId"),V.UserId)||!Rev(J,TEXT("revision"),V.Revision)||!ReadLimits(J,V.Limits)||!Envelope(J,V.PollAfterSeconds,V.ServerTime)||!Rows(J,TEXT("active"),V.Active,MAX_int32,Seen)||!Rows(J,TEXT("invites"),V.Invites,MAX_int32,Seen)||!Rows(J,TEXT("recent"),V.Recent,MAX_int32,Seen))return false;
    for(const auto& R:V.Active)if(!Contains(R.ParticipantIds,V.UserId)||(R.Status!=TEXT("lobby")&&R.Status!=TEXT("active")))return false;for(const auto& R:V.Invites)if(!Contains(R.InvitedIds,V.UserId)||R.Status!=TEXT("lobby"))return false;for(const auto& R:V.Recent)if(!Contains(R.ParticipantIds,V.UserId)||R.Status==TEXT("lobby")||(R.Status==TEXT("active")&&!Contains(R.ForfeitedIds,V.UserId)))return false;const TArray<TSharedPtr<FJsonValue>>* Pending;if(!J->TryGetArrayField(TEXT("pendingResults"),Pending))return false;TSet<FString> Sources;
    for(const auto& Row:*Pending){FHorizonMatchPendingResult P;if(!Row||Row->Type!=EJson::Object)return false;const auto O=Row->AsObject();if(!Id(O,TEXT("matchId"),P.MatchId)||Sources.Contains(P.MatchId.ToLower())||!Rev(O,TEXT("finalStateRevision"),P.FinalStateRevision)||!P.FinalStateRevision||!NullableId(O,TEXT("ownRankedRunId"),P.bHasOwnRankedRunId,P.OwnRankedRunId)||!Time(O,TEXT("expiresAt"),P.ExpiresAt))return false;Sources.Add(P.MatchId.ToLower());V.PendingResults.Add(P);}
    V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonMatchLimitsView::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMatchLimitsView& Out)
{
    using namespace HorizonMatchesJson;FHorizonMatchLimitsView V;const TSharedPtr<FJsonObject> *Config,*Capabilities;
    if(!ReadLimits(J,V.Limits)||!J->TryGetObjectField(TEXT("config"),Config)||!Rev(*Config,TEXT("revision"),V.Config.Revision)||!Null(*Config,TEXT("rankedLeaderboardKey"),V.Config.bHasRankedLeaderboardKey)||(V.Config.bHasRankedLeaderboardKey&&!Text(*Config,TEXT("rankedLeaderboardKey"),V.Config.RankedLeaderboardKey,1024,true))||!J->TryGetObjectField(TEXT("capabilities"),Capabilities)||!Bool(*Capabilities,TEXT("matches"),V.Capabilities.bMatches)||!Bool(*Capabilities,TEXT("matchesRanked"),V.Capabilities.bMatchesRanked)||!Bool(*Capabilities,TEXT("realtime"),V.Capabilities.bRealtime)||!Envelope(J,V.PollAfterSeconds,V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonMatchPage::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMatchPage& Out)
{
    using namespace HorizonMatchesJson;FHorizonMatchPage V;TSet<FString> Seen;if(!Rows(J,TEXT("matches"),V.Matches,50,Seen)||!Null(J,TEXT("nextCursor"),V.bHasNextCursor)||(V.bHasNextCursor&&!Text(J,TEXT("nextCursor"),V.NextCursor,256,true))||!Envelope(J,V.PollAfterSeconds,V.ServerTime))return false;if(V.bHasNextCursor){const FTCHARToUTF8 C(*V.NextCursor,V.NextCursor.Len()+1);if(!HorizonMatchesContract::Cursor(std::string(C.Get(),C.Length())))return false;}V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonMatchRead::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMatchRead& Out)
{
    using namespace HorizonMatchesJson;FHorizonMatchRead V;const TSharedPtr<FJsonObject>* M;if(!J||!J->TryGetObjectField(TEXT("match"),M)||!View(*M,V.Match)||!Envelope(J,V.PollAfterSeconds,V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonMatchCreated::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMatchCreated& Out)
{
    using namespace HorizonMatchesJson;FHorizonMatchCreated V;const TSharedPtr<FJsonObject>* M;if(!J||!J->TryGetObjectField(TEXT("match"),M)||!View(*M,V.Match)||!Rev(J,TEXT("playerRevision"),V.PlayerRevision)||!Envelope(J,V.PollAfterSeconds,V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonMatchReported::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMatchReported& Out)
{
    using namespace HorizonMatchesJson;FHorizonMatchReported V;const TSharedPtr<FJsonObject>* M;if(!J||!J->TryGetObjectField(TEXT("match"),M)||!View(*M,V.Match)||!Bool(J,TEXT("replayed"),V.bReplayed)||!Envelope(J,V.PollAfterSeconds,V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
