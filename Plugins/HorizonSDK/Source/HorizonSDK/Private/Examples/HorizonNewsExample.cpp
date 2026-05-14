// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonNewsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonNewsManager.h"
#include "Engine/GameInstance.h"

AHorizonNewsExample::AHorizonNewsExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonNewsExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[NewsExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonNewsExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonNewsExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[NewsExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonNewsExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[NewsExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonNewsExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->News)
	{
		UE_LOG(LogTemp, Error, TEXT("[NewsExample] News manager not available."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NewsExample] Loading up to 5 news entries..."));

	// News does not require a signed-in user.
	Horizon->News->LoadNews(5, TEXT("en"), false, FOnNewsComplete::CreateLambda(
		[](bool bSuccess, const TArray<FHorizonNewsEntry>& Entries)
		{
			if (!bSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[NewsExample] FAILED to load news."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[NewsExample] SUCCESS: %d entries"), Entries.Num());
			for (const FHorizonNewsEntry& Entry : Entries)
			{
				UE_LOG(LogTemp, Log, TEXT("[NewsExample]   - %s"), *Entry.Title);
			}
		}));
}
