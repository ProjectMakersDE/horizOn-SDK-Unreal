// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonLeaderboardManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonLeaderboardManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonLeaderboardManager initialized."));
}

// ============================================================
// Submit Score
// ============================================================

void UHorizonLeaderboardManager::SubmitScore(int64 Score, FOnRequestComplete OnComplete, const FString& Metadata)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::SubmitScore -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);
	Body->SetNumberField(TEXT("score"), static_cast<double>(Score));
	if (!Metadata.IsEmpty())
	{
		Body->SetStringField(TEXT("metadata"), Metadata);
	}

	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/leaderboard/submit"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				UHorizonLeaderboardManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("Leaderboard::SubmitScore -- Score submitted successfully."));
					Self->ClearCache();
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::SubmitScore -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

// ============================================================
// Get Top
// ============================================================

void UHorizonLeaderboardManager::GetTop(int32 Limit, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetTop -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardEntry>());
		return;
	}

	const FString CacheKey = FString::Printf(TEXT("top%d"), Limit);

	if (bUseCache)
	{
		const TArray<FHorizonLeaderboardEntry>* Cached = EntriesCache.Find(CacheKey);
		if (Cached)
		{
			UE_LOG(LogHorizonSDK, Verbose, TEXT("Leaderboard::GetTop -- Returning cached data for key '%s'."), *CacheKey);
			OnComplete.ExecuteIfBound(true, *Cached);
			return;
		}
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString Endpoint = FString::Printf(TEXT("api/v1/app/leaderboard/top?userId=%s&limit=%d"), *UserId, Limit);

	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnLeaderboardEntriesComplete CapturedOnComplete = OnComplete;
	FString CapturedCacheKey = CacheKey;

	HttpClient->Get(Endpoint, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, CapturedCacheKey](const FHorizonNetworkResponse& Response)
			{
				UHorizonLeaderboardManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetTop -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardEntry>());
					return;
				}

				TArray<FHorizonLeaderboardEntry> Entries = ParseEntries(Response.JsonData);
				Self->EntriesCache.Add(CapturedCacheKey, Entries);

				UE_LOG(LogHorizonSDK, Log, TEXT("Leaderboard::GetTop -- Retrieved %d entries."), Entries.Num());
				CapturedOnComplete.ExecuteIfBound(true, Entries);
			}
		));
}

// ============================================================
// Get Rank
// ============================================================

void UHorizonLeaderboardManager::GetRank(bool bUseCache, FOnLeaderboardRankComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetRank -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, FHorizonLeaderboardEntry());
		return;
	}

	if (bUseCache && RankCache.IsSet())
	{
		UE_LOG(LogHorizonSDK, Verbose, TEXT("Leaderboard::GetRank -- Returning cached rank."));
		OnComplete.ExecuteIfBound(true, RankCache.GetValue());
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString Endpoint = FString::Printf(TEXT("api/v1/app/leaderboard/rank?userId=%s"), *UserId);

	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnLeaderboardRankComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(Endpoint, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				UHorizonLeaderboardManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetRank -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, FHorizonLeaderboardEntry());
					return;
				}

				FHorizonLeaderboardEntry Entry = FHorizonLeaderboardEntry::FromJson(Response.JsonData);
				Self->RankCache = Entry;

				UE_LOG(LogHorizonSDK, Log, TEXT("Leaderboard::GetRank -- Position: %d, Score: %lld"), Entry.Position, Entry.Score);
				CapturedOnComplete.ExecuteIfBound(true, Entry);
			}
		));
}

// ============================================================
// Get Around
// ============================================================

void UHorizonLeaderboardManager::GetAround(int32 Range, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetAround -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardEntry>());
		return;
	}

	const FString CacheKey = FString::Printf(TEXT("around%d"), Range);

	if (bUseCache)
	{
		const TArray<FHorizonLeaderboardEntry>* Cached = EntriesCache.Find(CacheKey);
		if (Cached)
		{
			UE_LOG(LogHorizonSDK, Verbose, TEXT("Leaderboard::GetAround -- Returning cached data for key '%s'."), *CacheKey);
			OnComplete.ExecuteIfBound(true, *Cached);
			return;
		}
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString Endpoint = FString::Printf(TEXT("api/v1/app/leaderboard/around?userId=%s&range=%d"), *UserId, Range);

	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnLeaderboardEntriesComplete CapturedOnComplete = OnComplete;
	FString CapturedCacheKey = CacheKey;

	HttpClient->Get(Endpoint, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, CapturedCacheKey](const FHorizonNetworkResponse& Response)
			{
				UHorizonLeaderboardManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetAround -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardEntry>());
					return;
				}

				TArray<FHorizonLeaderboardEntry> Entries = ParseEntries(Response.JsonData);
				Self->EntriesCache.Add(CapturedCacheKey, Entries);

				UE_LOG(LogHorizonSDK, Log, TEXT("Leaderboard::GetAround -- Retrieved %d entries."), Entries.Num());
				CapturedOnComplete.ExecuteIfBound(true, Entries);
			}
		));
}

// ============================================================
// Cache
// ============================================================

void UHorizonLeaderboardManager::ClearCache()
{
	EntriesCache.Empty();
	RankCache.Reset();
	UE_LOG(LogHorizonSDK, Verbose, TEXT("Leaderboard cache cleared."));
}

// ============================================================
// Helpers
// ============================================================

TArray<FHorizonLeaderboardEntry> UHorizonLeaderboardManager::ParseEntries(const TSharedPtr<FJsonObject>& JsonData)
{
	TArray<FHorizonLeaderboardEntry> Entries;
	if (!JsonData.IsValid())
	{
		return Entries;
	}

	const TArray<TSharedPtr<FJsonValue>>* EntriesArray;
	if (JsonData->TryGetArrayField(TEXT("entries"), EntriesArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *EntriesArray)
		{
			if (Value.IsValid() && Value->Type == EJson::Object)
			{
				Entries.Add(FHorizonLeaderboardEntry::FromJson(Value->AsObject()));
			}
		}
	}

	return Entries;
}
