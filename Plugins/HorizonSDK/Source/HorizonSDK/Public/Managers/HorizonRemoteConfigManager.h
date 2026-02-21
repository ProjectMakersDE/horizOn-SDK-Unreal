// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonRemoteConfigManager.generated.h"

DECLARE_DELEGATE_TwoParams(FOnConfigComplete, bool /*bSuccess*/, const FString& /*Value*/);
DECLARE_DELEGATE_TwoParams(FOnAllConfigsComplete, bool /*bSuccess*/, const TMap<FString, FString>& /*Configs*/);

/**
 * Remote Config Manager for the horizOn SDK.
 *
 * Provides typed config value retrieval with in-memory caching.
 * Does NOT require a session token.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonRemoteConfigManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client reference. */
	void Initialize(UHorizonHttpClient* InHttpClient);

	/**
	 * Get a single config value by key.
	 * @param Key        Config key to look up.
	 * @param bUseCache  If true, return cached value when available.
	 * @param OnComplete Called with (bSuccess, Value).
	 */
	void GetConfig(const FString& Key, bool bUseCache, FOnConfigComplete OnComplete);

	/**
	 * Get all remote config key-value pairs.
	 * @param bUseCache  If true, return cached data when available.
	 * @param OnComplete Called with (bSuccess, Configs).
	 */
	void GetAllConfigs(bool bUseCache, FOnAllConfigsComplete OnComplete);

	/**
	 * Get a config value as string with a default fallback.
	 * @param Key          Config key.
	 * @param DefaultValue Returned if the key is not found.
	 * @param bUseCache    If true, return cached value when available.
	 * @param OnComplete   Called with (bSuccess, Value).
	 */
	void GetString(const FString& Key, const FString& DefaultValue, bool bUseCache, FOnConfigComplete OnComplete);

	/**
	 * Get a config value as int32 with a default fallback.
	 * @param Key          Config key.
	 * @param DefaultValue Returned if the key is not found or cannot be parsed.
	 * @param bUseCache    If true, return cached value when available.
	 * @param OnComplete   Called with (bSuccess, Value).
	 */
	void GetInt(const FString& Key, int32 DefaultValue, bool bUseCache, TDelegate<void(bool, int32)> OnComplete);

	/**
	 * Get a config value as float with a default fallback.
	 * @param Key          Config key.
	 * @param DefaultValue Returned if the key is not found or cannot be parsed.
	 * @param bUseCache    If true, return cached value when available.
	 * @param OnComplete   Called with (bSuccess, Value).
	 */
	void GetFloat(const FString& Key, float DefaultValue, bool bUseCache, TDelegate<void(bool, float)> OnComplete);

	/**
	 * Get a config value as bool with a default fallback.
	 * @param Key          Config key.
	 * @param DefaultValue Returned if the key is not found or cannot be parsed.
	 * @param bUseCache    If true, return cached value when available.
	 * @param OnComplete   Called with (bSuccess, Value).
	 */
	void GetBool(const FString& Key, bool DefaultValue, bool bUseCache, TDelegate<void(bool, bool)> OnComplete);

	/** Clear the in-memory config cache. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|RemoteConfig")
	void ClearCache();

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	/** In-memory cache of config values (key -> value). */
	TMap<FString, FString> ConfigCache;

	/** Whether the full config set has been fetched at least once. */
	bool bAllConfigsCached = false;
};
