#pragma once
#include "CoreMinimal.h"
#include "HorizonPlayerValues.generated.h"
class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerValueBalance
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString Key;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 Balance = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 EarnedToday = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bHasDailyCap = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 DailyCap = 0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerValueAmount
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString Key;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 Amount = 0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerValueOffer
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString Key;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString Name;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bActive = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") TArray<FHorizonPlayerValueAmount> Costs;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") TArray<FHorizonPlayerValueAmount> Rewards;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bHasPurchasesPerDay = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 PurchasesPerDay = 0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerValuesOperation
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString OperationId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bReplayed = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") TArray<FHorizonPlayerValueAmount> Applied;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerValuesState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString UserId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString Day;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 DefinitionVersion = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") TArray<FHorizonPlayerValueBalance> Values;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString ServerTime;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString OldestAcceptedAt;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FHorizonPlayerValuesOperation Operation;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json, FHorizonPlayerValuesState& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerValuesOffers
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") int64 DefinitionVersion = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString ServerTime;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") TArray<FHorizonPlayerValueBalance> Definitions;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") TArray<FHorizonPlayerValueOffer> Offers;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json, FHorizonPlayerValuesOffers& Out);
};
