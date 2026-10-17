#include "Models/HorizonFriends.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/Base64.h"
#include "Misc/Parse.h"
#include "String/LexFromString.h"
namespace HorizonFriendsJson
{
bool Uuid(const FString& Value) { FGuid Id; return FGuid::ParseExact(Value,EGuidFormats::DigitsWithHyphens,Id); }
bool String(const TSharedPtr<FJsonObject>& Object,const TCHAR* Field,FString& Out,int32 Maximum,bool Empty=false)
{ auto V=Object?Object->TryGetField(Field):nullptr; return V && V->Type==EJson::String && V->TryGetString(Out) && Out.Len()<=Maximum && (Empty || !Out.IsEmpty()); }
bool Optional(const TSharedPtr<FJsonObject>& Object,const TCHAR* Field,FString& Out,int32 Maximum)
{ auto V=Object?Object->TryGetField(Field):nullptr; return !V || V->IsNull() || String(Object,Field,Out,Maximum,true); }
bool Number(const TSharedPtr<FJsonObject>& Object,const TCHAR* Field,int64& Out,int64 Maximum=9007199254740991LL)
{
    auto V=Object?Object->TryGetField(Field):nullptr; FString Text;
    if (!V || V->Type!=EJson::Number || !V->TryGetString(Text) || Text.IsEmpty() || Text.Len()>16) return false;
    for (TCHAR C:Text) if (C<'0' || C>'9') return false;
    return LexTryParseString(Out,*Text) && Out>=0 && Out<=Maximum;
}
bool Small(const TSharedPtr<FJsonObject>& Object,const TCHAR* Field,int32& Out,int64 Maximum)
{ int64 Value; if (!Number(Object,Field,Value,Maximum) || Value<1) return false; Out=int32(Value); return true; }
bool Time(const FString& Value) { FDateTime Parsed; return Value.Len()<=40 && FDateTime::ParseIso8601(*Value,Parsed); }
bool Display(const TSharedPtr<FJsonValue>& Value,FHorizonFriendDisplay& Out)
{
    if (!Value || Value->IsNull()) return true;
    if (Value->Type!=EJson::Object || !Optional(Value->AsObject(),TEXT("name"),Out.Name,1024)) return false;
    const auto Profile=Value->AsObject()->TryGetField(TEXT("profile"));
    if (!Profile || Profile->IsNull()) return true;
    if (Profile->Type!=EJson::Object || !Optional(Profile->AsObject(),TEXT("avatarId"),Out.Profile.AvatarId,128) || !Optional(Profile->AsObject(),TEXT("frameId"),Out.Profile.FrameId,128)) return false;
    if (Profile->AsObject()->HasField(TEXT("badges")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Badges;
        if (!Profile->AsObject()->TryGetArrayField(TEXT("badges"),Badges) || Badges->Num()>32) return false;
        for (const auto& Badge:*Badges) { FString Text; if (!Badge || Badge->Type!=EJson::String || !Badge->TryGetString(Text) || Text.Len()>128) return false; Out.Profile.Badges.Add(Text); }
    }
    return true;
}
bool OptionalTime(const TSharedPtr<FJsonObject>& J,const TCHAR* Key,FString& Out,bool& Has){auto V=J?J->TryGetField(Key):nullptr;Has=V&&!V->IsNull();return !Has||(String(J,Key,Out,40)&&Time(Out));}
bool Requests(const TSharedPtr<FJsonObject>& Json,const TCHAR* Field,const FString& Actor,TArray<FHorizonFriendRequest>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows; TSet<FString> Peers;
    if (!Json->TryGetArrayField(Field,Rows)) return false;
    for (const auto& V:*Rows)
    {
        FHorizonFriendRequest R;
        if (!V || V->Type!=EJson::Object || !String(V->AsObject(),TEXT("userId"),R.UserId,36) || !Uuid(R.UserId) || R.UserId.Equals(Actor,ESearchCase::IgnoreCase) || Peers.Contains(R.UserId.ToLower()) ||
            !String(V->AsObject(),TEXT("requestId"),R.RequestId,36) || !Uuid(R.RequestId) || !OptionalTime(V->AsObject(),TEXT("createdAt"),R.CreatedAt,R.bHasCreatedAt) || !Display(V->AsObject()->TryGetField(TEXT("display")),R.Display)) return false;
        Peers.Add(R.UserId.ToLower()); Out.Add(R);
    }
    return true;
}
}
FString FHorizonFriendLookup::FriendCodeFor(const FString& User)
{
    if (!HorizonFriendsJson::Uuid(User)) return {};
    FString Digits=User.Replace(TEXT("-"),TEXT("")); TArray<uint8> Bytes; Bytes.Reserve(16);
    for (int32 I=0;I<32;I+=2) Bytes.Add(uint8((FParse::HexDigit(Digits[I])<<4)|FParse::HexDigit(Digits[I+1])));
    return TEXT("H-")+FBase64::Encode(Bytes,EBase64Mode::UrlSafe).Replace(TEXT("="),TEXT(""));
}
bool FHorizonFriendLookup::IsPlayerQuery(const FString& Value)
{
    if (HorizonFriendsJson::Uuid(Value)) return true;
    if (!Value.StartsWith(TEXT("H-")) || Value.Len()!=24) return false;
    const FString Encoded=Value.Mid(2);
    for (TCHAR C:Encoded) if (!(C>='a' && C<='z') && !(C>='A' && C<='Z') && !(C>='0' && C<='9') && C!='_' && C!='-') return false;
    TArray<uint8> Bytes;
    return FBase64::Decode(Encoded+TEXT("=="),Bytes,EBase64Mode::UrlSafe) && Bytes.Num()==16 && FBase64::Encode(Bytes,EBase64Mode::UrlSafe).Replace(TEXT("="),TEXT(""))==Encoded;
}
bool FHorizonFriendLookup::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonFriendLookup& Out)
{
    FHorizonFriendLookup R;
    if (!HorizonFriendsJson::String(Json,TEXT("userId"),R.UserId,36) || !HorizonFriendsJson::Uuid(R.UserId) || !HorizonFriendsJson::String(Json,TEXT("friendCode"),R.FriendCode,24) ||
        R.FriendCode!=FriendCodeFor(R.UserId) || !HorizonFriendsJson::Display(Json->TryGetField(TEXT("display")),R.Display)) return false;
    R.bPresent=true; Out=MoveTemp(R); return true;
}
bool FHorizonFriendsSnapshot::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonFriendsSnapshot& Out)
{
    using namespace HorizonFriendsJson;
    FHorizonFriendsSnapshot S; const TSharedPtr<FJsonObject>* Limits; const TArray<TSharedPtr<FJsonValue>>* Rows;
    if (!String(Json,TEXT("userId"),S.UserId,36) || !Uuid(S.UserId) || !Number(Json,TEXT("revision"),S.Revision) ||
        !String(Json,TEXT("friendCode"),S.FriendCode,24) || S.FriendCode!=FHorizonFriendLookup::FriendCodeFor(S.UserId) || !String(Json,TEXT("serverTime"),S.ServerTime,40) || !Time(S.ServerTime) ||
        !Json->TryGetObjectField(TEXT("limits"),Limits) || !Small(*Limits,TEXT("maxFriends"),S.Limits.MaxFriends,MAX_int32) ||
        !Small(*Limits,TEXT("maxIncomingRequests"),S.Limits.MaxIncomingRequests,MAX_int32) || !Small(*Limits,TEXT("maxOutgoingRequests"),S.Limits.MaxOutgoingRequests,MAX_int32) || !Small(*Limits,TEXT("maxBlocked"),S.Limits.MaxBlocked,MAX_int32) ||
        !Requests(Json,TEXT("incoming"),S.UserId,S.Incoming) || !Requests(Json,TEXT("outgoing"),S.UserId,S.Outgoing) || !Json->TryGetArrayField(TEXT("friends"),Rows)) return false;
    TSet<FString> Peers;
    for (const auto& V:*Rows)
    {
        FHorizonFriend F;
        if (!V || V->Type!=EJson::Object || !String(V->AsObject(),TEXT("userId"),F.UserId,36) || !Uuid(F.UserId) || F.UserId.Equals(S.UserId,ESearchCase::IgnoreCase) || Peers.Contains(F.UserId.ToLower()) ||
            !OptionalTime(V->AsObject(),TEXT("since"),F.Since,F.bHasSince) || !Optional(V->AsObject(),TEXT("lastActiveAt"),F.LastActiveAt,40) ||
            (!F.LastActiveAt.IsEmpty() && !Time(F.LastActiveAt)) || !HorizonFriendsJson::Display(V->AsObject()->TryGetField(TEXT("display")),F.Display)) return false;
        const auto Login=V->AsObject()->TryGetField(TEXT("lastActiveAt"));
        F.bHasLastActiveAt=Login && !Login->IsNull();
        if (F.bHasLastActiveAt && !Time(F.LastActiveAt)) return false;
        Peers.Add(F.UserId.ToLower()); S.Friends.Add(F);
    }
    if (!Json->TryGetArrayField(TEXT("blocked"),Rows)) return false;
    Peers.Empty();
    for (const auto& V:*Rows)
    {
        FHorizonBlockedPlayer P;
        if (!V || V->Type!=EJson::Object || !String(V->AsObject(),TEXT("userId"),P.UserId,36) || !Uuid(P.UserId) || P.UserId.Equals(S.UserId,ESearchCase::IgnoreCase) || Peers.Contains(P.UserId.ToLower()) ||
            !String(V->AsObject(),TEXT("blockedAt"),P.BlockedAt,40) || !Time(P.BlockedAt)) return false;
        Peers.Add(P.UserId.ToLower()); S.Blocked.Add(P);
    }
    S.bPresent=true; Out=MoveTemp(S); return true;
}
