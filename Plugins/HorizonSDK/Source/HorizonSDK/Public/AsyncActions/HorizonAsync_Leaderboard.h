// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonLeaderboardEntry.h"
#include "HorizonAsync_Leaderboard.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeaderboardAsyncSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaderboardAsyncFailure, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaderboardEntriesResult, const TArray<FHorizonLeaderboardEntry>&, Entries);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaderboardRankResult, const FHorizonLeaderboardEntry&, Entry);

/**
 * Async Blueprint node: Submit a leaderboard score.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LeaderboardSubmit : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardAsyncFailure OnFailure;

	/** Submit a score to the leaderboard. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Submit Leaderboard Score"), Category = "horizOn|Leaderboard")
	static UHorizonAsync_LeaderboardSubmit* SubmitScore(const UObject* WorldContextObject, int64 Score, const FString& Metadata);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	int64 ScoreValue;
	FString MetadataStr;

	void HandleResult(bool bSuccess, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node: Get top leaderboard entries.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LeaderboardGetTop : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardEntriesResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardAsyncFailure OnFailure;

	/** Get the top N leaderboard entries. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Top Scores"), Category = "horizOn|Leaderboard")
	static UHorizonAsync_LeaderboardGetTop* GetTopScores(const UObject* WorldContextObject, int32 Limit, bool bUseCache);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	int32 LimitValue;
	bool bCache;

	void HandleResult(bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries);
};

// ============================================================

/**
 * Async Blueprint node: Get the current user's rank.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LeaderboardGetRank : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardRankResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardAsyncFailure OnFailure;

	/** Get the current user's leaderboard rank. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Leaderboard Rank"), Category = "horizOn|Leaderboard")
	static UHorizonAsync_LeaderboardGetRank* GetRank(const UObject* WorldContextObject, bool bUseCache);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	bool bCache;

	void HandleResult(bool bSuccess, const FHorizonLeaderboardEntry& Entry);
};

// ============================================================

/**
 * Async Blueprint node: Get leaderboard entries around the current user.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LeaderboardGetAround : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardEntriesResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardAsyncFailure OnFailure;

	/** Get leaderboard entries surrounding the current user. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Around Scores"), Category = "horizOn|Leaderboard")
	static UHorizonAsync_LeaderboardGetAround* GetAroundScores(const UObject* WorldContextObject, int32 Range, bool bUseCache);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	int32 RangeValue;
	bool bCache;

	void HandleResult(bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries);
};
