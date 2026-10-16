#include "Models/HorizonGiftCodes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "String/LexFromString.h"

namespace HorizonGiftCodesJson
{
bool Key(const FString& Value)
{
    if (Value.IsEmpty() || Value.Len()>24 || !((Value[0]>='a' && Value[0]<='z') || (Value[0]>='0' && Value[0]<='9'))) return false;
    for (TCHAR C:Value) if (!(C>='a' && C<='z') && !(C>='0' && C<='9') && C!='_' && C!='.' && C!='-') return false;
    return true;
}
bool PositiveSafe(const TSharedPtr<FJsonObject>& Json,int64& Out)
{
    const auto Value=Json->TryGetField(TEXT("amount")); FString Lexeme;
    if (!Value || Value->Type!=EJson::Number || !Value->TryGetString(Lexeme) || Lexeme.IsEmpty() || Lexeme.Len()>16) return false;
    for (TCHAR C:Lexeme) if (C<'0' || C>'9') return false;
    return LexTryParseString(Out,*Lexeme) && Out>0 && Out<=9007199254740991LL;
}
}
bool FHorizonGiftCodeRedeemResult::TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGiftCodeRedeemResult& Out)
{
    FHorizonGiftCodeRedeemResult Result;
    if (!Json) return false;
    const auto Success=Json->TryGetField(TEXT("success")),Message=Json->TryGetField(TEXT("message"));
    if (!Success || Success->Type!=EJson::Boolean || !Success->TryGetBool(Result.bSuccess) ||
        !Message || Message->Type!=EJson::String || !Message->TryGetString(Result.Message)) return false;
    const auto Data=Json->TryGetField(TEXT("giftData"));
    if (Data && !Data->IsNull() && (Data->Type!=EJson::String || !Data->TryGetString(Result.GiftData))) return false;
    if (Json->HasField(TEXT("grantedUnlocks")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Values;
        if (!Json->TryGetArrayField(TEXT("grantedUnlocks"),Values)) return false;
        for (const auto& Value:*Values) { if (!Value || Value->Type!=EJson::String) return false; Result.GrantedUnlocks.Add(Value->AsString()); }
    }
    if (Json->HasField(TEXT("grantedValues")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Values; TSet<FString> Keys;
        if (!Json->TryGetArrayField(TEXT("grantedValues"),Values) || Values->Num()>8) return false;
        for (const auto& Value:*Values)
        {
            FHorizonPlayerValueAmount Amount;
            if (!Value || Value->Type!=EJson::Object) return false;
            const auto Key=Value->AsObject()->TryGetField(TEXT("key"));
            if (!Key || Key->Type!=EJson::String || !Key->TryGetString(Amount.Key) ||
                !HorizonGiftCodesJson::Key(Amount.Key) || !HorizonGiftCodesJson::PositiveSafe(Value->AsObject(),Amount.Amount) || Keys.Contains(Amount.Key)) return false;
            Keys.Add(Amount.Key); Result.GrantedValues.Add(Amount);
        }
    }
    const auto State=Json->TryGetField(TEXT("playerValues"));
    if (State && !State->IsNull() && (State->Type!=EJson::Object || !FHorizonPlayerValuesState::TryFromJson(State->AsObject(),Result.PlayerValues))) return false;
    if (!Result.GrantedValues.IsEmpty() && !Result.PlayerValues.bPresent) return false;
    const auto Replay=Json->TryGetField(TEXT("replayed"));
    if (Replay && (Replay->Type!=EJson::Boolean || !Replay->TryGetBool(Result.bReplayed))) return false;
    Out=MoveTemp(Result); return true;
}
