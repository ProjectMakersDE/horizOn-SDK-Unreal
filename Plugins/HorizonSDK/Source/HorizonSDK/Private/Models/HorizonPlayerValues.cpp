#include "Models/HorizonPlayerValues.h"
#include "Dom/JsonObject.h"
#include "String/LexFromString.h"
namespace
{
    constexpr int64 MaxSafe = 9007199254740991LL;
    bool Number(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, int64& Out)
    {
        const auto Value = Json ? Json->TryGetField(Key) : nullptr;
        FString Lexeme;
        if (!Value || Value->Type != EJson::Number || !Value->TryGetString(Lexeme) || Lexeme.IsEmpty() || Lexeme.Len() > 17) return false;
        for (int32 Index = 0; Index < Lexeme.Len(); ++Index)
            if (!(Index == 0 && Lexeme[Index] == TCHAR('-')) && (Lexeme[Index] < TCHAR('0') || Lexeme[Index] > TCHAR('9'))) return false;
        return LexTryParseString(Out, *Lexeme) && Out >= -MaxSafe && Out <= MaxSafe;
    }
    bool Balances(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, TArray<FHorizonPlayerValueBalance>& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Entries;
        if (!Json || !Json->TryGetArrayField(Key, Entries)) return false;
        for (const auto& Entry : *Entries)
        {
            if (!Entry || Entry->Type != EJson::Object) return false;
            const auto Object = Entry->AsObject(); FHorizonPlayerValueBalance Value;
            if (!Object->TryGetStringField(TEXT("key"), Value.Key) || !Number(Object, TEXT("balance"), Value.Balance) || !Number(Object, TEXT("earnedToday"), Value.EarnedToday) || Value.EarnedToday < 0) return false;
            const auto Cap = Object->TryGetField(TEXT("dailyCap"));
            Value.bHasDailyCap = Cap.IsValid() && !Cap->IsNull();
            if (Value.bHasDailyCap && (!Number(Object, TEXT("dailyCap"), Value.DailyCap) || Value.DailyCap < 0)) return false;
            Out.Add(Value);
        }
        return true;
    }
    bool Amounts(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, TArray<FHorizonPlayerValueAmount>& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Entries;
        if (!Json || !Json->TryGetArrayField(Key, Entries)) return false;
        for (const auto& Entry : *Entries)
        {
            if (!Entry || Entry->Type != EJson::Object) return false;
            FHorizonPlayerValueAmount Value;
            if (!Entry->AsObject()->TryGetStringField(TEXT("key"), Value.Key) || !Number(Entry->AsObject(), TEXT("amount"), Value.Amount)) return false;
            Out.Add(Value);
        }
        return true;
    }
}
bool FHorizonPlayerValuesState::TryFromJson(const TSharedPtr<FJsonObject>& Json, FHorizonPlayerValuesState& Out)
{
    FHorizonPlayerValuesState Value;
    const TSharedPtr<FJsonObject>* Window;
    if (!Json || !Json->TryGetStringField(TEXT("userId"), Value.UserId) || !Json->TryGetStringField(TEXT("day"), Value.Day) ||
        !Number(Json, TEXT("revision"), Value.Revision) || Value.Revision < 0 || !Number(Json, TEXT("definitionVersion"), Value.DefinitionVersion) || Value.DefinitionVersion < 0 ||
        !Balances(Json, TEXT("values"), Value.Values) || !Json->TryGetObjectField(TEXT("operationWindow"), Window) ||
        !(*Window)->TryGetStringField(TEXT("serverTime"), Value.ServerTime) || !(*Window)->TryGetStringField(TEXT("oldestAcceptedAt"), Value.OldestAcceptedAt)) return false;
    const TSharedPtr<FJsonObject>* Operation;
    if (Json->TryGetObjectField(TEXT("operation"), Operation))
    {
        Value.Operation.bPresent = true;
        if (!(*Operation)->TryGetStringField(TEXT("operationId"), Value.Operation.OperationId) || !(*Operation)->TryGetBoolField(TEXT("replayed"), Value.Operation.bReplayed) || !Amounts(*Operation, TEXT("applied"), Value.Operation.Applied)) return false;
    }
    Value.bPresent = true; Out = MoveTemp(Value); return true;
}
bool FHorizonPlayerValuesOffers::TryFromJson(const TSharedPtr<FJsonObject>& Json, FHorizonPlayerValuesOffers& Out)
{
    FHorizonPlayerValuesOffers Value;
    const TArray<TSharedPtr<FJsonValue>>* Offers;
    if (!Json || !Number(Json, TEXT("definitionVersion"), Value.DefinitionVersion) || Value.DefinitionVersion < 0 || !Json->TryGetStringField(TEXT("serverTime"), Value.ServerTime) || !Balances(Json, TEXT("definitions"), Value.Definitions) || !Json->TryGetArrayField(TEXT("offers"), Offers)) return false;
    for (const auto& Entry : *Offers)
    {
        if (!Entry || Entry->Type != EJson::Object) return false;
        const auto Object = Entry->AsObject(); FHorizonPlayerValueOffer Offer;
        if (!Object->TryGetStringField(TEXT("key"), Offer.Key) || !Object->TryGetStringField(TEXT("name"), Offer.Name) || !Object->TryGetBoolField(TEXT("active"), Offer.bActive) || !Amounts(Object, TEXT("costs"), Offer.Costs) || !Amounts(Object, TEXT("rewards"), Offer.Rewards)) return false;
        const auto Limit = Object->TryGetField(TEXT("purchasesPerDay"));
        Offer.bHasPurchasesPerDay = Limit.IsValid() && !Limit->IsNull();
        if (Offer.bHasPurchasesPerDay && (!Number(Object, TEXT("purchasesPerDay"), Offer.PurchasesPerDay) || Offer.PurchasesPerDay < 0)) return false;
        Value.Offers.Add(Offer);
    }
    Value.bPresent = true; Out = MoveTemp(Value); return true;
}
