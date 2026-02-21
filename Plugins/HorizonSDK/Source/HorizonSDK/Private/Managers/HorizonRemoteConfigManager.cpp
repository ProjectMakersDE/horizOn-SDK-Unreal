// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonRemoteConfigManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonRemoteConfigManager::Initialize(UHorizonHttpClient* InHttpClient)
{
	HttpClient = InHttpClient;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonRemoteConfigManager initialized."));
}

// ============================================================
// GetConfig
// ============================================================

void UHorizonRemoteConfigManager::GetConfig(const FString& Key, bool bUseCache, FOnConfigComplete OnComplete)
{
	if (bUseCache)
	{
		const FString* Cached = ConfigCache.Find(Key);
		if (Cached)
		{
			UE_LOG(LogHorizonSDK, Verbose, TEXT("RemoteConfig::GetConfig -- Returning cached value for key '%s'."), *Key);
			OnComplete.ExecuteIfBound(true, *Cached);
			return;
		}
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/remote-config/%s"), *Key);

	TWeakObjectPtr<UHorizonRemoteConfigManager> WeakSelf(this);
	FOnConfigComplete CapturedOnComplete = OnComplete;
	FString CapturedKey = Key;

	HttpClient->Get(Endpoint, false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, CapturedKey](const FHorizonNetworkResponse& Response)
			{
				UHorizonRemoteConfigManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("RemoteConfig::GetConfig -- Failed for key '%s': %s"), *CapturedKey, *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
					return;
				}

				bool bFound = false;
				FString Value;

				if (Response.JsonData.IsValid())
				{
					bFound = Response.JsonData->GetBoolField(TEXT("found"));
					if (bFound)
					{
						Value = Response.JsonData->GetStringField(TEXT("configValue"));
						Self->ConfigCache.Add(CapturedKey, Value);
					}
				}

				if (bFound)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("RemoteConfig::GetConfig -- Key '%s' = '%s'"), *CapturedKey, *Value);
					CapturedOnComplete.ExecuteIfBound(true, Value);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("RemoteConfig::GetConfig -- Key '%s' not found."), *CapturedKey);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
				}
			}
		));
}

// ============================================================
// GetAllConfigs
// ============================================================

void UHorizonRemoteConfigManager::GetAllConfigs(bool bUseCache, FOnAllConfigsComplete OnComplete)
{
	if (bUseCache && bAllConfigsCached)
	{
		UE_LOG(LogHorizonSDK, Verbose, TEXT("RemoteConfig::GetAllConfigs -- Returning cached configs (%d entries)."), ConfigCache.Num());
		OnComplete.ExecuteIfBound(true, ConfigCache);
		return;
	}

	TWeakObjectPtr<UHorizonRemoteConfigManager> WeakSelf(this);
	FOnAllConfigsComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(TEXT("api/v1/app/remote-config/all"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				UHorizonRemoteConfigManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("RemoteConfig::GetAllConfigs -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TMap<FString, FString>());
					return;
				}

				TMap<FString, FString> Configs;

				if (Response.JsonData.IsValid() && Response.JsonData->HasField(TEXT("configs")))
				{
					const TSharedPtr<FJsonObject> ConfigsObj = Response.JsonData->GetObjectField(TEXT("configs"));
					if (ConfigsObj.IsValid())
					{
						for (const auto& Pair : ConfigsObj->Values)
						{
							FString Value;
							if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
							{
								Configs.Add(Pair.Key, Value);
							}
						}
					}
				}

				// Update cache with all fetched configs
				Self->ConfigCache = Configs;
				Self->bAllConfigsCached = true;

				UE_LOG(LogHorizonSDK, Log, TEXT("RemoteConfig::GetAllConfigs -- Retrieved %d configs."), Configs.Num());
				CapturedOnComplete.ExecuteIfBound(true, Configs);
			}
		));
}

// ============================================================
// Typed Getters
// ============================================================

void UHorizonRemoteConfigManager::GetString(const FString& Key, const FString& DefaultValue, bool bUseCache, FOnConfigComplete OnComplete)
{
	FOnConfigComplete CapturedOnComplete = OnComplete;
	FString CapturedDefault = DefaultValue;

	GetConfig(Key, bUseCache, FOnConfigComplete::CreateLambda(
		[CapturedOnComplete, CapturedDefault](bool bSuccess, const FString& Value)
		{
			if (bSuccess && !Value.IsEmpty())
			{
				CapturedOnComplete.ExecuteIfBound(true, Value);
			}
			else
			{
				CapturedOnComplete.ExecuteIfBound(bSuccess, CapturedDefault);
			}
		}
	));
}

void UHorizonRemoteConfigManager::GetInt(const FString& Key, int32 DefaultValue, bool bUseCache, TDelegate<void(bool, int32)> OnComplete)
{
	TDelegate<void(bool, int32)> CapturedOnComplete = OnComplete;
	int32 CapturedDefault = DefaultValue;

	GetConfig(Key, bUseCache, FOnConfigComplete::CreateLambda(
		[CapturedOnComplete, CapturedDefault](bool bSuccess, const FString& Value)
		{
			if (bSuccess && !Value.IsEmpty())
			{
				if (Value.IsNumeric())
				{
					CapturedOnComplete.ExecuteIfBound(true, FCString::Atoi(*Value));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("RemoteConfig::GetInt -- Value '%s' is not a valid integer. Using default."), *Value);
					CapturedOnComplete.ExecuteIfBound(true, CapturedDefault);
				}
			}
			else
			{
				CapturedOnComplete.ExecuteIfBound(bSuccess, CapturedDefault);
			}
		}
	));
}

void UHorizonRemoteConfigManager::GetFloat(const FString& Key, float DefaultValue, bool bUseCache, TDelegate<void(bool, float)> OnComplete)
{
	TDelegate<void(bool, float)> CapturedOnComplete = OnComplete;
	float CapturedDefault = DefaultValue;

	GetConfig(Key, bUseCache, FOnConfigComplete::CreateLambda(
		[CapturedOnComplete, CapturedDefault](bool bSuccess, const FString& Value)
		{
			if (bSuccess && !Value.IsEmpty())
			{
				if (Value.IsNumeric())
				{
					CapturedOnComplete.ExecuteIfBound(true, FCString::Atof(*Value));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("RemoteConfig::GetFloat -- Value '%s' is not a valid number. Using default."), *Value);
					CapturedOnComplete.ExecuteIfBound(true, CapturedDefault);
				}
			}
			else
			{
				CapturedOnComplete.ExecuteIfBound(bSuccess, CapturedDefault);
			}
		}
	));
}

void UHorizonRemoteConfigManager::GetBool(const FString& Key, bool DefaultValue, bool bUseCache, TDelegate<void(bool, bool)> OnComplete)
{
	TDelegate<void(bool, bool)> CapturedOnComplete = OnComplete;
	bool CapturedDefault = DefaultValue;

	GetConfig(Key, bUseCache, FOnConfigComplete::CreateLambda(
		[CapturedOnComplete, CapturedDefault](bool bSuccess, const FString& Value)
		{
			if (bSuccess && !Value.IsEmpty())
			{
				const FString Lower = Value.ToLower();
				if (Lower == TEXT("true") || Lower == TEXT("1") || Lower == TEXT("yes"))
				{
					CapturedOnComplete.ExecuteIfBound(true, true);
				}
				else if (Lower == TEXT("false") || Lower == TEXT("0") || Lower == TEXT("no"))
				{
					CapturedOnComplete.ExecuteIfBound(true, false);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("RemoteConfig::GetBool -- Value '%s' is not a valid boolean. Using default."), *Value);
					CapturedOnComplete.ExecuteIfBound(true, CapturedDefault);
				}
			}
			else
			{
				CapturedOnComplete.ExecuteIfBound(bSuccess, CapturedDefault);
			}
		}
	));
}

// ============================================================
// Cache
// ============================================================

void UHorizonRemoteConfigManager::ClearCache()
{
	ConfigCache.Empty();
	bAllConfigsCached = false;
	UE_LOG(LogHorizonSDK, Verbose, TEXT("Remote config cache cleared."));
}
