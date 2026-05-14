// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonLeaderboardExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonLeaderboardManager.h"
#include "Engine/GameInstance.h"

AHorizonLeaderboardExample::AHorizonLeaderboardExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonLeaderboardExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeaderboardExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonLeaderboardExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonLeaderboardExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[LeaderboardExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonLeaderboardExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[LeaderboardExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonLeaderboardExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->Leaderboard)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeaderboardExample] Auth or Leaderboard manager not available."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LeaderboardExample] Connected. Signing in..."));

	UHorizonLeaderboardManager* Leaderboard = Horizon->Leaderboard;

	// Score submission requires a signed-in user, so sign up anonymously first.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[Leaderboard](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[LeaderboardExample] FAILED: sign-up did not complete."));
				return;
			}

			Leaderboard->SubmitScore(42000, FOnRequestComplete::CreateLambda(
				[Leaderboard](bool bSubmitSuccess, const FString& ErrorMessage)
				{
					if (!bSubmitSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[LeaderboardExample] FAILED to submit score: %s"), *ErrorMessage);
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[LeaderboardExample] Score submitted. Fetching top 5..."));

					Leaderboard->GetTop(5, false, FOnLeaderboardEntriesComplete::CreateLambda(
						[](bool bTopSuccess, const TArray<FHorizonLeaderboardEntry>& Entries)
						{
							if (!bTopSuccess)
							{
								UE_LOG(LogTemp, Error, TEXT("[LeaderboardExample] FAILED to fetch leaderboard."));
								return;
							}

							UE_LOG(LogTemp, Log, TEXT("[LeaderboardExample] SUCCESS: %d entries"), Entries.Num());
							for (const FHorizonLeaderboardEntry& Entry : Entries)
							{
								UE_LOG(LogTemp, Log, TEXT("[LeaderboardExample]   #%d %s %lld"),
									Entry.Position, *Entry.Username, Entry.Score);
							}
						}));
				}));
		}));
}
