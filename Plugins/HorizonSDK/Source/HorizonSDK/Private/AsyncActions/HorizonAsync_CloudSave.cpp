// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

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

UHorizonAsync_CloudSave* UHorizonAsync_CloudSave::SaveObject(const UObject* WorldContextObject, const TMap<FString, FString>& Data)
{
	UHorizonAsync_CloudSave* Action = NewObject<UHorizonAsync_CloudSave>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = ECloudSaveOp::SaveObject;
	Action->ObjectData = Data;
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
	case ECloudSaveOp::SaveObject:
		Subsystem->CloudSave->SaveObject(ObjectData, Callback);
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

// ============================================================
// UHorizonAsync_CloudSaveLoadSnapshot (load with revision)
// ============================================================

UHorizonAsync_CloudSaveLoadSnapshot* UHorizonAsync_CloudSaveLoadSnapshot::LoadDataSnapshot(const UObject* WorldContextObject)
{
	UHorizonAsync_CloudSaveLoadSnapshot* Action = NewObject<UHorizonAsync_CloudSaveLoadSnapshot>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_CloudSaveLoadSnapshot* UHorizonAsync_CloudSaveLoadSnapshot::LoadBytesSnapshot(const UObject* WorldContextObject)
{
	UHorizonAsync_CloudSaveLoadSnapshot* Action = NewObject<UHorizonAsync_CloudSaveLoadSnapshot>();
	Action->WorldContext = WorldContextObject;
	Action->bLoadBytes = true;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CloudSaveLoadSnapshot::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->CloudSave)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CloudSave manager not found."));
		SetReadyToDestroy();
		return;
	}

	FOnCloudSaveSnapshotComplete Callback = FOnCloudSaveSnapshotComplete::CreateUObject(this, &UHorizonAsync_CloudSaveLoadSnapshot::HandleSnapshot);
	if (bLoadBytes)
	{
		Subsystem->CloudSave->LoadBytesSnapshot(Callback);
	}
	else
	{
		Subsystem->CloudSave->LoadSnapshot(Callback);
	}
}

void UHorizonAsync_CloudSaveLoadSnapshot::HandleSnapshot(const FHorizonCloudSaveSnapshot& Snapshot)
{
	if (Snapshot.bSuccess)
	{
		OnSuccess.Broadcast(Snapshot);
	}
	else
	{
		OnFailure.Broadcast(Snapshot.ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// UHorizonAsync_CloudSaveAtRevision (save with If-Match)
// ============================================================

UHorizonAsync_CloudSaveAtRevision* UHorizonAsync_CloudSaveAtRevision::SaveDataAtRevision(const UObject* WorldContextObject, const FString& Data, int64 ExpectedRevision)
{
	UHorizonAsync_CloudSaveAtRevision* Action = NewObject<UHorizonAsync_CloudSaveAtRevision>();
	Action->WorldContext = WorldContextObject;
	Action->StringData = Data;
	Action->TargetRevision = ExpectedRevision;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_CloudSaveAtRevision* UHorizonAsync_CloudSaveAtRevision::SaveBytesAtRevision(const UObject* WorldContextObject, const TArray<uint8>& Data, int64 ExpectedRevision)
{
	UHorizonAsync_CloudSaveAtRevision* Action = NewObject<UHorizonAsync_CloudSaveAtRevision>();
	Action->WorldContext = WorldContextObject;
	Action->bSaveBytes = true;
	Action->BinaryData = Data;
	Action->TargetRevision = ExpectedRevision;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CloudSaveAtRevision::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->CloudSave)
	{
		FHorizonCloudSaveWriteResult Result;
		Result.ErrorMessage = TEXT("horizOn Subsystem or CloudSave manager not found.");
		OnFailure.Broadcast(Result);
		SetReadyToDestroy();
		return;
	}

	FOnCloudSaveWriteComplete Callback = FOnCloudSaveWriteComplete::CreateUObject(this, &UHorizonAsync_CloudSaveAtRevision::HandleWriteResult);
	if (bSaveBytes)
	{
		Subsystem->CloudSave->SaveBytes(BinaryData, TargetRevision, Callback);
	}
	else
	{
		Subsystem->CloudSave->Save(StringData, TargetRevision, Callback);
	}
}

void UHorizonAsync_CloudSaveAtRevision::HandleWriteResult(const FHorizonCloudSaveWriteResult& Result)
{
	switch (Result.Status)
	{
	case EHorizonCloudSaveWriteStatus::Saved:
		OnSaved.Broadcast(Result);
		break;
	case EHorizonCloudSaveWriteStatus::Conflict:
		OnConflict.Broadcast(Result);
		break;
	default:
		OnFailure.Broadcast(Result);
		break;
	}
	SetReadyToDestroy();
}
