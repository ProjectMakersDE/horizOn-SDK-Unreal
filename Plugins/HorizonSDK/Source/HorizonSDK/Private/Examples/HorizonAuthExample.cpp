// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonAuthExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Engine/GameInstance.h"

AHorizonAuthExample::AHorizonAuthExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonAuthExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[AuthExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonAuthExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonAuthExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[AuthExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonAuthExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[AuthExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonAuthExample::HandleConnected()
{
	UE_LOG(LogTemp, Log, TEXT("[AuthExample] Connected. Signing up anonymously..."));

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth)
	{
		UE_LOG(LogTemp, Error, TEXT("[AuthExample] Auth manager not available."));
		return;
	}

	UHorizonAuthManager* Auth = Horizon->Auth;
	Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[Auth](bool bSuccess)
		{
			if (!bSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[AuthExample] FAILED: anonymous sign-up did not complete."));
				return;
			}

			const FHorizonUserData User = Auth->GetCurrentUser();
			UE_LOG(LogTemp, Log, TEXT("[AuthExample] SUCCESS: signed in as %s (UserId %s)"),
				*User.DisplayName, *User.UserId);
		}));
}
