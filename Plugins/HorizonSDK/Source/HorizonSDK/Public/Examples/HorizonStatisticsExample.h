// Copyright (c) 2025-2026 horizOn. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonStatistics.h"
#include "HorizonStatisticsExample.generated.h"

/** A durable intent, scoped to the same project and player. No credentials stored here. */
class UHorizonExampleIdentity;

UCLASS()
class HORIZONSDK_API UHorizonStatisticsPendingBatch : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() FString UserId;
	UPROPERTY() FString OperationId;
	UPROPERTY() TArray<FHorizonStatisticUpdate> Updates;
};

/** Drop into a level after defining kills/sum and best_score/max in the dashboard. */
UCLASS()
class HORIZONSDK_API AHorizonStatisticsExample : public AActor
{
	GENERATED_BODY()
public:
	AHorizonStatisticsExample();
protected:
	virtual void BeginPlay() override;
private:
    friend struct FHorizonIdentityNative;
    UPROPERTY() UHorizonExampleIdentity* AuthOriginal=nullptr;
    FString AuthProject;
    bool bSignupSent=false,bRestoring=false,bAuthStarted=false;
    void AuthResult(bool Success);
	UFUNCTION() void Connected();
	void Authenticated(bool Success);
	void Loaded(bool Success, const FHorizonStatisticsSnapshot& Snapshot, const FString& Code, const FString& Message);
	void Applied(bool Success, const FHorizonStatisticsSnapshot& Snapshot, const FString& Code, const FString& Message);
	UPROPERTY() UHorizonStatisticsPendingBatch* Pending = nullptr;
	FString Slot;
};
