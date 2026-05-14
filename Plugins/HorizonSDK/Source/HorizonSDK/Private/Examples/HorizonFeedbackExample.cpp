// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonFeedbackExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonFeedbackManager.h"
#include "Engine/GameInstance.h"

AHorizonFeedbackExample::AHorizonFeedbackExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonFeedbackExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[FeedbackExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonFeedbackExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonFeedbackExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[FeedbackExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonFeedbackExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[FeedbackExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonFeedbackExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Feedback)
	{
		UE_LOG(LogTemp, Error, TEXT("[FeedbackExample] Feedback manager not available."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[FeedbackExample] Submitting general feedback..."));

	// Feedback does not require a signed-in user; the user ID is attached
	// automatically when one is available.
	Horizon->Feedback->SendGeneral(
		TEXT("Loving the game"),
		TEXT("Sent from the horizOn feedback example actor."),
		FOnRequestComplete::CreateLambda(
			[](bool bSuccess, const FString& ErrorMessage)
			{
				if (!bSuccess)
				{
					UE_LOG(LogTemp, Error, TEXT("[FeedbackExample] FAILED to submit feedback: %s"), *ErrorMessage);
					return;
				}

				UE_LOG(LogTemp, Log, TEXT("[FeedbackExample] SUCCESS: feedback submitted"));
			}));
}
