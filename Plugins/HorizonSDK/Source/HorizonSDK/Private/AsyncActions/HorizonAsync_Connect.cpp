// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Connect.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"

UHorizonAsync_Connect* UHorizonAsync_Connect::ConnectToServer(const UObject* WorldContextObject)
{
	UHorizonAsync_Connect* Action = NewObject<UHorizonAsync_Connect>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_Connect::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->OnConnected.AddDynamic(this, &UHorizonAsync_Connect::HandleSuccess);
	Subsystem->OnConnectionFailed.AddDynamic(this, &UHorizonAsync_Connect::HandleFailure);

	Subsystem->ConnectToServer();
}

void UHorizonAsync_Connect::HandleSuccess()
{
	OnSuccess.Broadcast();
	Cleanup();
}

void UHorizonAsync_Connect::HandleFailure(const FString& ErrorMessage)
{
	OnFailure.Broadcast(ErrorMessage);
	Cleanup();
}

void UHorizonAsync_Connect::Cleanup()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (Subsystem)
	{
		Subsystem->OnConnected.RemoveDynamic(this, &UHorizonAsync_Connect::HandleSuccess);
		Subsystem->OnConnectionFailed.RemoveDynamic(this, &UHorizonAsync_Connect::HandleFailure);
	}
	SetReadyToDestroy();
}
