#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonPlayerValues.h"
#include "HorizonGiftCodes.generated.h"

/** Additive redeem receipt. GiftData stays the original opaque string. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGiftCodeRedeemResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") bool bSuccess=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString Message;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString GiftData;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") TArray<FString> GrantedUnlocks;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") TArray<FHorizonPlayerValueAmount> GrantedValues;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FHorizonPlayerValuesState PlayerValues;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") bool bReplayed=false;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGiftCodeRedeemResult& Out);
};
