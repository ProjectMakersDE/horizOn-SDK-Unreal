// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_CloudSave.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonCloudSaveManager.h"

// ============================================================
// UHorizonAsync_CloudSave (Save / SaveBytes)
// ============================================================

UHorizonAsync_CloudSave* UHorizonAsync_CloudSave::SaveData(const UObject* WorldContextObject, const FString& Data)
{
	UHorizonAsync_CloudSave* Action = NewObject<UHorizonAsync_CloudSave>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = ECloudSaveOp::SaveString;
	Action->StringData = Data;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_CloudSave* UHorizonAsync_CloudSave::SaveBytes(const UObject* WorldContextObject, const TArray<uint8>& Data)
{
	UHorizonAsync_CloudSave* Action = NewObject<UHorizonAsync_CloudSave>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = ECloudSaveOp::SaveBytes;
	Action->BinaryData = Data;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CloudSave::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->CloudSave)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CloudSave manager not found."));
		SetReadyToDestroy();
		return;
	}

	FOnRequestComplete Callback = FOnRequestComplete::CreateUObject(this, &UHorizonAsync_CloudSave::HandleSaveResult);

	switch (Operation)
	{
	case ECloudSaveOp::SaveString:
		Subsystem->CloudSave->Save(StringData, Callback);
		break;
	case ECloudSaveOp::SaveBytes:
		Subsystem->CloudSave->SaveBytes(BinaryData, Callback);
		break;
	}
}

void UHorizonAsync_CloudSave::HandleSaveResult(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast(ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// UHorizonAsync_CloudSaveLoad (Load string)
// ============================================================

UHorizonAsync_CloudSaveLoad* UHorizonAsync_CloudSaveLoad::LoadData(const UObject* WorldContextObject)
{
	UHorizonAsync_CloudSaveLoad* Action = NewObject<UHorizonAsync_CloudSaveLoad>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CloudSaveLoad::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->CloudSave)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CloudSave manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->CloudSave->Load(FOnStringComplete::CreateUObject(this, &UHorizonAsync_CloudSaveLoad::HandleLoadResult));
}

void UHorizonAsync_CloudSaveLoad::HandleLoadResult(bool bSuccess, const FString& Data)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Data);
	}
	else
	{
		OnFailure.Broadcast(Data); // Data contains the error message on failure
	}
	SetReadyToDestroy();
}

// ============================================================
// UHorizonAsync_CloudSaveLoadBytes (Load binary)
// ============================================================

UHorizonAsync_CloudSaveLoadBytes* UHorizonAsync_CloudSaveLoadBytes::LoadBytes(const UObject* WorldContextObject)
{
	UHorizonAsync_CloudSaveLoadBytes* Action = NewObject<UHorizonAsync_CloudSaveLoadBytes>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CloudSaveLoadBytes::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->CloudSave)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CloudSave manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->CloudSave->LoadBytes(FOnBinaryComplete::CreateUObject(this, &UHorizonAsync_CloudSaveLoadBytes::HandleLoadBytesResult));
}

void UHorizonAsync_CloudSaveLoadBytes::HandleLoadBytesResult(bool bSuccess, const TArray<uint8>& Data)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Data);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to load binary data."));
	}
	SetReadyToDestroy();
}
