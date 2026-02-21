// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_RemoteConfig.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonRemoteConfigManager.h"

// ============================================================
// GetConfig (single key)
// ============================================================

UHorizonAsync_RemoteConfigGet* UHorizonAsync_RemoteConfigGet::GetConfig(
	const UObject* WorldContextObject, const FString& Key, bool bUseCache)
{
	UHorizonAsync_RemoteConfigGet* Action = NewObject<UHorizonAsync_RemoteConfigGet>();
	Action->WorldContext = WorldContextObject;
	Action->ConfigKey = Key;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_RemoteConfigGet::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->RemoteConfig)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or RemoteConfig manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->RemoteConfig->GetConfig(
		ConfigKey, bCache,
		FOnConfigComplete::CreateUObject(this, &UHorizonAsync_RemoteConfigGet::HandleResult)
	);
}

void UHorizonAsync_RemoteConfigGet::HandleResult(bool bSuccess, const FString& Value)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Value);
	}
	else
	{
		OnFailure.Broadcast(Value); // Value contains error message on failure
	}
	SetReadyToDestroy();
}

// ============================================================
// GetAllConfigs
// ============================================================

UHorizonAsync_RemoteConfigGetAll* UHorizonAsync_RemoteConfigGetAll::GetAllConfigs(
	const UObject* WorldContextObject, bool bUseCache)
{
	UHorizonAsync_RemoteConfigGetAll* Action = NewObject<UHorizonAsync_RemoteConfigGetAll>();
	Action->WorldContext = WorldContextObject;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_RemoteConfigGetAll::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->RemoteConfig)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or RemoteConfig manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->RemoteConfig->GetAllConfigs(
		bCache,
		FOnAllConfigsComplete::CreateUObject(this, &UHorizonAsync_RemoteConfigGetAll::HandleResult)
	);
}

void UHorizonAsync_RemoteConfigGetAll::HandleResult(bool bSuccess, const TMap<FString, FString>& Configs)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to retrieve remote configs."));
	}
	SetReadyToDestroy();
}
