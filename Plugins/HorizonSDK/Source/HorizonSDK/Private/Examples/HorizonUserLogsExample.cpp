// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonUserLogsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonUserLogManager.h"
#include "Engine/GameInstance.h"

AHorizonUserLogsExample::AHorizonUserLogsExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonUserLogsExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[UserLogsExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonUserLogsExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonUserLogsExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[UserLogsExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonUserLogsExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[UserLogsExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonUserLogsExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->UserLogs)
	{
		UE_LOG(LogTemp, Error, TEXT("[UserLogsExample] Auth or UserLogs manager not available."));
		return;
	}

	UHorizonUserLogManager* UserLogs = Horizon->UserLogs;

	// User logs require a signed-in user.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[UserLogs](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[UserLogsExample] FAILED: sign-up did not complete."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[UserLogsExample] Sending an INFO log entry..."));

			UserLogs->Info(TEXT("Example actor reached HandleConnected"), FOnUserLogComplete::CreateLambda(
				[](bool bSuccess, const FString& LogId, const FString& CreatedAt)
				{
					if (!bSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[UserLogsExample] FAILED to create log entry."));
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[UserLogsExample] SUCCESS: log %s created at %s"),
						*LogId, *CreatedAt);
				}));
		}));
}
