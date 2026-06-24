// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Localization.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonLocalizationManager.h"

// ============================================================
// GetLocalization (single key)
// ============================================================

UHorizonAsync_LocalizationGet* UHorizonAsync_LocalizationGet::GetLocalization(
	const UObject* WorldContextObject, const FString& Key, const FString& Lang)
{
	UHorizonAsync_LocalizationGet* Action = NewObject<UHorizonAsync_LocalizationGet>();
	Action->WorldContext = WorldContextObject;
	Action->LocalizationKey = Key;
	Action->LanguageCode = Lang;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LocalizationGet::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Localization)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Localization manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Localization->GetLocalization(
		LocalizationKey, LanguageCode,
		FOnLocalizationComplete::CreateUObject(this, &UHorizonAsync_LocalizationGet::HandleResult)
	);
}

void UHorizonAsync_LocalizationGet::HandleResult(bool bSuccess, const FString& Value)
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
// GetAllLocalizations
// ============================================================

UHorizonAsync_LocalizationGetAll* UHorizonAsync_LocalizationGetAll::GetAllLocalizations(
	const UObject* WorldContextObject, const FString& Lang)
{
	UHorizonAsync_LocalizationGetAll* Action = NewObject<UHorizonAsync_LocalizationGetAll>();
	Action->WorldContext = WorldContextObject;
	Action->LanguageCode = Lang;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LocalizationGetAll::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Localization)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Localization manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Localization->GetAllLocalizations(
		LanguageCode,
		FOnAllLocalizationsComplete::CreateUObject(this, &UHorizonAsync_LocalizationGetAll::HandleResult)
	);
}

void UHorizonAsync_LocalizationGetAll::HandleResult(bool bSuccess, const TMap<FString, FString>& Translations)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to retrieve translations."));
	}
	SetReadyToDestroy();
}

// ============================================================
// GetAvailableLanguages
// ============================================================

UHorizonAsync_LocalizationGetLanguages* UHorizonAsync_LocalizationGetLanguages::GetAvailableLanguages(
	const UObject* WorldContextObject)
{
	UHorizonAsync_LocalizationGetLanguages* Action = NewObject<UHorizonAsync_LocalizationGetLanguages>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_LocalizationGetLanguages::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Localization)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Localization manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Localization->GetAvailableLanguages(
		FOnLanguagesComplete::CreateUObject(this, &UHorizonAsync_LocalizationGetLanguages::HandleResult)
	);
}

void UHorizonAsync_LocalizationGetLanguages::HandleResult(bool bSuccess, const TArray<FString>& Languages)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Languages);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to retrieve available languages."));
	}
	SetReadyToDestroy();
}
