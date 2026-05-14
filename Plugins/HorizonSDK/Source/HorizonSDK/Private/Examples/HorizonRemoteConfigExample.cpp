// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonRemoteConfigExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonRemoteConfigManager.h"
#include "Engine/GameInstance.h"

AHorizonRemoteConfigExample::AHorizonRemoteConfigExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonRemoteConfigExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[RemoteConfigExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonRemoteConfigExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonRemoteConfigExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[RemoteConfigExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonRemoteConfigExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[RemoteConfigExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonRemoteConfigExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->RemoteConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("[RemoteConfigExample] RemoteConfig manager not available."));
		return;
	}

	UHorizonRemoteConfigManager* RemoteConfig = Horizon->RemoteConfig;

	// Remote config does not require a signed-in user.
	// First, read one value with a default fallback.
	RemoteConfig->GetString(TEXT("welcome_message"), TEXT("Welcome!"), false, FOnConfigComplete::CreateLambda(
		[RemoteConfig](bool bSuccess, const FString& Value)
		{
			if (!bSuccess)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RemoteConfigExample] welcome_message lookup failed, used default: %s"), *Value);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[RemoteConfigExample] welcome_message = %s"), *Value);
			}

			// Then fetch the full config map.
			RemoteConfig->GetAllConfigs(false, FOnAllConfigsComplete::CreateLambda(
				[](bool bAllSuccess, const TMap<FString, FString>& Configs)
				{
					if (!bAllSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[RemoteConfigExample] FAILED to fetch all configs."));
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[RemoteConfigExample] SUCCESS: %d config entries"), Configs.Num());
					for (const TPair<FString, FString>& Pair : Configs)
					{
						UE_LOG(LogTemp, Log, TEXT("[RemoteConfigExample]   %s = %s"), *Pair.Key, *Pair.Value);
					}
				}));
		}));
}
