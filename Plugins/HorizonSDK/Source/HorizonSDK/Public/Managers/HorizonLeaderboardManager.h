// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonLeaderboardBoard.h"
#include "Models/HorizonLeaderboardEntry.h"
#include "HorizonLeaderboardManager.generated.h"

DECLARE_DELEGATE_TwoParams(FOnLeaderboardEntriesComplete, bool /*bSuccess*/, const TArray<FHorizonLeaderboardEntry>& /*Entries*/);
DECLARE_DELEGATE_TwoParams(FOnLeaderboardRankComplete, bool /*bSuccess*/, const FHorizonLeaderboardEntry& /*Entry*/);
DECLARE_DELEGATE_TwoParams(FOnLeaderboardBoardsComplete, bool /*bSuccess*/, const TArray<FHorizonLeaderboardBoard>& /*Boards*/);

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
	 * @param BoardKey   Optional board key for multi-board leaderboards.
	 */
	void SubmitScore(int64 Score, FOnRequestComplete OnComplete, const FString& Metadata = TEXT(""), const FString& BoardKey = TEXT(""));

	/**
	 * Get the top N leaderboard entries.
	 * @param Limit      Maximum number of entries to retrieve.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Entries).
	 * @param BoardKey   Optional board key for multi-board leaderboards.
	 */
	void GetTop(int32 Limit, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete, const FString& BoardKey = TEXT(""));

	/**
	 * Get the rank entry for the current signed-in user.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Entry).
	 * @param BoardKey   Optional board key for multi-board leaderboards.
	 */
	void GetRank(bool bUseCache, FOnLeaderboardRankComplete OnComplete, const FString& BoardKey = TEXT(""));

	/**
	 * Get leaderboard entries around the current user.
	 * @param Range      Number of entries above and below.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Entries).
	 * @param BoardKey   Optional board key for multi-board leaderboards.
	 */
	void GetAround(int32 Range, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete, const FString& BoardKey = TEXT(""));

	/**
	 * List available leaderboard boards for this app.
	 * @param OnComplete Called with (bSuccess, Boards).
	 */
	void ListBoards(FOnLeaderboardBoardsComplete OnComplete);

	/** Clear all cached leaderboard data. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Leaderboard")
	void ClearCache();

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;

	/** Cache for entries, keyed by board/action/range. */
	TMap<FString, TArray<FHorizonLeaderboardEntry>> EntriesCache;

	/** Cache for the current user's rank, keyed by board. */
	TMap<FString, FHorizonLeaderboardEntry> RankCache;

	/** Parse an array of leaderboard entries from a JSON "entries" field. */
	static TArray<FHorizonLeaderboardEntry> ParseEntries(const TSharedPtr<FJsonObject>& JsonData);
};
