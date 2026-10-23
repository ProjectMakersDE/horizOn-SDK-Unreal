// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonAchievements.h"
#include "Models/HorizonStatistics.h"
#include "HorizonAchievementsExample.generated.h"
class UHorizonSubsystem;
UCLASS()
class HORIZONSDK_API UHorizonAchievementPendingDelivery : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString ProjectScope;
    UPROPERTY() FString UserId;
    UPROPERTY() TArray<FString> Keys;
};
/** Existing restored session only. Bind actions to UI, not BeginPlay or a timer. */
UCLASS()
class HORIZONSDK_API AHorizonAchievementsExample : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements Example") void LoadAchievements();
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements Example") void RefreshAfterStatistics(const FHorizonStatisticsSnapshot& Statistics);
    /** Repeat the same key after an uncertain result; the server owns its permanent identity. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements Example") void UnlockAchievement(const FString& Key);
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements Example") void DeliverPendingRewards(const TArray<FString>& Keys);
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements Example") void RetrySavedDelivery();
    UFUNCTION(BlueprintPure,Category="horizOn|Achievements Example") FHorizonAchievementsSnapshot GetCurrentAchievements() const;
    /** Link to Inbox: load Inbox, then bind its explicit protected Claim action to the player UI. */
    UFUNCTION(BlueprintPure,Category="horizOn|Achievements Example") FString GetDeliveredInboxEntryId(const FString& Key) const;
private:
    UHorizonSubsystem* Session() const;
    void Deliver(const TArray<FString>& Keys,bool Retry);
    bool bDelivering=false;
};
