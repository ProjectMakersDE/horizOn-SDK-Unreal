// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

UHorizonSubsystem* UHorizonBlueprintLibrary::GetHorizonSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI)
	{
		return nullptr;
	}

	return GI->GetSubsystem<UHorizonSubsystem>();
}

bool UHorizonBlueprintLibrary::IsHorizonConnected(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsConnected() : false;
}

bool UHorizonBlueprintLibrary::IsHorizonSignedIn(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (!Subsystem || !Subsystem->Auth)
	{
		return false;
	}
	return Subsystem->Auth->IsSignedIn();
}

FHorizonUserData UHorizonBlueprintLibrary::GetHorizonCurrentUser(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (!Subsystem || !Subsystem->Auth)
	{
		return FHorizonUserData();
	}
	return Subsystem->Auth->GetCurrentUser();
}
