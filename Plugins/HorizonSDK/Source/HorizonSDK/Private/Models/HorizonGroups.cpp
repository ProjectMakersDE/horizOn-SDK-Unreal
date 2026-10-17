#include "Models/HorizonGroups.h"
#include "Http/HorizonLosslessJson.h"
#include "Models/HorizonGroupsWire.h"
#include "Transport/HorizonGroupsTransportContract.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "String/LexFromString.h"
namespace HorizonGroupsJson
{
bool Uuid(const FString& S) { FGuid G; return FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G); }
bool String(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FString& S,int32 Bytes,bool Empty=false)
{ auto V=J?J->TryGetField(Key):nullptr;return V && V->Type==EJson::String && V->TryGetString(S) && FTCHARToUTF8(*S,S.Len()).Length()<=Bytes && (Empty || !S.IsEmpty()); }
bool UuidField(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FString& S) {return String(J,Key,S,36) && Uuid(S);}
bool Number(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,int64& N,int64 Max=9007199254740991LL)
{auto V=J?J->TryGetField(Key):nullptr;FString S;if(!V || V->Type!=EJson::Number || !V->TryGetString(S) || S.IsEmpty() || S.Len()>16)return false;for(TCHAR C:S)if(C<'0'||C>'9')return false;return LexTryParseString(N,*S) && N>=0 && N<=Max;}
bool Small(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,int32& N,int32 Max) {int64 V;if(!Number(J,Key,V,Max)||V<1)return false;N=int32(V);return true;}
bool OptionalTime(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FString& S,bool& Has);
bool Time(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FString& S) {FDateTime T;return String(J,Key,S,40) && FDateTime::ParseIso8601(*S,T);}
bool OptionalTime(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FString& S,bool& Has){auto V=J?J->TryGetField(Key):nullptr;Has=V&&!V->IsNull();return !Has||Time(J,Key,S);}
bool Role(const FString& S) {return S==TEXT("leader")||S==TEXT("officer")||S==TEXT("member");}
bool Mode(const FString& S) {return S==TEXT("open")||S==TEXT("request")||S==TEXT("invite");}
bool ParseData(const TSharedPtr<FJsonObject>& J,FHorizonGroupJsonData& Out)
{
    FString Raw;
    return J && HorizonLosslessJson::Object(J.ToSharedRef(),Raw) && FHorizonGroupJsonData::TryFromJson(Raw,Out);
}
bool Display(const TSharedPtr<FJsonObject>& J,FHorizonFriendDisplay& Out)
{
    if(!J)return false; auto Name=J->TryGetField(TEXT("name"));if(Name && !Name->IsNull() && !String(J,TEXT("name"),Out.Name,1024,true))return false;
    auto P=J->TryGetField(TEXT("profile"));if(!P||P->IsNull())return true;if(P->Type!=EJson::Object)return false;
    auto Profile=P->AsObject();for(auto Pair:{TPair<const TCHAR*,FString*>(TEXT("avatarId"),&Out.Profile.AvatarId),TPair<const TCHAR*,FString*>(TEXT("frameId"),&Out.Profile.FrameId)}){auto V=Profile->TryGetField(Pair.Key);if(V&&!V->IsNull()&&!String(Profile,Pair.Key,*Pair.Value,128,true))return false;}
    auto Badges=Profile->TryGetField(TEXT("badges"));if(Badges&&!Badges->IsNull()){if(Badges->Type!=EJson::Array||Badges->AsArray().Num()>32)return false;for(auto V:Badges->AsArray()){FString S;if(!V||V->Type!=EJson::String||!V->TryGetString(S)||S.Len()>128)return false;Out.Profile.Badges.Add(S);}}return true;
}
bool PlayerPending(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,TArray<FHorizonGroupPendingView>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows;TSet<FString> Seen;if(!J->TryGetArrayField(Key,Rows))return false;
    for(auto V:*Rows){FHorizonGroupPendingView P;if(!V||V->Type!=EJson::Object)return false;auto O=V->AsObject();if(!UuidField(O,TEXT("groupId"),P.GroupId)||!UuidField(O,TEXT("requestId"),P.RequestId)||!UuidField(O,TEXT("issuerId"),P.IssuerId)||!OptionalTime(O,TEXT("createdAt"),P.CreatedAt,P.bHasCreatedAt)||Seen.Contains(P.GroupId.ToLower()))return false;Seen.Add(P.GroupId.ToLower());auto G=O->TryGetField(TEXT("group"));P.bHasGroup=G&&!G->IsNull();if(P.bHasGroup && (G->Type!=EJson::Object||!FHorizonGroupView::TryFromJson(G->AsObject(),P.Group)||P.Group.Id!=P.GroupId||P.Group.bHasData||P.Group.bHasMyRole))return false;Out.Add(P);}return true;
}
bool GroupPending(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,TArray<FHorizonGroupPending>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows;TSet<FString> Seen;if(!J->TryGetArrayField(Key,Rows))return false;
    for(auto V:*Rows){FHorizonGroupPending P;if(!V||V->Type!=EJson::Object)return false;auto O=V->AsObject();if(!UuidField(O,TEXT("userId"),P.UserId)||!UuidField(O,TEXT("requestId"),P.RequestId)||!UuidField(O,TEXT("issuerId"),P.IssuerId)||!OptionalTime(O,TEXT("createdAt"),P.CreatedAt,P.bHasCreatedAt)||Seen.Contains(P.UserId.ToLower()))return false;Seen.Add(P.UserId.ToLower());Out.Add(P);}return true;
}
}
bool FHorizonGroupJsonData::TryFromJson(const FString& Raw,FHorizonGroupJsonData& Out)
{
    const FTCHARToUTF8 Bytes(*Raw);std::string Compact;
    if(Raw!=FString(UTF8_TO_TCHAR(Bytes.Get()))||!HorizonGroupsContract::CompactObject(std::string(Bytes.Get(),Bytes.Length()),Compact,1024,size_t(Bytes.Length())))return false;
    FHorizonGroupJsonData Data;Data.Json=UTF8_TO_TCHAR(Compact.c_str());Out=MoveTemp(Data);return true;
}
bool FHorizonGroupView::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonGroupView& Out)
{
    using namespace HorizonGroupsJson;FHorizonGroupView G;
    if(!UuidField(J,TEXT("id"),G.Id)||!Number(J,TEXT("revision"),G.Revision)||!String(J,TEXT("name"),G.Name,MAX_int32)||!String(J,TEXT("description"),G.Description,MAX_int32,true)||!String(J,TEXT("joinMode"),G.JoinMode,7)||!Mode(G.JoinMode)||!UuidField(J,TEXT("leaderId"),G.LeaderId)||!Small(J,TEXT("memberCount"),G.MemberCount,MAX_int32)||!Time(J,TEXT("createdAt"),G.CreatedAt)||!Time(J,TEXT("updatedAt"),G.UpdatedAt))return false;
    auto D=J->TryGetField(TEXT("data")),R=J->TryGetField(TEXT("myRole"));G.bHasData=D&&!D->IsNull();G.bHasMyRole=R&&!R->IsNull();
    if(G.bHasData!=G.bHasMyRole || (G.bHasData && (D->Type!=EJson::Object||!(D->PreferStringRepresentation()?([&](){FString Raw;return D->TryGetString(Raw)&&FHorizonGroupJsonData::TryFromJson(Raw,G.Data);})():ParseData(D->AsObject(),G.Data)))) || (G.bHasMyRole && (!String(J,TEXT("myRole"),G.MyRole,7)||!Role(G.MyRole))))return false;
    G.bPresent=true;Out=MoveTemp(G);return true;
}
bool FHorizonMyGroupsView::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonMyGroupsView& Out)
{
    using namespace HorizonGroupsJson;FHorizonMyGroupsView S;const TSharedPtr<FJsonObject>* L;const TArray<TSharedPtr<FJsonValue>>* Rows;
    if(!UuidField(J,TEXT("userId"),S.UserId)||!Number(J,TEXT("revision"),S.Revision)||!Time(J,TEXT("serverTime"),S.ServerTime)||!J->TryGetObjectField(TEXT("limits"),L)||!Small(*L,TEXT("maxMembers"),S.Limits.MaxMembers,MAX_int32)||!Small(*L,TEXT("maxGroupsPerPlayer"),S.Limits.MaxGroupsPerPlayer,MAX_int32)||!Small(*L,TEXT("maxInvitations"),S.Limits.MaxInvitations,MAX_int32)||!Small(*L,TEXT("maxJoinRequests"),S.Limits.MaxJoinRequests,MAX_int32)||!Small(*L,TEXT("maxDataBytes"),S.Limits.MaxDataBytes,MAX_int32)||!Small(*L,TEXT("memberPageSize"),S.Limits.MemberPageSize,MAX_int32)||!Small(*L,TEXT("maxPlayerInvitations"),S.Limits.MaxPlayerInvitations,MAX_int32)||!Small(*L,TEXT("maxPlayerRequests"),S.Limits.MaxPlayerRequests,MAX_int32)||!Small(*L,TEXT("maxNameBytes"),S.Limits.MaxNameBytes,MAX_int32)||!Small(*L,TEXT("maxDescriptionBytes"),S.Limits.MaxDescriptionBytes,MAX_int32)||!Small(*L,TEXT("maxDataDepth"),S.Limits.MaxDataDepth,MAX_int32)||!J->TryGetArrayField(TEXT("groups"),Rows)||!PlayerPending(J,TEXT("incomingInvites"),S.IncomingInvites)||!PlayerPending(J,TEXT("outgoingRequests"),S.OutgoingRequests))return false;
    TSet<FString> Seen;for(auto V:*Rows){FHorizonGroupView G;if(!V||V->Type!=EJson::Object||!FHorizonGroupView::TryFromJson(V->AsObject(),G)||!G.bHasMyRole||!G.bHasData||Seen.Contains(G.Id.ToLower()))return false;Seen.Add(G.Id.ToLower());S.Groups.Add(G);}S.bPresent=true;Out=MoveTemp(S);return true;
}
bool FHorizonGroupMutationView::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonGroupMutationView& Out)
{
    FHorizonGroupMutationView M;const TSharedPtr<FJsonObject>* G;if(!J||!J->TryGetObjectField(TEXT("groups"),G)||!FHorizonMyGroupsView::TryFromJson(*G,M.Groups))return false;auto V=J->TryGetField(TEXT("group"));M.bHasGroup=V&&!V->IsNull();if(M.bHasGroup&&(V->Type!=EJson::Object||!FHorizonGroupView::TryFromJson(V->AsObject(),M.Group)))return false;Out=MoveTemp(M);return true;
}
bool FHorizonGroupMembersView::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonGroupMembersView& Out)
{
    using namespace HorizonGroupsJson;FHorizonGroupMembersView S;const TArray<TSharedPtr<FJsonValue>>* Rows;
    if(!UuidField(J,TEXT("groupId"),S.GroupId)||!Number(J,TEXT("revision"),S.Revision)||!Time(J,TEXT("serverTime"),S.ServerTime)||!J->TryGetArrayField(TEXT("members"),Rows))return false;
    TSet<FString> Seen;for(auto V:*Rows){FHorizonGroupMemberView M;const TSharedPtr<FJsonObject>* D;if(!V||V->Type!=EJson::Object)return false;auto O=V->AsObject();if(!UuidField(O,TEXT("userId"),M.UserId)||!String(O,TEXT("role"),M.Role,7)||!Role(M.Role)||!Time(O,TEXT("joinedAt"),M.JoinedAt)||!O->TryGetObjectField(TEXT("display"),D)||!Display(*D,M.Display)||Seen.Contains(M.UserId.ToLower()))return false;Seen.Add(M.UserId.ToLower());S.Members.Add(M);}
    auto Cursor=J->TryGetField(TEXT("nextAfterUserId"));S.bHasNextAfterUserId=Cursor&&!Cursor->IsNull();if(S.bHasNextAfterUserId && (!UuidField(J,TEXT("nextAfterUserId"),S.NextAfterUserId)||S.Members.IsEmpty()||S.NextAfterUserId!=S.Members.Last().UserId))return false;Out=MoveTemp(S);return true;
}
bool FHorizonGroupPendingSnapshot::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonGroupPendingSnapshot& Out)
{
    using namespace HorizonGroupsJson;FHorizonGroupPendingSnapshot S;if(!UuidField(J,TEXT("groupId"),S.GroupId)||!Number(J,TEXT("revision"),S.Revision)||!Time(J,TEXT("serverTime"),S.ServerTime)||!GroupPending(J,TEXT("invitations"),S.Invitations)||!GroupPending(J,TEXT("joinRequests"),S.JoinRequests))return false;Out=MoveTemp(S);return true;
}
