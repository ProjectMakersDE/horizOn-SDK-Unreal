// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonCloudSave.h"
#include "HorizonAsync_CloudSave.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCloudSaveAsyncSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveAsyncFailure, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveDataLoaded, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveBytesLoaded, const TArray<uint8>&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveSnapshotAsyncLoaded, const FHorizonCloudSaveSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveWriteAsyncResult, const FHorizonCloudSaveWriteResult&, Result);

/**
 * Async Blueprint nodes for horizOn Cloud Save operations.
 *
 * Supports saving/loading both string (JSON) and binary data.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CloudSave : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// --- Save / SaveBytes ---

	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveAsyncFailure OnFailure;

	/** Save string data to the cloud. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Save Data"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSave* SaveData(const UObject* WorldContextObject, const FString& Data);

	/** Save string dictionary data to the cloud as a JSON object. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Save Object"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSave* SaveObject(const UObject* WorldContextObject, const TMap<FString, FString>& Data);

	/** Save binary data to the cloud. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Save Bytes"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSave* SaveBytes(const UObject* WorldContextObject, const TArray<uint8>& Data);

	virtual void Activate() override;

private:
	enum class ECloudSaveOp : uint8
	{
		SaveString,
		SaveObject,
		SaveBytes
	};

	TWeakObjectPtr<const UObject> WorldContext;
	ECloudSaveOp Operation;
	FString StringData;
	TMap<FString, FString> ObjectData;
	TArray<uint8> BinaryData;

	void HandleSaveResult(bool bSuccess, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node for loading string data from cloud save.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CloudSaveLoad : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveDataLoaded OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveAsyncFailure OnFailure;

	/** Load string data from the cloud. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Load Data"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSaveLoad* LoadData(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;

	void HandleLoadResult(bool bSuccess, const FString& Data);
};

// ============================================================

/**
 * Async Blueprint node for loading binary data from cloud save.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CloudSaveLoadBytes : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveBytesLoaded OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveAsyncFailure OnFailure;

	/** Load binary data from the cloud. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Load Bytes"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSaveLoadBytes* LoadBytes(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;

	void HandleLoadBytesResult(bool bSuccess, const TArray<uint8>& Data);
};

// ============================================================

/**
 * Async Blueprint nodes for loading a cloud save together with its server revision,
 * for saving from several devices. Pass Snapshot.Revision to "Cloud Save ... At Revision".
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CloudSaveLoadSnapshot : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Also fires for an empty slot: bFound false, Revision 0. */
	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveSnapshotAsyncLoaded OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveAsyncFailure OnFailure;

	/** Load string data with its revision. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Load Data Snapshot"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSaveLoadSnapshot* LoadDataSnapshot(const UObject* WorldContextObject);

	/** Load binary data with its revision. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Load Bytes Snapshot"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSaveLoadSnapshot* LoadBytesSnapshot(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	bool bLoadBytes = false;

	void HandleSnapshot(const FHorizonCloudSaveSnapshot& Snapshot);
};

// ============================================================

/**
 * Async Blueprint nodes for saving only while the cloud save is still at the expected
 * revision (If-Match). On a conflict nothing was written: load a new snapshot and decide.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CloudSaveAtRevision : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Written. Result.Revision is the expected revision for the next save. */
	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveWriteAsyncResult OnSaved;

	/** Another device saved first (HTTP 409). Nothing was written. */
	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveWriteAsyncResult OnConflict;

	UPROPERTY(BlueprintAssignable)
	FOnCloudSaveWriteAsyncResult OnFailure;

	/** Save string data if the cloud save is still at ExpectedRevision (0 for an empty slot). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Save Data At Revision"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSaveAtRevision* SaveDataAtRevision(const UObject* WorldContextObject, const FString& Data, int64 ExpectedRevision);

	/** Save binary data if the cloud save is still at ExpectedRevision (0 for an empty slot). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Save Bytes At Revision"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSaveAtRevision* SaveBytesAtRevision(const UObject* WorldContextObject, const TArray<uint8>& Data, int64 ExpectedRevision);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	bool bSaveBytes = false;
	FString StringData;
	TArray<uint8> BinaryData;
	int64 TargetRevision = 0;

	void HandleWriteResult(const FHorizonCloudSaveWriteResult& Result);
};
