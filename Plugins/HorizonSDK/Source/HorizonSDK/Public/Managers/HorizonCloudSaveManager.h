// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonCloudSave.h"
#include "HorizonCloudSaveManager.generated.h"

/**
 * Cloud Save Manager for the horizOn SDK.
 *
 * Provides save/load operations in both JSON (string) and binary modes.
 * All operations require the user to be signed in.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonCloudSaveManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/**
	 * Save string data to the cloud for the current user.
	 * @param Data       The string data to save.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 */
	void Save(const FString& Data, FOnRequestComplete OnComplete);

	/**
	 * Save a string dictionary to the cloud for the current user.
	 * The dictionary is serialized as a JSON object and stored via the string save endpoint.
	 * @param Data       Key-value data to save.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 */
	void SaveObject(const TMap<FString, FString>& Data, FOnRequestComplete OnComplete);

	/**
	 * Load string data from the cloud for the current user.
	 * @param OnComplete Called with (bSuccess, Data).
	 */
	void Load(FOnStringComplete OnComplete);

	/**
	 * Save binary data to the cloud for the current user.
	 * @param Data       The binary data to save.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 */
	void SaveBytes(const TArray<uint8>& Data, FOnRequestComplete OnComplete);

	/**
	 * Load binary data from the cloud for the current user.
	 * @param OnComplete Called with (bSuccess, Data).
	 */
	void LoadBytes(FOnBinaryComplete OnComplete);

	// -- Several devices: revision-checked saves --

	/**
	 * Load the string save together with its server revision.
	 * Pass Snapshot.Revision as ExpectedRevision to Save(Data, ExpectedRevision, OnComplete).
	 * bFound is false and Revision 0 for an empty slot. Revision is -1 when the server sent
	 * none: the state is unknown, not empty.
	 */
	void LoadSnapshot(FOnCloudSaveSnapshotComplete OnComplete);

	/**
	 * Load the binary save together with its server revision.
	 * Pass Snapshot.Revision as ExpectedRevision to SaveBytes(Data, ExpectedRevision, OnComplete).
	 */
	void LoadBytesSnapshot(FOnCloudSaveSnapshotComplete OnComplete);

	/**
	 * Save string data only if the cloud save is still at ExpectedRevision (sends If-Match).
	 * When another device saved first, nothing is written and the result is Conflict:
	 * load a new snapshot and decide. A conflict is never retried or overwritten by the SDK.
	 * @param ExpectedRevision Revision from the last snapshot or save, 0 for an empty slot.
	 */
	void Save(const FString& Data, int64 ExpectedRevision, FOnCloudSaveWriteComplete OnComplete);

	/** Save binary data only if the cloud save is still at ExpectedRevision. See Save. */
	void SaveBytes(const TArray<uint8>& Data, int64 ExpectedRevision, FOnCloudSaveWriteComplete OnComplete);

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;
};
