// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonValidatedActionsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonCloudSaveManager.h"
#include "Managers/HorizonValidatedActionsManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Math/RandomStream.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/GameInstance.h"

AHorizonValidatedActionsExample::AHorizonValidatedActionsExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

UHorizonSubsystem* AHorizonValidatedActionsExample::GetHorizon() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
}

void AHorizonValidatedActionsExample::BeginPlay()
{
	Super::BeginPlay();

	UHorizonSubsystem* Horizon = GetHorizon();
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
	UHorizonSubsystem* Horizon = GetHorizon();
	if (!Horizon || !Horizon->Auth || !Horizon->ValidatedActions)
	{
		UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] Auth or ValidatedActions manager not available."));
		return;
	}

	UHorizonValidatedActionsManager* ValidatedActions = Horizon->ValidatedActions;
	TWeakObjectPtr<AHorizonValidatedActionsExample> WeakThis(this);

	// Runs need a signed-in player; a leaderboard run also needs a display name.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[WeakThis, ValidatedActions](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[ValidatedActionsExample] FAILED: sign-up did not complete."));
				return;
			}

			// Step 1: read the server-owned values (at game start, before showing currency).
			ValidatedActions->GetState(FOnPlayerStateLoaded::CreateLambda(
				[WeakThis](bool bLoaded, const FHorizonPlayerState& State, const FString& ErrorCode, const FString& ErrorMessage)
				{
					if (bLoaded)
					{
						UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] State on %s: %d value(s)"), *State.Day, State.Values.Num());
					}
					else
					{
						// NOT_SUPPORTED on a backend without the feature; the run below still shows the flow.
						UE_LOG(LogTemp, Warning, TEXT("[ValidatedActionsExample] GetState failed (%s): %s"), *ErrorCode, *ErrorMessage);
					}

					if (AHorizonValidatedActionsExample* Self = WeakThis.Get())
					{
						Self->StartAndSubmitRun();
					}
				}));
		}));
}

void AHorizonValidatedActionsExample::StartAndSubmitRun()
{
	UHorizonSubsystem* Horizon = GetHorizon();
	if (!Horizon || !Horizon->ValidatedActions)
	{
		return;
	}

	UHorizonValidatedActionsManager* ValidatedActions = Horizon->ValidatedActions;
	TWeakObjectPtr<AHorizonValidatedActionsExample> WeakThis(this);

	// Earned values only for keys the rules define; an unknown key rejects the run (UNKNOWN_VALUE_KEY).
	TArray<FHorizonEarnedValue> Earned;
	if (!ValueKey.IsEmpty())
	{
		Earned.Add(FHorizonEarnedValue(ValueKey, EarnedAmount));
	}

	UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Starting run..."));

	ValidatedActions->StartRun(LeaderboardKey, FOnValidatedRunStarted::CreateLambda(
		[WeakThis, ValidatedActions, Earned](bool bStarted, const FHorizonValidatedRun& Run,
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
			ValidatedActions->SubmitValidated(Score, InputLog, FString(), FString(), Earned,
				FOnValidatedSubmitComplete::CreateLambda(
					[WeakThis](bool bAccepted, const FHorizonValidatedSubmitResult& Result,
						const FString& SubmitErrorCode, const FString& SubmitErrorMessage)
					{
						if (!bAccepted)
						{
							// Rule codes such as DURATION_TOO_SHORT, value codes such as UNKNOWN_VALUE_KEY or
							// INSUFFICIENT_BALANCE; the ticket is used up. LEADERBOARD_MISMATCH keeps the run.
							UE_LOG(LogTemp, Warning, TEXT("[ValidatedActionsExample] REJECTED (%s): %s"),
								*SubmitErrorCode, *SubmitErrorMessage);
							return;
						}

						UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] SUCCESS: best %lld, rank %lld, measured %lld s"),
							Result.BestScore, Result.Rank, Result.DurationSeconds);

						if (AHorizonValidatedActionsExample* Self = WeakThis.Get())
						{
							Self->HandleStateAfterRun(Result.State);
						}
					}));
		}));
}

void AHorizonValidatedActionsExample::HandleStateAfterRun(const FHorizonPlayerState& State)
{
	if (State.IsEmpty())
	{
		// The rules define no values: the server sends no state.
		UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] No server-owned values defined for this API key."));
		return;
	}

	for (const FHorizonPlayerStateValue& Value : State.Values)
	{
		const FString Cap = Value.DailyCap > 0 ? FString::Printf(TEXT("%lld"), Value.DailyCap) : FString(TEXT("no cap"));
		UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample]   %s: balance %lld, requested %lld, credited %lld, today %lld / %s"),
			*Value.Key, Value.Balance, Value.Requested, Value.Credited, Value.EarnedToday, *Cap);

		// A spend is either applied in full or not at all: grant a purchase only when fully credited.
		if (Value.Requested < 0 && !Value.IsFullyCredited())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ValidatedActionsExample]   %s: spend not applied, do not grant the purchase."), *Value.Key);
		}
	}

	// Mirror the server values into the cloud save (display and offline start only).
	const FString Mirror = BuildCloudSaveMirror(State);
	UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Cloud save mirror: %s"), *Mirror);

	UHorizonSubsystem* Horizon = GetHorizon();
	if (bMirrorToCloudSave && Horizon && Horizon->CloudSave)
	{
		TMap<FString, FString> SaveData;
		SaveData.Add(TEXT("validatedState"), Mirror);
		Horizon->CloudSave->SaveObject(SaveData, FOnRequestComplete::CreateLambda(
			[](bool bSaved, const FString& SaveErrorMessage)
			{
				UE_LOG(LogTemp, Log, TEXT("[ValidatedActionsExample] Cloud save mirror %s %s"),
					bSaved ? TEXT("written") : TEXT("FAILED:"), *SaveErrorMessage);
			}));
	}
}

FString AHorizonValidatedActionsExample::BuildCloudSaveMirror(const FHorizonPlayerState& State)
{
	// {"day":"2026-09-29","balances":{"gold":1250}} as a string. Balances are written as JSON
	// numbers; they stay exact up to 9,007,199,254,740,991, the server's upper limit.
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("day"), State.Day);

	TSharedRef<FJsonObject> Balances = MakeShared<FJsonObject>();
	for (const FHorizonPlayerStateValue& Value : State.Values)
	{
		Balances->SetNumberField(Value.Key, static_cast<double>(Value.Balance));
	}
	Root->SetObjectField(TEXT("balances"), Balances);

	FString Serialized;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Serialized);
	FJsonSerializer::Serialize(Root, Writer);
	return Serialized;
}
