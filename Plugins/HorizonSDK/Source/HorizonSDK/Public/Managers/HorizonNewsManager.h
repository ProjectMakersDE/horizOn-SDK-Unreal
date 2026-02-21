// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Models/HorizonNewsEntry.h"
#include "HorizonNewsManager.generated.h"

DECLARE_DELEGATE_TwoParams(FOnNewsComplete, bool /*bSuccess*/, const TArray<FHorizonNewsEntry>& /*Entries*/);

/**
 * News Manager for the horizOn SDK.
 *
 * Fetches and caches news entries from the server with a 5-minute TTL.
 * Does NOT require a session token.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonNewsManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client reference. */
	void Initialize(UHorizonHttpClient* InHttpClient);

	/**
	 * Load news entries from the server.
	 * @param Limit        Maximum entries to retrieve (capped at 100).
	 * @param LanguageCode Language filter (e.g. "en", "de").
	 * @param bUseCache    If true, return cached data when within the 5-minute TTL.
	 * @param OnComplete   Called with (bSuccess, Entries).
	 */
	void LoadNews(int32 Limit, const FString& LanguageCode, bool bUseCache, FOnNewsComplete OnComplete);

	/**
	 * Look up a cached news entry by ID.
	 * @param Id  The news entry ID to find.
	 * @return    The matching entry, or an empty entry if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "horizOn|News")
	FHorizonNewsEntry GetNewsById(const FString& Id) const;

	/** Clear the cached news data. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|News")
	void ClearCache();

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	/** Cached news entries. */
	TArray<FHorizonNewsEntry> CachedNews;

	/** Timestamp (seconds since app start) when the cache was last populated. */
	double CacheTimestamp = 0.0;

	/** Cache TTL in seconds. */
	static constexpr double CacheTTLSeconds = 300.0; // 5 minutes
};
