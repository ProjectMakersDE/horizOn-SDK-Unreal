// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Leaderboard.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonLeaderboardManager.h"

// ============================================================
// SubmitScore
// ============================================================

UHorizonAsync_LeaderboardSubmit* UHorizonAsync_LeaderboardSubmit::SubmitScore(
	const UObject* WorldContextObject, int64 Score, const FString& Metadata)
{
	UHorizonAsync_LeaderboardSubmit* Action = NewObject<UHorizonAsync_LeaderboardSubmit>();
	Action->WorldContext = WorldContextObject;
	Action->ScoreValue = Score;
	Action->MetadataStr = Metadata;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LeaderboardSubmit::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Leaderboard)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Leaderboard manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Leaderboard->SubmitScore(
		ScoreValue,
		FOnRequestComplete::CreateUObject(this, &UHorizonAsync_LeaderboardSubmit::HandleResult),
		MetadataStr
	);
}

void UHorizonAsync_LeaderboardSubmit::HandleResult(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast(ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// GetTopScores
// ============================================================

UHorizonAsync_LeaderboardGetTop* UHorizonAsync_LeaderboardGetTop::GetTopScores(
	const UObject* WorldContextObject, int32 Limit, bool bUseCache)
{
	UHorizonAsync_LeaderboardGetTop* Action = NewObject<UHorizonAsync_LeaderboardGetTop>();
	Action->WorldContext = WorldContextObject;
	Action->LimitValue = Limit;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LeaderboardGetTop::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Leaderboard)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Leaderboard manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Leaderboard->GetTop(
		LimitValue, bCache,
		FOnLeaderboardEntriesComplete::CreateUObject(this, &UHorizonAsync_LeaderboardGetTop::HandleResult)
	);
}

void UHorizonAsync_LeaderboardGetTop::HandleResult(bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Entries);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to retrieve top scores."));
	}
	SetReadyToDestroy();
}

// ============================================================
// GetRank
// ============================================================

UHorizonAsync_LeaderboardGetRank* UHorizonAsync_LeaderboardGetRank::GetRank(
	const UObject* WorldContextObject, bool bUseCache)
{
	UHorizonAsync_LeaderboardGetRank* Action = NewObject<UHorizonAsync_LeaderboardGetRank>();
	Action->WorldContext = WorldContextObject;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LeaderboardGetRank::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Leaderboard)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Leaderboard manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Leaderboard->GetRank(
		bCache,
		FOnLeaderboardRankComplete::CreateUObject(this, &UHorizonAsync_LeaderboardGetRank::HandleResult)
	);
}

void UHorizonAsync_LeaderboardGetRank::HandleResult(bool bSuccess, const FHorizonLeaderboardEntry& Entry)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Entry);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to retrieve rank."));
	}
	SetReadyToDestroy();
}

// ============================================================
// GetAroundScores
// ============================================================

UHorizonAsync_LeaderboardGetAround* UHorizonAsync_LeaderboardGetAround::GetAroundScores(
	const UObject* WorldContextObject, int32 Range, bool bUseCache)
{
	UHorizonAsync_LeaderboardGetAround* Action = NewObject<UHorizonAsync_LeaderboardGetAround>();
	Action->WorldContext = WorldContextObject;
	Action->RangeValue = Range;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LeaderboardGetAround::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Leaderboard)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Leaderboard manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Leaderboard->GetAround(
		RangeValue, bCache,
		FOnLeaderboardEntriesComplete::CreateUObject(this, &UHorizonAsync_LeaderboardGetAround::HandleResult)
	);
}

void UHorizonAsync_LeaderboardGetAround::HandleResult(bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Entries);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to retrieve surrounding scores."));
	}
	SetReadyToDestroy();
}
