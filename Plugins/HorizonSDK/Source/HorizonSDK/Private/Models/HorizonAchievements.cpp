#include "Models/HorizonAchievements.h"
#include "Models/HorizonInbox.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "String/LexFromString.h"
namespace HorizonAchievementsJson
{
bool Key(const FString& Value)
{
    if (Value.IsEmpty() || Value.Len()>24) return false;
    for (int32 I=0;I<Value.Len();++I) { const TCHAR C=Value[I]; if (!(C>='a' && C<='z') && !(C>='0' && C<='9') && (I==0 || (C!='.' && C!='_' && C!='-'))) return false; }
    return true;
}
bool String(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,FString& Out,int32 Max,bool Empty=false)
{ auto V=Object?Object->TryGetField(Key):nullptr; return V && V->Type==EJson::String && V->TryGetString(Out) && Out.Len()<=Max && (Empty || !Out.IsEmpty()); }
bool Optional(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,FString& Out,int32 Max)
{ auto V=Object?Object->TryGetField(Key):nullptr; return !V || V->IsNull() || String(Object,Key,Out,Max,true); }
bool Boolean(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,bool& Out)
{ auto V=Object?Object->TryGetField(Key):nullptr; return V && V->Type==EJson::Boolean && V->TryGetBool(Out); }
bool Number(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,int64& Out)
{
    auto V=Object?Object->TryGetField(Key):nullptr; FString Text;
    if (!V || V->Type!=EJson::Number || !V->TryGetString(Text) || Text.IsEmpty() || Text.Len()>16) return false;
    for (TCHAR C:Text) if (C<'0' || C>'9') return false;
    return LexTryParseString(Out,*Text) && Out>=0 && Out<=9007199254740991LL;
}
bool Time(const FString& Value) { FDateTime T; return Value.Len()<=40 && FDateTime::ParseIso8601(*Value,T); }
bool Amounts(const TSharedPtr<FJsonObject>& Object,TArray<FHorizonPlayerValueAmount>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Array; TSet<FString> Keys;
    if (!Object || !Object->TryGetArrayField(TEXT("values"),Array) || Array->Num()>8) return false;
    for (const auto& V:*Array)
    {
        FHorizonPlayerValueAmount A;
        if (!V || V->Type!=EJson::Object || !String(V->AsObject(),TEXT("key"),A.Key,24) || !Key(A.Key) || !Number(V->AsObject(),TEXT("amount"),A.Amount) || A.Amount==0 || Keys.Contains(A.Key)) return false;
        Keys.Add(A.Key); Out.Add(A);
    }
    return true;
}
bool Definition(const TSharedPtr<FJsonObject>& Object,FHorizonAchievementDefinition& D)
{
    const TSharedPtr<FJsonObject>* Reward;
    if (!String(Object,TEXT("key"),D.Key,24) || !Key(D.Key) || !String(Object,TEXT("name"),D.Name,120) || FTCHARToUTF8(*D.Name).Length()>120 ||
        !String(Object,TEXT("description"),D.Description,512,true) || FTCHARToUTF8(*D.Description).Length()>512 || !Optional(Object,TEXT("imageUrl"),D.ImageUrl,2048) ||
        (!D.ImageUrl.IsEmpty() && !D.ImageUrl.StartsWith(TEXT("http://")) && !D.ImageUrl.StartsWith(TEXT("https://"))) || !Boolean(Object,TEXT("hidden"),D.bHidden) ||
        !Boolean(Object,TEXT("active"),D.bActive) || !String(Object,TEXT("mode"),D.Mode,16) || !Object->TryGetObjectField(TEXT("rewards"),Reward) || !Amounts(*Reward,D.Rewards.Values)) return false;
    auto Condition=Object->TryGetField(TEXT("condition"));
    if (D.Mode==TEXT("manual")) return !Condition || Condition->IsNull();
    if (D.Mode!=TEXT("statistic") || !Condition || Condition->Type!=EJson::Object ||
        !String(Condition->AsObject(),TEXT("statisticKey"),D.Condition.StatisticKey,24) || !Key(D.Condition.StatisticKey) || !Number(Condition->AsObject(),TEXT("threshold"),D.Condition.Threshold)) return false;
    D.Condition.bPresent=true; return true;
}
bool State(const FString& Value) { return Value==TEXT("complete") || Value==TEXT("none") || Value==TEXT("pending") || Value==TEXT("delivered"); }
bool Entry(const FString& Value) { return Value.StartsWith(TEXT("message:")) && FHorizonInboxEntry::IsEntryId(Value); }
bool Progress(const TSharedPtr<FJsonObject>& Object,FHorizonAchievementProgress& P)
{
    if (!String(Object,TEXT("key"),P.Key,24) || !Key(P.Key) ||
        !String(Object,TEXT("rewardState"),P.RewardState,16) || !State(P.RewardState) || !Optional(Object,TEXT("inboxEntryId"),P.InboxEntryId,64)) return false;
    const auto UnlockedAt=Object->TryGetField(TEXT("unlockedAt"));
    if (P.RewardState==TEXT("complete"))
    { if (!UnlockedAt || !UnlockedAt->IsNull()) return false; }
    else if (!String(Object,TEXT("unlockedAt"),P.UnlockedAt,40) || !Time(P.UnlockedAt)) return false;
    auto Pending=Object->TryGetField(TEXT("pendingReward"));
    if (Pending && !Pending->IsNull())
    {
        if (Pending->Type!=EJson::Object || !String(Pending->AsObject(),TEXT("issuedAt"),P.PendingReward.IssuedAt,40) || !Time(P.PendingReward.IssuedAt) ||
            !Amounts(Pending->AsObject(),P.PendingReward.Values) || P.PendingReward.Values.IsEmpty()) return false;
        P.PendingReward.bPresent=true;
    }
    if (P.RewardState==TEXT("pending")) return P.PendingReward.bPresent && P.InboxEntryId.IsEmpty();
    return !P.PendingReward.bPresent && (P.RewardState==TEXT("delivered")?Entry(P.InboxEntryId):P.InboxEntryId.IsEmpty());
}
bool Results(const TSharedPtr<FJsonObject>& Object,const TCHAR* Field,TArray<FHorizonAchievementDeliveryResult>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Array; TSet<FString> Keys;
    if (!Object || !Object->TryGetArrayField(Field,Array) || Array->Num()>8) return false;
    for (const auto& V:*Array)
    {
        FHorizonAchievementDeliveryResult R;
        if (!V || V->Type!=EJson::Object || !String(V->AsObject(),TEXT("key"),R.Key,24) || !Key(R.Key) || Keys.Contains(R.Key) ||
            !String(V->AsObject(),TEXT("state"),R.State,16) || !State(R.State) || !Optional(V->AsObject(),TEXT("inboxEntryId"),R.InboxEntryId,64) || !Optional(V->AsObject(),TEXT("code"),R.Code,128)) return false;
        if (R.State==TEXT("delivered")?!Entry(R.InboxEntryId):!R.InboxEntryId.IsEmpty()) return false;
        Keys.Add(R.Key); Out.Add(R);
    }
    return true;
}
}
bool FHorizonAchievementsSnapshot::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonAchievementsSnapshot& Out)
{
    using namespace HorizonAchievementsJson;
    FHorizonAchievementsSnapshot S; FGuid User; int64 Active,Lifetime;
    const TSharedPtr<FJsonObject>* Limits; const TArray<TSharedPtr<FJsonValue>>* Rows;
    if (!String(Json,TEXT("userId"),S.UserId,36) || !FGuid::ParseExact(S.UserId,EGuidFormats::DigitsWithHyphens,User) || !Number(Json,TEXT("revision"),S.Revision) ||
        !String(Json,TEXT("definitionVersion"),S.DefinitionVersion,512) || !String(Json,TEXT("serverTime"),S.ServerTime,40) || !Time(S.ServerTime) ||
        !Json->TryGetObjectField(TEXT("limits"),Limits) || !Number(*Limits,TEXT("maxDefinitions"),Active) || Active<1 || Active>MAX_int32 ||
        !Number(*Limits,TEXT("maxLifetimeKeys"),Lifetime) || Lifetime<1 || Lifetime>MAX_int32 || !Json->TryGetArrayField(TEXT("progress"),Rows)) return false;
    S.Limits.MaxDefinitions=int32(Active); S.Limits.MaxLifetimeKeys=int32(Lifetime); TSet<FString> Keys;
    for (const auto& V:*Rows) { FHorizonAchievementProgress P; if (!V || V->Type!=EJson::Object || !HorizonAchievementsJson::Progress(V->AsObject(),P) || Keys.Contains(P.Key)) return false; Keys.Add(P.Key); S.Progress.Add(P); }
    auto Definitions=Json->TryGetField(TEXT("definitions")); if (!Definitions) return false;
    if (!Definitions->IsNull())
    {
        if (!Json->TryGetArrayField(TEXT("definitions"),Rows)) return false;
        Keys.Empty(); S.bDefinitionsIncluded=true;
        for (const auto& V:*Rows) { FHorizonAchievementDefinition D; if (!V || V->Type!=EJson::Object || !Definition(V->AsObject(),D) || Keys.Contains(D.Key)) return false; Keys.Add(D.Key); S.Definitions.Add(D); }
    }
    S.bPresent=true; Out=MoveTemp(S); return true;
}
bool FHorizonAchievementUnlockResponse::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonAchievementUnlockResponse& Out)
{
    FHorizonAchievementUnlockResponse R; const TSharedPtr<FJsonObject>* Snapshot; const TArray<TSharedPtr<FJsonValue>>* Keys;
    if (!Json || !Json->TryGetObjectField(TEXT("achievements"),Snapshot) || !FHorizonAchievementsSnapshot::TryFromJson(*Snapshot,R.Achievements) ||
        !HorizonAchievementsJson::Boolean(Json,TEXT("replayed"),R.bReplayed) || !Json->TryGetArrayField(TEXT("newlyUnlocked"),Keys) || Keys->Num()>1 ||
        !HorizonAchievementsJson::Results(Json,TEXT("deliveryResults"),R.DeliveryResults)) return false;
    for (const auto& V:*Keys) { FString Key; if (!V || V->Type!=EJson::String || !V->TryGetString(Key) || !HorizonAchievementsJson::Key(Key)) return false; R.NewlyUnlocked.Add(Key); }
    if (R.bReplayed && !R.NewlyUnlocked.IsEmpty()) return false;
    Out=MoveTemp(R); return true;
}
bool FHorizonAchievementDeliveryResponse::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonAchievementDeliveryResponse& Out)
{
    FHorizonAchievementDeliveryResponse R; const TSharedPtr<FJsonObject>* Snapshot;
    if (!Json || !Json->TryGetObjectField(TEXT("achievements"),Snapshot) || !FHorizonAchievementsSnapshot::TryFromJson(*Snapshot,R.Achievements) || !HorizonAchievementsJson::Results(Json,TEXT("results"),R.Results)) return false;
    Out=MoveTemp(R); return true;
}
