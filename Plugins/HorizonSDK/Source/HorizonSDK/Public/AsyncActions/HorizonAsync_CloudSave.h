// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_CloudSave.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCloudSaveAsyncSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveAsyncFailure, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveDataLoaded, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloudSaveBytesLoaded, const TArray<uint8>&, Data);

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

	/** Save binary data to the cloud. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cloud Save Bytes"), Category = "horizOn|CloudSave")
	static UHorizonAsync_CloudSave* SaveBytes(const UObject* WorldContextObject, const TArray<uint8>& Data);

	virtual void Activate() override;

private:
	enum class ECloudSaveOp : uint8
	{
		SaveString,
		SaveBytes
	};

	TWeakObjectPtr<const UObject> WorldContext;
	ECloudSaveOp Operation;
	FString StringData;
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
