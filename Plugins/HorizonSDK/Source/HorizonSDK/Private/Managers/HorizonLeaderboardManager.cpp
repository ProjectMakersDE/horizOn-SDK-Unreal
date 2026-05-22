// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonLeaderboardManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	FString NormalizeBoardKey(const FString& BoardKey)
	{
		return BoardKey.TrimStartAndEnd();
	}

	FString BuildLeaderboardEndpoint(const FString& BoardKey, const TCHAR* Action)
	{
		const FString NormalizedBoardKey = NormalizeBoardKey(BoardKey);
		if (NormalizedBoardKey.IsEmpty())
		{
			return FString::Printf(TEXT("api/v1/app/leaderboard/%s"), Action);
		}

		return FString::Printf(TEXT("api/v1/app/leaderboards/%s/%s"), *NormalizedBoardKey, Action);
	}

	FString BuildLeaderboardCacheKey(const FString& BoardKey, const TCHAR* Action, int32 Value)
	{
		FString NormalizedBoardKey = NormalizeBoardKey(BoardKey);
		if (NormalizedBoardKey.IsEmpty())
		{
			NormalizedBoardKey = TEXT("default");
		}

		return FString::Printf(TEXT("%s:%s:%d"), *NormalizedBoardKey, Action, Value);
	}
}

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

void UHorizonLeaderboardManager::SubmitScore(int64 Score, FOnRequestComplete OnComplete, const FString& Metadata, const FString& BoardKey)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::SubmitScore -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString NormalizedBoardKey = NormalizeBoardKey(BoardKey);

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);
	Body->SetNumberField(TEXT("score"), static_cast<double>(Score));
	if (!NormalizedBoardKey.IsEmpty())
	{
		Body->SetStringField(TEXT("leaderboardKey"), NormalizedBoardKey);
	}
	if (!Metadata.IsEmpty())
	{
		Body->SetStringField(TEXT("metadata"), Metadata);
	}

	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;
	const FString Endpoint = BuildLeaderboardEndpoint(NormalizedBoardKey, TEXT("submit"));

	HttpClient->PostJson(Body, Endpoint, true,
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

void UHorizonLeaderboardManager::GetTop(int32 Limit, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete, const FString& BoardKey)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetTop -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardEntry>());
		return;
	}

	const FString CacheKey = BuildLeaderboardCacheKey(BoardKey, TEXT("top"), Limit);

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
	const FString BaseEndpoint = BuildLeaderboardEndpoint(BoardKey, TEXT("top"));
	const FString Endpoint = FString::Printf(TEXT("%s?userId=%s&limit=%d"), *BaseEndpoint, *UserId, Limit);

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

void UHorizonLeaderboardManager::GetRank(bool bUseCache, FOnLeaderboardRankComplete OnComplete, const FString& BoardKey)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetRank -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, FHorizonLeaderboardEntry());
		return;
	}

	const FString CacheKey = BuildLeaderboardCacheKey(BoardKey, TEXT("rank"), 0);

	if (bUseCache)
	{
		const FHorizonLeaderboardEntry* Cached = RankCache.Find(CacheKey);
		if (Cached)
		{
			UE_LOG(LogHorizonSDK, Verbose, TEXT("Leaderboard::GetRank -- Returning cached rank for key '%s'."), *CacheKey);
			OnComplete.ExecuteIfBound(true, *Cached);
			return;
		}
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString BaseEndpoint = BuildLeaderboardEndpoint(BoardKey, TEXT("rank"));
	const FString Endpoint = FString::Printf(TEXT("%s?userId=%s"), *BaseEndpoint, *UserId);

	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnLeaderboardRankComplete CapturedOnComplete = OnComplete;
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
					UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetRank -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, FHorizonLeaderboardEntry());
					return;
				}

				FHorizonLeaderboardEntry Entry = FHorizonLeaderboardEntry::FromJson(Response.JsonData);
				Self->RankCache.Add(CapturedCacheKey, Entry);

				UE_LOG(LogHorizonSDK, Log, TEXT("Leaderboard::GetRank -- Position: %d, Score: %lld"), Entry.Position, Entry.Score);
				CapturedOnComplete.ExecuteIfBound(true, Entry);
			}
		));
}

// ============================================================
// Get Around
// ============================================================

void UHorizonLeaderboardManager::GetAround(int32 Range, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete, const FString& BoardKey)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::GetAround -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardEntry>());
		return;
	}

	const FString CacheKey = BuildLeaderboardCacheKey(BoardKey, TEXT("around"), Range);

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
	const FString BaseEndpoint = BuildLeaderboardEndpoint(BoardKey, TEXT("around"));
	const FString Endpoint = FString::Printf(TEXT("%s?userId=%s&range=%d"), *BaseEndpoint, *UserId, Range);

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
// List Boards
// ============================================================

void UHorizonLeaderboardManager::ListBoards(FOnLeaderboardBoardsComplete OnComplete)
{
	TWeakObjectPtr<UHorizonLeaderboardManager> WeakSelf(this);
	FOnLeaderboardBoardsComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(TEXT("api/v1/app/leaderboards"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Leaderboard::ListBoards -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<FHorizonLeaderboardBoard>());
					return;
				}

				TArray<FHorizonLeaderboardBoard> Boards;
				if (Response.JsonData.IsValid())
				{
					const TArray<TSharedPtr<FJsonValue>>* BoardsArray;
					if (Response.JsonData->TryGetArrayField(TEXT("boards"), BoardsArray))
					{
						for (const TSharedPtr<FJsonValue>& Value : *BoardsArray)
						{
							if (Value.IsValid() && Value->Type == EJson::Object)
							{
								Boards.Add(FHorizonLeaderboardBoard::FromJson(Value->AsObject()));
							}
						}
					}
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("Leaderboard::ListBoards -- Retrieved %d boards."), Boards.Num());
				CapturedOnComplete.ExecuteIfBound(true, Boards);
			}
		));
}

// ============================================================
// Cache
// ============================================================

void UHorizonLeaderboardManager::ClearCache()
{
	EntriesCache.Empty();
	RankCache.Empty();
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
