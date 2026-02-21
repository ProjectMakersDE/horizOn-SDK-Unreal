// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_RemoteConfig.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoteConfigValueResult, const FString&, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRemoteConfigAllSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoteConfigAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Get a single remote config value by key.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_RemoteConfigGet : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnRemoteConfigValueResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnRemoteConfigAsyncFailure OnFailure;

	/** Get a single config value by key. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Remote Config"), Category = "horizOn|RemoteConfig")
	static UHorizonAsync_RemoteConfigGet* GetConfig(const UObject* WorldContextObject, const FString& Key, bool bUseCache);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString ConfigKey;
	bool bCache;

	void HandleResult(bool bSuccess, const FString& Value);
};

// ============================================================

/**
 * Async Blueprint node: Fetch all remote config values.
 *
 * After success, call GetAllConfigs on the RemoteConfig manager
 * to access the cached values, or use GetConfig for individual keys.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_RemoteConfigGetAll : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnRemoteConfigAllSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnRemoteConfigAsyncFailure OnFailure;

	/** Fetch all remote config key-value pairs. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get All Remote Configs"), Category = "horizOn|RemoteConfig")
	static UHorizonAsync_RemoteConfigGetAll* GetAllConfigs(const UObject* WorldContextObject, bool bUseCache);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	bool bCache;

	void HandleResult(bool bSuccess, const TMap<FString, FString>& Configs);
};
