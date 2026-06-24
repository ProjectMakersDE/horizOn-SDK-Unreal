// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonLocalizationExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonLocalizationManager.h"
#include "Engine/GameInstance.h"

AHorizonLocalizationExample::AHorizonLocalizationExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonLocalizationExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[LocalizationExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonLocalizationExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonLocalizationExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[LocalizationExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonLocalizationExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[LocalizationExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonLocalizationExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Localization)
	{
		UE_LOG(LogTemp, Error, TEXT("[LocalizationExample] Localization manager not available."));
		return;
	}

	UHorizonLocalizationManager* Localization = Horizon->Localization;

	// Localization does not require a signed-in user.
	// First, read one value for the active language (empty Lang uses CurrentLanguage).
	Localization->GetLocalization(TEXT("welcome_message"), TEXT(""), FOnLocalizationComplete::CreateLambda(
		[Localization](bool bSuccess, const FString& Value)
		{
			if (!bSuccess)
			{
				UE_LOG(LogTemp, Warning, TEXT("[LocalizationExample] welcome_message lookup failed."));
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[LocalizationExample] welcome_message = %s"), *Value);
			}

			// Then fetch the full translation map for the active language.
			Localization->GetAllLocalizations(TEXT(""), FOnAllLocalizationsComplete::CreateLambda(
				[](bool bAllSuccess, const TMap<FString, FString>& Translations)
				{
					if (!bAllSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[LocalizationExample] FAILED to fetch all translations."));
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[LocalizationExample] SUCCESS: %d translation entries"), Translations.Num());
					for (const TPair<FString, FString>& Pair : Translations)
					{
						UE_LOG(LogTemp, Log, TEXT("[LocalizationExample]   %s = %s"), *Pair.Key, *Pair.Value);
					}
				}));
		}));
}
