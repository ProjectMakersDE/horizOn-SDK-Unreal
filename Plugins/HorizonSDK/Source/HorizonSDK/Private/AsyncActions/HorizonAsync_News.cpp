// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_News.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonNewsManager.h"

UHorizonAsync_News* UHorizonAsync_News::LoadNews(
	const UObject* WorldContextObject, int32 Limit, const FString& LanguageCode, bool bUseCache)
{
	UHorizonAsync_News* Action = NewObject<UHorizonAsync_News>();
	Action->WorldContext = WorldContextObject;
	Action->LimitValue = Limit;
	Action->Language = LanguageCode;
	Action->bCache = bUseCache;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_News::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->News)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or News manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->News->LoadNews(
		LimitValue, Language, bCache,
		FOnNewsComplete::CreateUObject(this, &UHorizonAsync_News::HandleResult)
	);
}

void UHorizonAsync_News::HandleResult(bool bSuccess, const TArray<FHorizonNewsEntry>& Entries)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Entries);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to load news entries."));
	}
	SetReadyToDestroy();
}
