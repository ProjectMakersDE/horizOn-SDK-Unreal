// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HelloHorizon/HelloHorizonActor.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonLeaderboardManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

AHelloHorizonActor::AHelloHorizonActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHelloHorizonActor::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		Report(TEXT("ERROR: horizOn subsystem not available. Is the plugin enabled?"));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHelloHorizonActor::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHelloHorizonActor::HandleConnectionFailed);

	Report(TEXT("Connecting..."));
	Horizon->ConnectToServer();
}

void AHelloHorizonActor::HandleConnectionFailed(const FString& ErrorMessage)
{
	Report(FString::Printf(TEXT("Connection failed: %s"), *ErrorMessage));
}

void AHelloHorizonActor::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->Leaderboard)
	{
		Report(TEXT("ERROR: Auth or Leaderboard manager not available."));
		return;
	}

	Report(TEXT("Connected. Signing up..."));

	UHorizonLeaderboardManager* Leaderboard = Horizon->Leaderboard;
	TWeakObjectPtr<AHelloHorizonActor> WeakSelf(this);
	const int64 ScoreToSubmit = Score;

	Horizon->Auth->SignUpAnonymous(DisplayName, FOnAuthComplete::CreateLambda(
		[WeakSelf, Leaderboard, ScoreToSubmit](bool bAuthSuccess)
		{
			AHelloHorizonActor* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}
			if (!bAuthSuccess)
			{
				Self->Report(TEXT("FAILED: anonymous sign-up did not complete."));
				return;
			}

			Self->Report(TEXT("Signed up. Submitting score..."));

			Leaderboard->SubmitScore(ScoreToSubmit, FOnRequestComplete::CreateLambda(
				[WeakSelf](bool bSubmitSuccess, const FString& ErrorMessage)
				{
					AHelloHorizonActor* Inner = WeakSelf.Get();
					if (!Inner)
					{
						return;
					}
					if (!bSubmitSuccess)
					{
						Inner->Report(FString::Printf(TEXT("FAILED to submit score: %s"), *ErrorMessage));
						return;
					}

					Inner->Report(TEXT("Done: score submitted. You are live on horizOn."));
				}));
		}));
}

void AHelloHorizonActor::Report(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("[HelloHorizon] %s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Cyan,
			FString::Printf(TEXT("[HelloHorizon] %s"), *Message));
	}
}
