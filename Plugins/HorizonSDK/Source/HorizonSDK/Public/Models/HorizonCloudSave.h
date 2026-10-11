// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonCloudSave.generated.h"

/** Outcome of a save at an expected revision. */
UENUM(BlueprintType)
enum class EHorizonCloudSaveWriteStatus : uint8
{
	Saved		UMETA(DisplayName = "Saved"),
	/** Another device saved first (HTTP 409). Nothing was written. */
	Conflict	UMETA(DisplayName = "Conflict"),
	Failed		UMETA(DisplayName = "Failed")
};

/**
 * A loaded cloud save together with its server revision, for saving from several devices.
 * Pass Revision as ExpectedRevision to the next save.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonCloudSaveSnapshot
{
	GENERATED_BODY()

	/** True when the load reached the server and succeeded, also for an empty slot. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	bool bSuccess = false;

	/** True when the slot holds data. False for an empty slot. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	bool bFound = false;

	/** String data, set by LoadSnapshot. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	FString Data;

	/** Binary data, set by LoadBytesSnapshot. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	TArray<uint8> BinaryData;

	/**
	 * Server revision: 0 for an empty slot. -1 when the server sent none (simpleServer):
	 * the state is unknown, not empty.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	int64 Revision = -1;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	FString ErrorMessage;

	/** HTTP status code, 0 when the request failed locally or did not connect. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	int32 StatusCode = 0;

	bool HasRevision() const { return Revision >= 0; }
};

/** Result of a save at an expected revision. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonCloudSaveWriteResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	EHorizonCloudSaveWriteStatus Status = EHorizonCloudSaveWriteStatus::Failed;

	/** New revision after Saved, the ExpectedRevision for the next save. -1 when the server sent none. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	int64 Revision = -1;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	int64 DataSizeBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	FString ErrorMessage;

	/** HTTP status code, 0 when the request failed locally or did not connect. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|CloudSave")
	int32 StatusCode = 0;

	bool IsSaved() const { return Status == EHorizonCloudSaveWriteStatus::Saved; }
	bool IsConflict() const { return Status == EHorizonCloudSaveWriteStatus::Conflict; }
};

DECLARE_DELEGATE_OneParam(FOnCloudSaveSnapshotComplete, const FHorizonCloudSaveSnapshot& /*Snapshot*/);
DECLARE_DELEGATE_OneParam(FOnCloudSaveWriteComplete, const FHorizonCloudSaveWriteResult& /*Result*/);
