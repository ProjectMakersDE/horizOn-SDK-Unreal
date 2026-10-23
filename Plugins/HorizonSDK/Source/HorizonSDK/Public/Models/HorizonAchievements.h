// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonPlayerValues.h"
#include "HorizonAchievements.generated.h"
class FJsonObject;
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementCondition
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString StatisticKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") int64 Threshold=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementRewards
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FHorizonPlayerValueAmount> Values;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementDefinition
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Name;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Description;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString ImageUrl;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bHidden=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bActive=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Mode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FHorizonAchievementCondition Condition;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FHorizonAchievementRewards Rewards;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementPendingReward
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString IssuedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FHorizonPlayerValueAmount> Values;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementProgress
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString UnlockedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString RewardState;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString InboxEntryId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FHorizonAchievementPendingReward PendingReward;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") int32 MaxDefinitions=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") int32 MaxLifetimeKeys=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementsSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString DefinitionVersion;
    /** False for wire null; manager materializes matching cached definitions before returning. */
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bDefinitionsIncluded=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FHorizonAchievementDefinition> Definitions;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FHorizonAchievementProgress> Progress;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FHorizonAchievementLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonAchievementsSnapshot& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementDeliveryResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString State;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString InboxEntryId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FString Code;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementUnlockResponse
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FHorizonAchievementsSnapshot Achievements;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FString> NewlyUnlocked;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") bool bReplayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FHorizonAchievementDeliveryResult> DeliveryResults;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonAchievementUnlockResponse& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonAchievementDeliveryResponse
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") FHorizonAchievementsSnapshot Achievements;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Achievements") TArray<FHorizonAchievementDeliveryResult> Results;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonAchievementDeliveryResponse& Out);
};
DECLARE_DELEGATE_FourParams(FOnAchievementsComplete,bool,const FHorizonAchievementsSnapshot&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnAchievementUnlockComplete,bool,const FHorizonAchievementUnlockResponse&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnAchievementDeliveryComplete,bool,const FHorizonAchievementDeliveryResponse&,const FString&,const FString&);
