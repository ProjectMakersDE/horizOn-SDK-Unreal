// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonValidatedActionsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonValidatedActionsManager.h"
#include "Math/RandomStream.h"
#include "Engine/GameInstance.h"

AHorizonValidatedActionsExample::AHorizonValidatedActionsExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonValidatedActionsExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonValidatedActionsExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonValidatedActionsExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonValidatedActionsExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonValidatedActionsExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->ValidatedActions)
	{
		UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] Auth or ValidatedActions manager not available."));
		return;
	}

	UHorizonValidatedActionsManager* ValidatedActions = Horizon->ValidatedActions;
	const FString BoardKey = LeaderboardKey;

	// Runs need a signed-in player; a leaderboard run also needs a display name.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[ValidatedActions, BoardKey](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] FAILED: sign-up did not complete."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Starting run..."));

			ValidatedActions->StartRun(BoardKey, FOnValidatedRunStarted::CreateLambda(
				[ValidatedActions](bool bStarted, const FHorizonValidatedRun& Run,
					const FString& ErrorCode, const FString& ErrorMessage)
				{
					if (!bStarted)
					{
						// For example RUN_RATE_LIMITED (retry later, not automatically) or NOT_SUPPORTED.
						UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] FAILED to start run (%s): %s"), *ErrorCode, *ErrorMessage);
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Run %s started, seed %d"), *Run.RunId, Run.Seed);

					// "Play": all randomness comes from the server seed, every input goes into the log.
					FRandomStream Random(Run.Seed);
					TArray<uint8> InputLog;
					int64 Score = 0;
					for (int32 Step = 0; Step < 16; ++Step)
					{
						const uint8 Input = static_cast<uint8>(Random.RandRange(0, 3)); // 0..3 = up, down, left, right
						InputLog.Add(Input);
						Score += 10 + Input;
					}

					UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Submitting score %lld, input log hash %s"),
						Score, *UHorizonValidatedActionsManager::ComputeInputLogHash(InputLog));

					// Empty stage and board: the ticket's board is used.
					ValidatedActions->SubmitValidated(Score, InputLog, FString(), FString(), TArray<FHorizonEarnedValue>(),
						FOnValidatedSubmitComplete::CreateLambda(
							[](bool bAccepted, const FHorizonValidatedSubmitResult& Result,
								const FString& SubmitErrorCode, const FString& SubmitErrorMessage)
							{
								if (!bAccepted)
								{
									// Rule codes such as DURATION_TOO_SHORT or SCORE_ABOVE_MAX; the ticket is used up.
									UE_LOG(LogTemp, Warning, TEXT("[ValidatedActionsExample] REJECTED (%s): %s"),
										*SubmitErrorCode, *SubmitErrorMessage);
									return;
								}

								UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] SUCCESS: best %lld, rank %lld, measured %lld s"),
									Result.BestScore, Result.Rank, Result.DurationSeconds);
							}));
				}));
		}));
}
