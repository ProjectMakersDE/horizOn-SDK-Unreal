// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
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

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;
};
