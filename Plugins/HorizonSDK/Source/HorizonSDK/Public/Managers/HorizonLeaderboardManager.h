// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonLeaderboardEntry.h"
#include "HorizonLeaderboardManager.generated.h"

DECLARE_DELEGATE_TwoParams(FOnLeaderboardEntriesComplete, bool /*bSuccess*/, const TArray<FHorizonLeaderboardEntry>& /*Entries*/);
DECLARE_DELEGATE_TwoParams(FOnLeaderboardRankComplete, bool /*bSuccess*/, const FHorizonLeaderboardEntry& /*Entry*/);

/**
 * Leaderboard Manager for the horizOn SDK.
 *
 * Supports submitting scores, retrieving top entries, player rank,
 * and surrounding entries with in-memory caching.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonLeaderboardManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/**
	 * Submit a score for the current signed-in user.
	 * Clears the leaderboard cache on success.
	 * @param Score      The score to submit.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 * @param Metadata   Optional metadata string.
	 */
	void SubmitScore(int64 Score, FOnRequestComplete OnComplete, const FString& Metadata = TEXT(""));

	/**
	 * Get the top N leaderboard entries.
	 * @param Limit      Maximum number of entries to retrieve.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Entries).
	 */
	void GetTop(int32 Limit, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete);

	/**
	 * Get the rank entry for the current signed-in user.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Entry).
	 */
	void GetRank(bool bUseCache, FOnLeaderboardRankComplete OnComplete);

	/**
	 * Get leaderboard entries around the current user.
	 * @param Range      Number of entries above and below.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Entries).
	 */
	void GetAround(int32 Range, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete);

	/** Clear all cached leaderboard data. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Leaderboard")
	void ClearCache();

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;

	/** Cache for entries (keyed by "top{limit}" or "around{range}"). */
	TMap<FString, TArray<FHorizonLeaderboardEntry>> EntriesCache;

	/** Cache for the current user's rank. */
	TOptional<FHorizonLeaderboardEntry> RankCache;

	/** Parse an array of leaderboard entries from a JSON "entries" field. */
	static TArray<FHorizonLeaderboardEntry> ParseEntries(const TSharedPtr<FJsonObject>& JsonData);
};
