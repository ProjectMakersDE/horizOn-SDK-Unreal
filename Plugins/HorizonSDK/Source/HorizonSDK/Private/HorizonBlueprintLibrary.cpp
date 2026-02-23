// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonCrashManager.h"
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

void UHorizonBlueprintLibrary::HorizonStartCrashCapture(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->StartCapture();
	}
}

void UHorizonBlueprintLibrary::HorizonStopCrashCapture(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->StopCapture();
	}
}

void UHorizonBlueprintLibrary::HorizonRecordBreadcrumb(const UObject* WorldContextObject, const FString& Type, const FString& Message)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->RecordBreadcrumb(Type, Message);
	}
}

void UHorizonBlueprintLibrary::HorizonSetCrashCustomKey(const UObject* WorldContextObject, const FString& Key, const FString& Value)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->SetCustomKey(Key, Value);
	}
}
