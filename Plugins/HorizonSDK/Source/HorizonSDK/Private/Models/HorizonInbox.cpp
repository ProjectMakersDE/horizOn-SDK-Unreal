// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Models/HorizonInbox.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "String/LexFromString.h"
#include "Models/HorizonItemsWire.h"

namespace HorizonInboxJson
{
bool Number(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,int64& Out,int64 Maximum=9007199254740991LL)
{
    auto Value=Object?Object->TryGetField(Key):nullptr; FString Lexeme;
    if (!Value || Value->Type!=EJson::Number || !Value->TryGetString(Lexeme) || Lexeme.IsEmpty() || Lexeme.Len()>16) return false;
    for (TCHAR C:Lexeme) if (C<'0' || C>'9') return false;
    return LexTryParseString(Out,*Lexeme) && Out>=0 && Out<=Maximum;
}
bool SmallNumber(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,int32& Out,int64 Maximum=2147483647LL)
{ int64 Value; if (!Number(Object,Key,Value,Maximum)) return false; Out=int32(Value); return true; }
bool String(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,FString& Out,int32 Max,bool Empty=false)
{ return Object && Object->TryGetStringField(Key,Out) && Out.Len()<=Max && (Empty || !Out.IsEmpty()); }
bool Identifier(const FString& Value,int32 Max)
{
    if (Value.IsEmpty() || Value.Len()>Max) return false;
    for (TCHAR C:Value) if (!(C>='a' && C<='z') && !(C>='A' && C<='Z') && !(C>='0' && C<='9') && C!='_' && C!='-' && C!='.') return false;
    return true;
}
bool Cursor(const FString& Value)
{
    for (TCHAR C:Value) if (!(C>='a' && C<='z') && !(C>='A' && C<='Z') && !(C>='0' && C<='9') && C!=':' && C!='_' && C!='-') return false;
    return true;
}
bool Time(const FString& Value) { FDateTime Parsed; return !Value.IsEmpty() && Value.Len()<=40 && FDateTime::ParseIso8601(*Value,Parsed); }
bool Uuid(const FString& Value) { FGuid Id; return Value.Len()==36 && FGuid::ParseExact(Value,EGuidFormats::DigitsWithHyphens,Id); }
bool Attachment(const TSharedPtr<FJsonValue>& Json,FHorizonInboxAttachment& Out)
{
    if (!Json) return false;
    if (Json->IsNull()) return true;
    if (Json->Type!=EJson::Object) return false;
    auto Object=Json->AsObject(); const TArray<TSharedPtr<FJsonValue>>* Values;
    for (const auto& Pair:Object->Values) if (Pair.Key!=TEXT("values") && Pair.Key!=TEXT("itemReturn")) return false;
    TArray<TSharedPtr<FJsonValue>> Empty;
    if (Object->HasField(TEXT("values"))) { if (!Object->TryGetArrayField(TEXT("values"),Values) || Values->Num()>8) return false; }
    else Values=&Empty;
    TSet<FString> Keys;
    for (const auto& Item:*Values)
    {
        if (!Item || Item->Type!=EJson::Object) return false;
        FHorizonPlayerValueAmount Value;
        if (Item->AsObject()->Values.Num()!=2 || !String(Item->AsObject(),TEXT("key"),Value.Key,24) || !Identifier(Value.Key,24) || !Number(Item->AsObject(),TEXT("amount"),Value.Amount) || Value.Amount==0 || Keys.Contains(Value.Key)) return false;
        Keys.Add(Value.Key); Out.Values.Add(Value);
    }
    auto Return=Object->TryGetField(TEXT("itemReturn"));
    if (Return && !Return->IsNull())
    {
        if (Return->Type!=EJson::Object) return false;
        const auto Item=Return->AsObject(); auto& R=Out.ItemReturn;
        if (Item->Values.Num()!=3 || !String(Item,TEXT("listingId"),R.ListingId,36) || !Uuid(R.ListingId) ||
            !String(Item,TEXT("instanceId"),R.InstanceId,36) || !Uuid(R.InstanceId) || !String(Item,TEXT("itemKey"),R.ItemKey,64)) return false;
        const FTCHARToUTF8 Key(*R.ItemKey,R.ItemKey.Len());
        if (!HorizonItemsContract::Key(std::string(Key.Get(),Key.Length()))) return false;
        R.bPresent=true;
    }
    Out.bPresent=true; return true;
}
bool Entry(const TSharedPtr<FJsonValue>& Json,FHorizonInboxEntry& Out)
{
    if (!Json || Json->Type!=EJson::Object) return false;
    const auto Object=Json->AsObject(); const TSharedPtr<FJsonObject>* Parameters;
    if (!String(Object,TEXT("id"),Out.Id,64) || !FHorizonInboxEntry::IsEntryId(Out.Id) || !String(Object,TEXT("kind"),Out.Kind,16) ||
        !Out.Id.StartsWith(Out.Kind+TEXT(":")) || !String(Object,TEXT("type"),Out.Type,32) || !Identifier(Out.Type,32) ||
        !String(Object,TEXT("reference"),Out.Reference,128,true) || !String(Object,TEXT("textKey"),Out.TextKey,128) ||
        !String(Object,TEXT("createdAt"),Out.CreatedAt,40) || !Time(Out.CreatedAt) || !String(Object,TEXT("expiresAt"),Out.ExpiresAt,40) || !Time(Out.ExpiresAt) ||
        !Object->TryGetBoolField(TEXT("overdue"),Out.bOverdue) || !Object->TryGetBoolField(TEXT("read"),Out.bRead) || !Object->TryGetBoolField(TEXT("claimed"),Out.bClaimed) ||
        !Object->TryGetObjectField(TEXT("parameters"),Parameters) || (*Parameters)->Values.Num()>8 || !Attachment(Object->TryGetField(TEXT("attachment")),Out.Attachment)) return false;
    for (const auto& Pair:(*Parameters)->Values)
    {
        FString Value; const FString Key(*Pair.Key);
        const bool FreeText=Out.Kind==TEXT("message") && Out.Type==TEXT("free_text") && Out.TextKey==TEXT("horizon.inbox.free_text") && Key==TEXT("text");
        if (!Identifier(Key,32) || Key.Contains(TEXT(".")) || !Pair.Value || Pair.Value->Type!=EJson::String || !Pair.Value->TryGetString(Value) || (!FreeText && FTCHARToUTF8(*Value).Length()>120)) return false;
        Out.Parameters.Add(Key,Value);
    }
    FHorizonInboxSource Source;
    if (Out.SourceKind()!=EHorizonInboxSourceKind::Unknown && !Out.TryGetSource(Source)) return false;
    FString Text;
    if (Out.Kind==TEXT("message") && Out.Type==TEXT("free_text") && Out.TextKey==TEXT("horizon.inbox.free_text") && !Out.TryGetFreeText(Text)) return false;
    return true;
}
}
EHorizonInboxSourceKind FHorizonInboxEntry::SourceKind() const
{
    if (Kind!=TEXT("message")) return EHorizonInboxSourceKind::Unknown;
    const FString Pair=Type+TEXT(":")+TextKey;
    if (Pair==TEXT("friend_request:horizon.friends.request")) return EHorizonInboxSourceKind::FriendRequest;
    if (Pair==TEXT("friend_accept:horizon.friends.accept")) return EHorizonInboxSourceKind::FriendAccepted;
    if (Pair==TEXT("group_invite:horizon.groups.invite")) return EHorizonInboxSourceKind::GroupInvitation;
    if (Pair==TEXT("group_request:horizon.groups.request")) return EHorizonInboxSourceKind::GroupRequest;
    if (Pair==TEXT("group_accept-invite:horizon.groups.accept-invite")) return EHorizonInboxSourceKind::GroupInvitationAccepted;
    if (Pair==TEXT("group_accept-request:horizon.groups.accept-request")) return EHorizonInboxSourceKind::GroupRequestAccepted;
    if (Pair==TEXT("match_turn:horizon.matches.your_turn")) return EHorizonInboxSourceKind::MatchTurn;
    if (Pair==TEXT("marketplace:marketplace.item_return")) return EHorizonInboxSourceKind::MarketReturn;
    return EHorizonInboxSourceKind::Unknown;
}
bool FHorizonInboxEntry::TryGetSource(FHorizonInboxSource& Out) const
{
    FHorizonInboxSource Value; Value.Kind=SourceKind();
    if (Value.Kind==EHorizonInboxSourceKind::Unknown || !HorizonInboxJson::Uuid(Reference)) return false;
    Value.SourceId=Reference.ToLower();
    auto ParameterId=[&](const TCHAR* Name,FString& OutId) { const auto* Raw=Parameters.Find(Name); if (!Raw || !HorizonInboxJson::Uuid(*Raw)) return false; OutId=Raw->ToLower(); return true; };
    if (Value.Kind==EHorizonInboxSourceKind::FriendRequest || Value.Kind==EHorizonInboxSourceKind::FriendAccepted)
    { if (!ParameterId(TEXT("userId"),Value.UserId)) return false; }
    else if (Value.Kind==EHorizonInboxSourceKind::GroupInvitation || Value.Kind==EHorizonInboxSourceKind::GroupRequest || Value.Kind==EHorizonInboxSourceKind::GroupInvitationAccepted || Value.Kind==EHorizonInboxSourceKind::GroupRequestAccepted)
    {
        if (!ParameterId(TEXT("groupId"),Value.GroupId)) return false;
    }
    else if (Value.Kind==EHorizonInboxSourceKind::MatchTurn)
    {
        FString Match; const auto* Revision=Parameters.Find(TEXT("revision"));
        if (!ParameterId(TEXT("matchId"),Match) || Match!=Value.SourceId || !Revision || Revision->IsEmpty() || Revision->Len()>16) return false;
        for (TCHAR C:*Revision) if (C<'0' || C>'9') return false;
        if (!LexTryParseString(Value.Revision,**Revision) || Value.Revision<0 || Value.Revision>9007199254740991LL) return false;
        Value.bHasRevision=true;
    }
    else if (!Attachment.ItemReturn.bPresent || !Attachment.ItemReturn.ListingId.Equals(Reference,ESearchCase::IgnoreCase)) return false;
    Value.bPresent=true; Out=MoveTemp(Value); return true;
}
bool FHorizonInboxEntry::TryGetFreeText(FString& Out) const
{
    if (Kind!=TEXT("message") || Type!=TEXT("free_text") || TextKey!=TEXT("horizon.inbox.free_text")) return false;
    const auto* Text=Parameters.Find(TEXT("text")); if (!Text) return false; Out=*Text; return true;
}
bool FHorizonInboxEntry::IsEntryId(const FString& Value)
{
    FString Kind,Id; FGuid Guid;
    return Value.Split(TEXT(":"),&Kind,&Id) && (Kind==TEXT("message") || Kind==TEXT("broadcast")) && FGuid::ParseExact(Id,EGuidFormats::DigitsWithHyphens,Guid);
}
bool FHorizonInboxSnapshot::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonInboxSnapshot& Out)
{
    using namespace HorizonInboxJson;
    FHorizonInboxSnapshot Value; FGuid User; const TArray<TSharedPtr<FJsonValue>>* Entries;
    const TSharedPtr<FJsonObject>* Counts; const TSharedPtr<FJsonObject>* Limits;
    if (!String(Json,TEXT("userId"),Value.UserId,36) || !FGuid::ParseExact(Value.UserId,EGuidFormats::DigitsWithHyphens,User) ||
        !String(Json,TEXT("cursor"),Value.Cursor,128) || !HorizonInboxJson::Cursor(Value.Cursor) || !Number(Json,TEXT("revision"),Value.Revision) || !Number(Json,TEXT("broadcastRevision"),Value.BroadcastRevision) ||
        !String(Json,TEXT("serverTime"),Value.ServerTime,40) || !Time(Value.ServerTime) || !SmallNumber(Json,TEXT("pollAfterSeconds"),Value.PollAfterSeconds,2147483647LL) || Value.PollAfterSeconds==0 ||
        !Json->TryGetBoolField(TEXT("unchanged"),Value.bUnchanged) || !Json->TryGetArrayField(TEXT("entries"),Entries) ||
        (Value.bUnchanged && Entries->Num()!=0) || !Json->TryGetObjectField(TEXT("counts"),Counts) ||
        !Json->TryGetObjectField(TEXT("limits"),Limits) || !SmallNumber(*Limits,TEXT("targeted"),Value.Limits.Targeted) || Value.Limits.Targeted==0 ||
        !SmallNumber(*Limits,TEXT("broadcasts"),Value.Limits.Broadcasts) || Value.Limits.Broadcasts==0) return false;
    for (const auto& Limit: {TPair<const TCHAR*,int32*>(TEXT("freeTexts"),&Value.Limits.FreeTexts), TPair<const TCHAR*,int32*>(TEXT("freeTextBytes"),&Value.Limits.FreeTextBytes), TPair<const TCHAR*,int32*>(TEXT("freeTextRetentionDays"),&Value.Limits.FreeTextRetentionDays)})
        if ((*Limits)->HasField(Limit.Key) && (!SmallNumber(*Limits,Limit.Key,*Limit.Value) || *Limit.Value==0)) return false;
    TSet<FString> Ids; TMap<FString,FHorizonInboxCount> Actual;
    for (const auto& JsonEntry:*Entries)
    {
        FHorizonInboxEntry Item;
        if (!Entry(JsonEntry,Item) || Ids.Contains(Item.Id.ToLower())) return false;
        Ids.Add(Item.Id.ToLower()); auto& Count=Actual.FindOrAdd(Item.Type); Count.Total++; Count.Unread+=!Item.bRead;
        Count.Claimable+=Item.CanClaim(); Value.Entries.Add(MoveTemp(Item));
    }
    for (const auto& Pair:(*Counts)->Values)
    {
        const FString Key(*Pair.Key);
        if (!Identifier(Key,32) || !Pair.Value || Pair.Value->Type!=EJson::Object) return false;
        auto Object=Pair.Value->AsObject(); FHorizonInboxCount Count;
        if (!SmallNumber(Object,TEXT("total"),Count.Total) || !SmallNumber(Object,TEXT("unread"),Count.Unread) || !SmallNumber(Object,TEXT("claimable"),Count.Claimable) || Count.Unread>Count.Total || Count.Claimable>Count.Total) return false;
        Value.Counts.Add(Key,Count);
        if (!Value.bUnchanged)
        {
            const auto* Expected=Actual.Find(Key);
            if (!Expected || Expected->Total!=Count.Total || Expected->Unread!=Count.Unread || Expected->Claimable!=Count.Claimable) return false;
        }
    }
    if (!Value.bUnchanged && Actual.Num()!=Value.Counts.Num()) return false;
    Value.bPresent=true; Out=MoveTemp(Value); return true;
}
bool FHorizonInboxClaim::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonInboxClaim& Out)
{
    FHorizonInboxClaim Value; const TSharedPtr<FJsonObject>* InboxJson; const TSharedPtr<FJsonObject>* ValuesJson;
    if (!HorizonInboxJson::String(Json,TEXT("entryId"),Value.EntryId,64) || !FHorizonInboxEntry::IsEntryId(Value.EntryId) ||
        !Json->TryGetBoolField(TEXT("replayed"),Value.bReplayed) || !Json->TryGetObjectField(TEXT("inbox"),InboxJson) || !FHorizonInboxSnapshot::TryFromJson(*InboxJson,Value.Inbox) || Value.Inbox.bUnchanged ||
        !Json->TryGetObjectField(TEXT("playerValues"),ValuesJson) || !FHorizonPlayerValuesState::TryFromJson(*ValuesJson,Value.PlayerValues) || Value.PlayerValues.UserId!=Value.Inbox.UserId ||
        !FHorizonItemReference::TryArrayFromJson(Json,TEXT("items"),Value.Items)) return false;
    Value.bPresent=true; Out=MoveTemp(Value); return true;
}
