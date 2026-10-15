// Copyright (c) 2025-2026 horizOn. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonStatistics.h"
#include "HorizonAsync_Statistics.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatisticsAsyncSuccess, const FHorizonStatisticsSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatisticsCatalogAsyncSuccess, const FHorizonStatisticsCatalog&, Catalog);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatisticsAsyncFailure, const FString&, ErrorCode, const FString&, ErrorMessage);

UCLASS()
class HORIZONSDK_API UHorizonAsync_GetStatistics : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable) FOnStatisticsAsyncSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable) FOnStatisticsAsyncFailure OnFailure;
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "horizOn|Statistics")
	static UHorizonAsync_GetStatistics* GetStatistics(const UObject* WorldContextObject);
	virtual void Activate() override;
private:
	TWeakObjectPtr<const UObject> WorldContext;
	void Complete(bool bSuccess, const FHorizonStatisticsSnapshot& Result, const FString& Code, const FString& Message);
};

UCLASS()
class HORIZONSDK_API UHorizonAsync_GetStatisticsDefinitions : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable) FOnStatisticsCatalogAsyncSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable) FOnStatisticsAsyncFailure OnFailure;
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "horizOn|Statistics")
	static UHorizonAsync_GetStatisticsDefinitions* GetStatisticsDefinitions(const UObject* WorldContextObject);
	virtual void Activate() override;
private:
	TWeakObjectPtr<const UObject> WorldContext;
	void Complete(bool bSuccess, const FHorizonStatisticsCatalog& Result, const FString& Code, const FString& Message);
};

UCLASS()
class HORIZONSDK_API UHorizonAsync_UpdateStatistics : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable) FOnStatisticsAsyncSuccess OnSuccess;
	UPROPERTY(BlueprintAssignable) FOnStatisticsAsyncFailure OnFailure;
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "horizOn|Statistics")
	static UHorizonAsync_UpdateStatistics* UpdateStatistics(const UObject* WorldContextObject, const TArray<FHorizonStatisticUpdate>& Updates, const FString& OperationId);
	virtual void Activate() override;
private:
	TWeakObjectPtr<const UObject> WorldContext;
	TArray<FHorizonStatisticUpdate> OriginalUpdates;
	FString OriginalOperationId;
	void Complete(bool bSuccess, const FHorizonStatisticsSnapshot& Result, const FString& Code, const FString& Message);
};
