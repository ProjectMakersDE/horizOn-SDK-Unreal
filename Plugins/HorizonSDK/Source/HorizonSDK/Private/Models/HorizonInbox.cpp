#include "Models/HorizonInbox.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "String/LexFromString.h"

namespace HorizonInboxJson
{
bool Number(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,int64& Out,int64 Maximum=9007199254740991LL)
{
    auto Value=Object?Object->TryGetField(Key):nullptr; FString Lexeme;
    if (!Value || Value->Type!=EJson::Number || !Value->TryGetString(Lexeme) || Lexeme.IsEmpty() || Lexeme.Len()>16) return false;
    for (TCHAR C:Lexeme) if (C<'0' || C>'9') return false;
    return LexTryParseString(Out,*Lexeme) && Out>=0 && Out<=Maximum;
}
bool SmallNumber(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,int32& Out,int64 Maximum=550)
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
bool Attachment(const TSharedPtr<FJsonValue>& Json,FHorizonInboxAttachment& Out)
{
    if (!Json) return false;
    if (Json->IsNull()) return true;
    if (Json->Type!=EJson::Object) return false;
    auto Object=Json->AsObject(); const TArray<TSharedPtr<FJsonValue>>* Values;
    if (Object->Values.Num()!=1 || !Object->TryGetArrayField(TEXT("values"),Values) || Values->Num()>8) return false;
    TSet<FString> Keys;
    for (const auto& Item:*Values)
    {
        if (!Item || Item->Type!=EJson::Object) return false;
        FHorizonPlayerValueAmount Value;
        if (!String(Item->AsObject(),TEXT("key"),Value.Key,24) || !Identifier(Value.Key,24) || !Number(Item->AsObject(),TEXT("amount"),Value.Amount) || Value.Amount==0 || Keys.Contains(Value.Key)) return false;
        Keys.Add(Value.Key); Out.Values.Add(Value);
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
        if (!Identifier(Key,32) || Key.Contains(TEXT(".")) || !Pair.Value || Pair.Value->Type!=EJson::String || !Pair.Value->TryGetString(Value) || FTCHARToUTF8(*Value).Length()>120) return false;
        Out.Parameters.Add(Key,Value);
    }
    return true;
}
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
        !Json->TryGetBoolField(TEXT("unchanged"),Value.bUnchanged) || !Json->TryGetArrayField(TEXT("entries"),Entries) || Entries->Num()>550 ||
        (Value.bUnchanged && Entries->Num()!=0) || !Json->TryGetObjectField(TEXT("counts"),Counts) || (*Counts)->Values.Num()>550 ||
        !Json->TryGetObjectField(TEXT("limits"),Limits) || !SmallNumber(*Limits,TEXT("targeted"),Value.Limits.Targeted,50) || Value.Limits.Targeted==0 ||
        !SmallNumber(*Limits,TEXT("broadcasts"),Value.Limits.Broadcasts,500) || Value.Limits.Broadcasts==0) return false;
    TSet<FString> Ids; TMap<FString,FHorizonInboxCount> Actual;
    for (const auto& JsonEntry:*Entries)
    {
        FHorizonInboxEntry Item;
        if (!Entry(JsonEntry,Item) || Ids.Contains(Item.Id)) return false;
        Ids.Add(Item.Id); auto& Count=Actual.FindOrAdd(Item.Type); Count.Total++; Count.Unread+=!Item.bRead;
        Count.Claimable+=!Item.bClaimed && !Item.Attachment.Values.IsEmpty(); Value.Entries.Add(MoveTemp(Item));
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
        !Json->TryGetObjectField(TEXT("playerValues"),ValuesJson) || !FHorizonPlayerValuesState::TryFromJson(*ValuesJson,Value.PlayerValues) || Value.PlayerValues.UserId!=Value.Inbox.UserId) return false;
    Value.bPresent=true; Out=MoveTemp(Value); return true;
}
