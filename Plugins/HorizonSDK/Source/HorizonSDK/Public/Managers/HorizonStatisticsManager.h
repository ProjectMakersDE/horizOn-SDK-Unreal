// Copyright (c) 2025-2026 horizOn. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonStatistics.h"
#include "HorizonStatisticsManager.generated.h"

/** Explicit statistics snapshots and atomic batches. Initialization/sign-in performs no HTTP. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonStatisticsManager : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(UHorizonHttpClient* Http, UHorizonAuthManager* Auth);
	void GetStatistics(FOnStatisticsLoaded OnComplete);
	void GetDefinitions(FOnStatisticsDefinitionsLoaded OnComplete);
	/** Persist OperationId + original Updates before calling; never replace an uncertain ID. */
	void UpdateStatistics(const TArray<FHorizonStatisticUpdate>& Updates, const FString& OperationId, FOnStatisticsLoaded OnComplete);
	UFUNCTION(BlueprintCallable, Category = "horizOn|Statistics")
	FString CreateOperationId();
	UFUNCTION(BlueprintCallable, Category = "horizOn|Statistics")
	void ClearCache();
	UFUNCTION(BlueprintCallable, Category = "horizOn|Statistics")
	FHorizonStatisticsSnapshot GetCurrentStatistics();
	UFUNCTION(BlueprintCallable, Category = "horizOn|Statistics")
	FHorizonStatisticsCatalog GetCurrentDefinitions();
	UFUNCTION(BlueprintPure, Category = "horizOn|Statistics")
	FString GetLastErrorCode() const { return LastErrorCode; }
	UFUNCTION(BlueprintPure, Category = "horizOn|Statistics")
	FString GetLastSubmittedOperationId() const { return LastOperationId; }
private:
	UPROPERTY() UHorizonHttpClient* HttpClient = nullptr;
	UPROPERTY() UHorizonAuthManager* AuthManager = nullptr;
	struct FScope { FString User, Token, Host, Project; bool operator==(const FScope& Other) const { return User == Other.User && Token == Other.Token && Host == Other.Host && Project == Other.Project; } };
	FScope LastScope;
	FHorizonStatisticsSnapshot Current;
	FHorizonStatisticsCatalog Catalog;
	int64 CacheEpoch = 0, MaxRevision = -1, LastServerTime = 0, ClockOffset = 0;
	FString LastErrorCode, LastOperationId;
	FScope Scope() const;
	void SyncScope();
	bool HasSession() const;
	bool SameScope(const FScope& Expected, int64 Epoch);
	bool AcceptCatalog(const FHorizonStatisticsCatalog& Incoming);
	bool AcceptSnapshot(FHorizonStatisticsSnapshot& Incoming);
	void HandleSnapshot(const FHorizonNetworkResponse& Response, const FScope& Expected, int64 Epoch, const FString& OperationId, FOnStatisticsLoaded Complete);
	void Fail(const FString& Code, FOnStatisticsLoaded Complete, const FString& Message = TEXT("Statistics request failed"));
	UFUNCTION() void OnSignedIn();
	UFUNCTION() void OnSignedOut();
};
