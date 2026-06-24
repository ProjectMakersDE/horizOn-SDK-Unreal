// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonLocalizationManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonLocalizationManager::Initialize(UHorizonHttpClient* InHttpClient)
{
	HttpClient = InHttpClient;
	CurrentLanguage = DetectSystemLanguage();
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonLocalizationManager initialized (language '%s')."), *CurrentLanguage);
}

// ============================================================
// GetLocalization
// ============================================================

void UHorizonLocalizationManager::GetLocalization(const FString& Key, const FString& Lang, FOnLocalizationComplete OnComplete)
{
	const FString EffectiveLang = Lang.IsEmpty() ? CurrentLanguage : Lang.ToLower();
	const bool bIsCurrentLang = EffectiveLang == CurrentLanguage;

	if (bIsCurrentLang)
	{
		const FString* Cached = LocalizationCache.Find(Key);
		if (Cached)
		{
			UE_LOG(LogHorizonSDK, Verbose, TEXT("Localization::GetLocalization -- Returning cached value for key '%s'."), *Key);
			OnComplete.ExecuteIfBound(true, *Cached);
			return;
		}
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/localization/%s?lang=%s"), *Key, *EffectiveLang);

	TWeakObjectPtr<UHorizonLocalizationManager> WeakSelf(this);
	FOnLocalizationComplete CapturedOnComplete = OnComplete;
	FString CapturedKey = Key;

	HttpClient->Get(Endpoint, false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, CapturedKey, bIsCurrentLang](const FHorizonNetworkResponse& Response)
			{
				UHorizonLocalizationManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Localization::GetLocalization -- Failed for key '%s': %s"), *CapturedKey, *Response.ErrorMessage);
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
						Value = Response.JsonData->GetStringField(TEXT("value"));
						if (bIsCurrentLang)
						{
							Self->LocalizationCache.Add(CapturedKey, Value);
						}
					}
				}

				if (bFound)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("Localization::GetLocalization -- Key '%s' = '%s'"), *CapturedKey, *Value);
					CapturedOnComplete.ExecuteIfBound(true, Value);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("Localization::GetLocalization -- Key '%s' not found."), *CapturedKey);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
				}
			}
		));
}

// ============================================================
// GetAllLocalizations
// ============================================================

void UHorizonLocalizationManager::GetAllLocalizations(const FString& Lang, FOnAllLocalizationsComplete OnComplete)
{
	const FString EffectiveLang = Lang.IsEmpty() ? CurrentLanguage : Lang.ToLower();
	const bool bIsCurrentLang = EffectiveLang == CurrentLanguage;

	if (bIsCurrentLang && bAllLocalizationsCached)
	{
		UE_LOG(LogHorizonSDK, Verbose, TEXT("Localization::GetAllLocalizations -- Returning cached translations (%d entries)."), LocalizationCache.Num());
		OnComplete.ExecuteIfBound(true, LocalizationCache);
		return;
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/localization/all?lang=%s"), *EffectiveLang);

	TWeakObjectPtr<UHorizonLocalizationManager> WeakSelf(this);
	FOnAllLocalizationsComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(Endpoint, false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, bIsCurrentLang](const FHorizonNetworkResponse& Response)
			{
				UHorizonLocalizationManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Localization::GetAllLocalizations -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TMap<FString, FString>());
					return;
				}

				TMap<FString, FString> Translations;

				if (Response.JsonData.IsValid() && Response.JsonData->HasField(TEXT("translations")))
				{
					const TSharedPtr<FJsonObject> TranslationsObj = Response.JsonData->GetObjectField(TEXT("translations"));
					if (TranslationsObj.IsValid())
					{
						for (const auto& Pair : TranslationsObj->Values)
						{
							FString Value;
							if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
							{
								Translations.Add(Pair.Key, Value);
							}
						}
					}
				}

				// Update cache only when the fetched language is the active one.
				if (bIsCurrentLang)
				{
					Self->LocalizationCache = Translations;
					Self->bAllLocalizationsCached = true;
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("Localization::GetAllLocalizations -- Retrieved %d translations."), Translations.Num());
				CapturedOnComplete.ExecuteIfBound(true, Translations);
			}
		));
}

// ============================================================
// GetAvailableLanguages
// ============================================================

void UHorizonLocalizationManager::GetAvailableLanguages(FOnLanguagesComplete OnComplete)
{
	TWeakObjectPtr<UHorizonLocalizationManager> WeakSelf(this);
	FOnLanguagesComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(TEXT("api/v1/app/localization/languages"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				UHorizonLocalizationManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Localization::GetAvailableLanguages -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<FString>());
					return;
				}

				TArray<FString> Languages;

				if (Response.JsonData.IsValid() && Response.JsonData->HasField(TEXT("languages")))
				{
					const TArray<TSharedPtr<FJsonValue>>* LanguagesArray = nullptr;
					if (Response.JsonData->TryGetArrayField(TEXT("languages"), LanguagesArray) && LanguagesArray)
					{
						for (const TSharedPtr<FJsonValue>& Entry : *LanguagesArray)
						{
							FString Value;
							if (Entry.IsValid() && Entry->TryGetString(Value))
							{
								Languages.Add(Value);
							}
						}
					}
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("Localization::GetAvailableLanguages -- Retrieved %d languages."), Languages.Num());
				CapturedOnComplete.ExecuteIfBound(true, Languages);
			}
		));
}

// ============================================================
// Language
// ============================================================

void UHorizonLocalizationManager::SetLanguage(const FString& Lang)
{
	const FString Normalized = Lang.ToLower();
	if (Normalized == CurrentLanguage)
	{
		return;
	}

	if (!IsSupportedLanguage(Normalized))
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Localization::SetLanguage -- Unsupported language '%s', keeping '%s'."), *Normalized, *CurrentLanguage);
		return;
	}

	CurrentLanguage = Normalized;
	ClearCache();
	UE_LOG(LogHorizonSDK, Log, TEXT("Localization::SetLanguage -- Active language set to '%s'."), *CurrentLanguage);
}

// ============================================================
// Cache
// ============================================================

void UHorizonLocalizationManager::ClearCache()
{
	LocalizationCache.Empty();
	bAllLocalizationsCached = false;
	UE_LOG(LogHorizonSDK, Verbose, TEXT("Localization cache cleared."));
}

// ============================================================
// Helpers
// ============================================================

FString UHorizonLocalizationManager::DetectSystemLanguage()
{
	const FCulturePtr Culture = FInternationalization::Get().GetCurrentLanguage();
	if (Culture.IsValid())
	{
		const FString TwoLetter = Culture->GetTwoLetterISOLanguageName().ToLower();
		if (IsSupportedLanguage(TwoLetter))
		{
			return TwoLetter;
		}
	}
	return TEXT("en");
}

const TArray<FString>& UHorizonLocalizationManager::GetSupportedLanguages()
{
	static const TArray<FString> Supported = {
		TEXT("en"), TEXT("de"), TEXT("es"), TEXT("fr"), TEXT("it"),
		TEXT("pt"), TEXT("nl"), TEXT("pl"), TEXT("ru"), TEXT("ja"),
		TEXT("zh"), TEXT("ar"), TEXT("ko"), TEXT("tr"), TEXT("id")
	};
	return Supported;
}

bool UHorizonLocalizationManager::IsSupportedLanguage(const FString& Lang)
{
	return GetSupportedLanguages().Contains(Lang.ToLower());
}
