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
// GetJson
// ============================================================

UHorizonAsync_RemoteConfigGetJson* UHorizonAsync_RemoteConfigGetJson::GetJson(
	const UObject* WorldContextObject, const FString& Key, const FString& DefaultValue, bool bUseCache)
{
	UHorizonAsync_RemoteConfigGetJson* Action = NewObject<UHorizonAsync_RemoteConfigGetJson>();
	Action->WorldContext = WorldContextObject;
	Action->ConfigKey = Key;
	Action->DefaultValueStr = DefaultValue;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_RemoteConfigGetJson::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->RemoteConfig)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or RemoteConfig manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->RemoteConfig->GetJson(
		ConfigKey, DefaultValueStr, bCache,
		FOnConfigComplete::CreateUObject(this, &UHorizonAsync_RemoteConfigGetJson::HandleResult)
	);
}

void UHorizonAsync_RemoteConfigGetJson::HandleResult(bool bSuccess, const FString& Value)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Value);
	}
	else
	{
		OnFailure.Broadcast(Value);
	}
	SetReadyToDestroy();
}

// ============================================================
// HasKey
// ============================================================

UHorizonAsync_RemoteConfigHasKey* UHorizonAsync_RemoteConfigHasKey::HasKey(
	const UObject* WorldContextObject, const FString& Key, bool bUseCache)
{
	UHorizonAsync_RemoteConfigHasKey* Action = NewObject<UHorizonAsync_RemoteConfigHasKey>();
	Action->WorldContext = WorldContextObject;
	Action->ConfigKey = Key;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_RemoteConfigHasKey::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->RemoteConfig)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or RemoteConfig manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->RemoteConfig->HasKey(
		ConfigKey, bCache,
		FOnConfigExistsComplete::CreateUObject(this, &UHorizonAsync_RemoteConfigHasKey::HandleResult)
	);
}

void UHorizonAsync_RemoteConfigHasKey::HandleResult(bool bSuccess, bool bExists)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(bExists);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to check remote config key."));
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
