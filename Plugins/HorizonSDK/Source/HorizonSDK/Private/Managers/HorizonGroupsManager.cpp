// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonGroupsManager.h"
#include "Models/HorizonGroupsWire.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Serialization/JsonSerializer.h"
namespace HorizonGroupsRequest
{
bool Uuid(const FString& S) { FGuid G;return FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G); }
bool Revision(int64 R) {return R>=0 && R<=9007199254740991LL;}
int64 Millis(const FString& S) {FDateTime T;return FDateTime::ParseIso8601(*S,T)?T.ToUnixTimestamp()*1000+T.GetMillisecond():-1;}
FString Base(const FString& G) {return TEXT("/api/v1/app/groups")+(G.IsEmpty()?FString():TEXT("/")+G);}
FString Read(const FString& U,const FString& G,const FString& Suffix=TEXT("")) {return Base(G)+Suffix+TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U);}
TSharedRef<FJsonObject> Body(const FString& User,int64 Expected,const TCHAR* Key=TEXT("expectedRevision")) {auto B=MakeShared<FJsonObject>();B->SetStringField(TEXT("userId"),User);B->SetField(Key,MakeShared<FJsonValueNumberString>(LexToString(Expected)));return B;}
}
void UHorizonGroupsManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A) {HttpClient=H;AuthManager=A;ClearCache();}
void UHorizonGroupsManager::Sync()
{
    const FString Next=HttpClient && AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 E=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if(Next!=Identity || E!=Context) {Identity=Next;Context=E;++Epoch;++Ticket;bBusy=false;PlayerRevision=ServerMillis=-1;GroupRevisions.Empty();Current={};}
}
void UHorizonGroupsManager::ClearCache() {Sync();++Epoch;++Ticket;bBusy=false;Current={};}
uint64 UHorizonGroupsManager::GetCacheGeneration() {Sync();return Epoch;}
FHorizonMyGroupsView UHorizonGroupsManager::GetCurrentGroups() {Sync();return Current;}
bool UHorizonGroupsManager::ParseGroupData(const FString& Json,FHorizonGroupJsonData& Data) {return FHorizonGroupJsonData::TryFromJson(Json,Data);}
FString UHorizonGroupsManager::GroupDataJson(const FHorizonGroupJsonData& Data) {return Data.GetJson();}
FHorizonNetworkResponse UHorizonGroupsManager::Error(const FString& Code) {FHorizonNetworkResponse R;R.ServerErrorCode=Code;R.ErrorMessage=Code;return R;}
void UHorizonGroupsManager::Call(const FString& User,const FString& Route,const TSharedPtr<FJsonObject>& B,FOnHttpResponse Complete,bool Put)
{
    Sync();FString Failure;
    if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)Failure=TEXT("SESSION_REQUIRED");
    else if(User!=AuthManager->GetCurrentUser().UserId)Failure=TEXT("SESSION_FORBIDDEN");
    else if(!HorizonGroupsRequest::Uuid(User))Failure=TEXT("INVALID_REQUEST");else if(bBusy)Failure=TEXT("REQUEST_IN_PROGRESS");
    if(!Failure.IsEmpty()) {Complete.ExecuteIfBound(Error(Failure));return;}
    bBusy=true;if(B){++Epoch;Current={};}const uint64 T=++Ticket,E=Epoch,C=Context;const FString I=Identity;TWeakObjectPtr<UHorizonGroupsManager> Weak(this);
    auto Done=FOnHttpResponse::CreateLambda([Weak,T,E,C,I,Complete](const FHorizonNetworkResponse& R){auto* Self=Weak.Get();if(!Self)return;Self->Sync();if(Self->Ticket!=T||Self->Epoch!=E||Self->Context!=C||Self->Identity!=I){Complete.ExecuteIfBound(Error(TEXT("STALE_RESPONSE")));return;}Self->bBusy=false;if(R.StatusCode==401||R.StatusCode==403)Self->ClearCache();FHorizonNetworkResponse Parsed=R;
        if(R.bSuccess && (R.RawJsonBody.IsEmpty()||!HorizonGroupsWire::Parse(R.RawJsonBody,Parsed.JsonData))){Complete.ExecuteIfBound(Error(TEXT("INVALID_RESPONSE")));return;}
        Complete.ExecuteIfBound(Parsed);});
    // These revision-only writes have no replay receipt. Never retry them implicitly.
    if(B)HttpClient->JsonOnce(B.ToSharedRef(),Route,Put,true,Done);else HttpClient->Get(Route,true,Done);
}
bool UHorizonGroupsManager::CheckRevision(const FString& G,int64 R) const {const auto* Old=GroupRevisions.Find(G.ToLower());return !Old||R>=*Old;}
bool UHorizonGroupsManager::CheckGroup(const FHorizonGroupView& G) const {return G.bPresent && CheckRevision(G.Id,G.Revision);}
bool UHorizonGroupsManager::CheckSnapshot(const FString& U,const FHorizonMyGroupsView& S) const
{
    if(!S.bPresent||S.UserId!=U||S.Revision<PlayerRevision||HorizonGroupsRequest::Millis(S.ServerTime)<ServerMillis)return false;for(const auto& G:S.Groups)if(!CheckGroup(G))return false;return true;
}
void UHorizonGroupsManager::RecordRevision(const FString& G,int64 R)
{
    if(Current.Groups.ContainsByPredicate([&](const FHorizonGroupView& Item){return Item.Id.Equals(G,ESearchCase::IgnoreCase)&&Item.Revision<R;}))ClearCache();
    const auto* Old=GroupRevisions.Find(G.ToLower());GroupRevisions.Add(G.ToLower(),Old?FMath::Max(*Old,R):R);
}
void UHorizonGroupsManager::Remember(const FHorizonGroupView& G) {RecordRevision(G.Id,G.Revision);}
void UHorizonGroupsManager::Adopt(const FHorizonMyGroupsView& S) {Current=S;PlayerRevision=S.Revision;ServerMillis=HorizonGroupsRequest::Millis(S.ServerTime);for(const auto& G:S.Groups)Remember(G);}
void UHorizonGroupsManager::GetGroups(const FString& U,FOnGroupsComplete Done)
{
    Call(U,HorizonGroupsRequest::Read(U,TEXT("")),nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,U,Done](const FHorizonNetworkResponse& R){if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonMyGroupsView S;if(!FHorizonMyGroupsView::TryFromJson(R.JsonData,S)){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid groups snapshot"));return;}if(!CheckSnapshot(U,S)){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older or foreign groups snapshot"));return;}Adopt(S);Done.ExecuteIfBound(true,S,TEXT(""),TEXT(""));}));
}
void UHorizonGroupsManager::GetGroup(const FString& U,const FString& G,FOnGroupComplete Done)
{
    if(!HorizonGroupsRequest::Uuid(G)){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Group UUID required"));return;}
    Call(U,HorizonGroupsRequest::Read(U,G),nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,G,Done](const FHorizonNetworkResponse& R){if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonGroupView S;if(!FHorizonGroupView::TryFromJson(R.JsonData,S)||!S.Id.Equals(G,ESearchCase::IgnoreCase)){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid group"));return;}if(!CheckGroup(S)){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older group"));return;}ClearCache();Remember(S);Done.ExecuteIfBound(true,S,TEXT(""),TEXT(""));}));
}
void UHorizonGroupsManager::GetMembers(const FString& U,const FString& G,FOnGroupMembersComplete Done,const FString& After,int32 Limit)
{
    if(!HorizonGroupsRequest::Uuid(G)||(!After.IsEmpty()&&!HorizonGroupsRequest::Uuid(After))||Limit<1||Limit==MAX_int32){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Explicit bounded member page required"));return;}
    FString Route=HorizonGroupsRequest::Read(U,G,TEXT("/members"))+TEXT("&limit=")+LexToString(Limit);if(!After.IsEmpty())Route+=TEXT("&afterUserId=")+FGenericPlatformHttp::UrlEncode(After);
    Call(U,Route,nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,G,After,Limit,Done](const FHorizonNetworkResponse& R){if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonGroupMembersView S;if(!FHorizonGroupMembersView::TryFromJson(R.JsonData,S)||!S.GroupId.Equals(G,ESearchCase::IgnoreCase)||S.Members.Num()>Limit){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid members page"));return;}FString Last=After;for(const auto& M:S.Members){if(!Last.IsEmpty()&&M.UserId.Compare(Last,ESearchCase::IgnoreCase)<=0){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Nonadvancing member cursor"));return;}Last=M.UserId;}if(!CheckRevision(G,S.Revision)||HorizonGroupsRequest::Millis(S.ServerTime)<ServerMillis){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older members page"));return;}RecordRevision(G,S.Revision);ServerMillis=HorizonGroupsRequest::Millis(S.ServerTime);Done.ExecuteIfBound(true,S,TEXT(""),TEXT(""));}));
}
void UHorizonGroupsManager::GetPending(const FString& U,const FString& G,FOnGroupPendingComplete Done)
{
    if(!HorizonGroupsRequest::Uuid(G)){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Group UUID required"));return;}
    Call(U,HorizonGroupsRequest::Read(U,G,TEXT("/pending")),nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,G,Done](const FHorizonNetworkResponse& R){if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonGroupPendingSnapshot S;if(!FHorizonGroupPendingSnapshot::TryFromJson(R.JsonData,S)||!S.GroupId.Equals(G,ESearchCase::IgnoreCase)){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid pending snapshot"));return;}if(!CheckRevision(G,S.Revision)||HorizonGroupsRequest::Millis(S.ServerTime)<ServerMillis){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older pending snapshot"));return;}RecordRevision(G,S.Revision);ServerMillis=HorizonGroupsRequest::Millis(S.ServerTime);Done.ExecuteIfBound(true,S,TEXT(""),TEXT(""));}));
}
void UHorizonGroupsManager::ReceiveMutation(const FString& U,const FString& G,const FHorizonNetworkResponse& R,FOnGroupMutationComplete Done)
{
    if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonGroupMutationView S;if(!FHorizonGroupMutationView::TryFromJson(R.JsonData,S)||(S.bHasGroup&&!G.IsEmpty()&&!S.Group.Id.Equals(G,ESearchCase::IgnoreCase))){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid mutation snapshot"));return;}
    if(S.bHasGroup){const auto* Own=S.Groups.Groups.FindByPredicate([&](const FHorizonGroupView& Item){return Item.Id.Equals(S.Group.Id,ESearchCase::IgnoreCase);});if((Own && (Own->Revision!=S.Group.Revision||Own->MyRole!=S.Group.MyRole||!S.Group.bHasData))||(!Own && S.Group.bHasData)){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Mixed membership/group snapshot"));return;}}
    if(!CheckSnapshot(U,S.Groups)||(S.bHasGroup&&!CheckGroup(S.Group))){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older mutation snapshot"));return;}Adopt(S.Groups);if(S.bHasGroup)Remember(S.Group);Done.ExecuteIfBound(true,S,TEXT(""),TEXT(""));
}
void UHorizonGroupsManager::Metadata(const FString& U,const FString& G,int64 Expected,const FHorizonGroupMetadata& M,bool Create,FOnGroupMutationComplete Done)
{
    FHorizonGroupJsonData Validated;
    if(!HorizonGroupsRequest::Revision(Expected)||(!Create&&!HorizonGroupsRequest::Uuid(G))||M.Name.TrimStartAndEnd().IsEmpty()||(M.JoinMode!=TEXT("open")&&M.JoinMode!=TEXT("request")&&M.JoinMode!=TEXT("invite"))||!FHorizonGroupJsonData::TryFromJson(M.Data.GetJson(),Validated)) {Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Explicit revision and valid group metadata required"));return;}
    auto B=HorizonGroupsRequest::Body(U,Expected,Create?TEXT("expectedPlayerRevision"):TEXT("expectedRevision"));B->SetStringField(TEXT("name"),M.Name);B->SetStringField(TEXT("description"),M.Description);B->SetStringField(TEXT("joinMode"),M.JoinMode);B->SetField(TEXT("data"),MakeShared<FHorizonGroupRawJsonValue>(Validated.GetJson()));
    Call(U,HorizonGroupsRequest::Base(G),B,FOnHttpResponse::CreateWeakLambda(this,[this,U,G,Done](const FHorizonNetworkResponse& R){ReceiveMutation(U,G,R,Done);}),!Create);
}
void UHorizonGroupsManager::CreateGroup(const FString& U,int64 Expected,const FHorizonGroupMetadata& M,FOnGroupMutationComplete Done) {Metadata(U,TEXT(""),Expected,M,true,Done);}
void UHorizonGroupsManager::UpdateGroup(const FString& U,const FString& G,int64 Expected,const FHorizonGroupMetadata& M,FOnGroupMutationComplete Done) {Metadata(U,G,Expected,M,false,Done);}
void UHorizonGroupsManager::Action(const FString& U,const FString& G,const FString& Target,int64 Expected,const FString& Request,const FString& Role,const FString& A,FOnGroupMutationComplete Done)
{
    const bool Targeted=A==TEXT("invite")||A==TEXT("cancel-invite")||A==TEXT("accept-request")||A==TEXT("decline-request")||A==TEXT("remove")||A==TEXT("role")||A==TEXT("transfer");const bool Pending=A==TEXT("accept-invite")||A==TEXT("decline-invite")||A==TEXT("cancel-invite")||A==TEXT("accept-request")||A==TEXT("decline-request")||A==TEXT("cancel-request");
    if(!HorizonGroupsRequest::Uuid(G)||!HorizonGroupsRequest::Revision(Expected)||(Targeted?(!HorizonGroupsRequest::Uuid(Target)||Target.Equals(U,ESearchCase::IgnoreCase)):!Target.IsEmpty())||(Pending?!HorizonGroupsRequest::Uuid(Request):!Request.IsEmpty())||(A==TEXT("role")?(Role!=TEXT("officer")&&Role!=TEXT("member")):!Role.IsEmpty())){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Preserve original group, revision, target and request UUID"));return;}
    auto B=HorizonGroupsRequest::Body(U,Expected);if(Targeted)B->SetStringField(TEXT("targetUserId"),Target);if(Pending)B->SetStringField(TEXT("requestId"),Request);if(A==TEXT("role"))B->SetStringField(TEXT("role"),Role);
    Call(U,HorizonGroupsRequest::Base(G)+TEXT("/")+A,B,FOnHttpResponse::CreateWeakLambda(this,[this,U,G,Done](const FHorizonNetworkResponse& R){ReceiveMutation(U,G,R,Done);}));
}
FOnGroupMutationComplete UHorizonGroupsManager::MutationEvent() {return FOnGroupMutationComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGroupMutationView& R,const FString& C,const FString& M){OnGroupMutationResult.Broadcast(Ok,R,C,M);});}
void UHorizonGroupsManager::RequestGroups(const FString& U) {GetGroups(U,FOnGroupsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonMyGroupsView& R,const FString& C,const FString& M){OnGroupsResult.Broadcast(Ok,R,C,M);}));}
void UHorizonGroupsManager::RequestGroup(const FString& U,const FString& G) {GetGroup(U,G,FOnGroupComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGroupView& R,const FString& C,const FString& M){OnGroupResult.Broadcast(Ok,R,C,M);}));}
void UHorizonGroupsManager::RequestGroupMembers(const FString& U,const FString& G,const FString& A,int32 L) {GetMembers(U,G,FOnGroupMembersComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGroupMembersView& R,const FString& C,const FString& M){OnGroupMembersResult.Broadcast(Ok,R,C,M);}),A,L);}
void UHorizonGroupsManager::RequestGroupPending(const FString& U,const FString& G) {GetPending(U,G,FOnGroupPendingComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGroupPendingSnapshot& R,const FString& C,const FString& M){OnGroupPendingResult.Broadcast(Ok,R,C,M);}));}
void UHorizonGroupsManager::CreatePlayerGroup(const FString& U,int64 E,const FHorizonGroupMetadata& M) {CreateGroup(U,E,M,MutationEvent());}
void UHorizonGroupsManager::UpdatePlayerGroup(const FString& U,const FString& G,int64 E,const FHorizonGroupMetadata& M) {UpdateGroup(U,G,E,M,MutationEvent());}
void UHorizonGroupsManager::JoinGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,TEXT(""),TEXT(""),TEXT("join"),Complete); }
void UHorizonGroupsManager::RequestJoin(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,TEXT(""),TEXT(""),TEXT("request"),Complete); }
void UHorizonGroupsManager::InvitePlayer(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,TEXT(""),TEXT(""),TEXT("invite"),Complete); }
void UHorizonGroupsManager::AcceptInvite(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,OriginalRequestId,TEXT(""),TEXT("accept-invite"),Complete); }
void UHorizonGroupsManager::DeclineInvite(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,OriginalRequestId,TEXT(""),TEXT("decline-invite"),Complete); }
void UHorizonGroupsManager::CancelInvite(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,OriginalRequestId,TEXT(""),TEXT("cancel-invite"),Complete); }
void UHorizonGroupsManager::AcceptRequest(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,OriginalRequestId,TEXT(""),TEXT("accept-request"),Complete); }
void UHorizonGroupsManager::DeclineRequest(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,OriginalRequestId,TEXT(""),TEXT("decline-request"),Complete); }
void UHorizonGroupsManager::CancelRequest(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,OriginalRequestId,TEXT(""),TEXT("cancel-request"),Complete); }
void UHorizonGroupsManager::LeaveGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,TEXT(""),TEXT(""),TEXT("leave"),Complete); }
void UHorizonGroupsManager::RemoveMember(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,TEXT(""),TEXT(""),TEXT("remove"),Complete); }
void UHorizonGroupsManager::SetMemberRole(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& Role,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,TEXT(""),Role,TEXT("role"),Complete); }
void UHorizonGroupsManager::TransferLeadership(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TargetUserId,ExpectedRevision,TEXT(""),TEXT(""),TEXT("transfer"),Complete); }
void UHorizonGroupsManager::DeleteGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete)
{ Action(UserId,GroupId,TEXT(""),ExpectedRevision,TEXT(""),TEXT(""),TEXT("delete"),Complete); }
void UHorizonGroupsManager::JoinGroupAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision)
{ JoinGroup(UserId,GroupId,ExpectedRevision,MutationEvent()); }
void UHorizonGroupsManager::RequestJoinAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision)
{ RequestJoin(UserId,GroupId,ExpectedRevision,MutationEvent()); }
void UHorizonGroupsManager::InvitePlayerAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision)
{ InvitePlayer(UserId,GroupId,TargetUserId,ExpectedRevision,MutationEvent()); }
void UHorizonGroupsManager::AcceptInviteAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId)
{ AcceptInvite(UserId,GroupId,ExpectedRevision,OriginalRequestId,MutationEvent()); }
void UHorizonGroupsManager::DeclineInviteAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId)
{ DeclineInvite(UserId,GroupId,ExpectedRevision,OriginalRequestId,MutationEvent()); }
void UHorizonGroupsManager::CancelInviteAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId)
{ CancelInvite(UserId,GroupId,TargetUserId,ExpectedRevision,OriginalRequestId,MutationEvent()); }
void UHorizonGroupsManager::AcceptRequestAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId)
{ AcceptRequest(UserId,GroupId,TargetUserId,ExpectedRevision,OriginalRequestId,MutationEvent()); }
void UHorizonGroupsManager::DeclineRequestAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId)
{ DeclineRequest(UserId,GroupId,TargetUserId,ExpectedRevision,OriginalRequestId,MutationEvent()); }
void UHorizonGroupsManager::CancelRequestAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId)
{ CancelRequest(UserId,GroupId,ExpectedRevision,OriginalRequestId,MutationEvent()); }
void UHorizonGroupsManager::LeaveGroupAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision)
{ LeaveGroup(UserId,GroupId,ExpectedRevision,MutationEvent()); }
void UHorizonGroupsManager::RemoveMemberAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision)
{ RemoveMember(UserId,GroupId,TargetUserId,ExpectedRevision,MutationEvent()); }
void UHorizonGroupsManager::SetMemberRoleAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& Role)
{ SetMemberRole(UserId,GroupId,TargetUserId,ExpectedRevision,Role,MutationEvent()); }
void UHorizonGroupsManager::TransferLeadershipAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision)
{ TransferLeadership(UserId,GroupId,TargetUserId,ExpectedRevision,MutationEvent()); }
void UHorizonGroupsManager::DeleteGroupAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision)
{ DeleteGroup(UserId,GroupId,ExpectedRevision,MutationEvent()); }
