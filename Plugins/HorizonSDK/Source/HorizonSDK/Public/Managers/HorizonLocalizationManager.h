// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonLocalizationManager.generated.h"

DECLARE_DELEGATE_TwoParams(FOnLocalizationComplete, bool /*bSuccess*/, const FString& /*Value*/);
DECLARE_DELEGATE_TwoParams(FOnAllLocalizationsComplete, bool /*bSuccess*/, const TMap<FString, FString>& /*Translations*/);
DECLARE_DELEGATE_TwoParams(FOnLanguagesComplete, bool /*bSuccess*/, const TArray<FString>& /*Languages*/);

/**
 * Localization Manager for the horizOn SDK.
 *
 * Provides translated string retrieval with in-memory caching and a
 * configurable active language.  Does NOT require a session token.
 *
 * The server falls back to English ("en") when a key has no translation
 * for the requested language.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonLocalizationManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client reference. */
	void Initialize(UHorizonHttpClient* InHttpClient);

	/**
	 * Get a single translated value by key.
	 * @param Key        Localization key to look up.
	 * @param Lang       Two-letter language code; empty uses CurrentLanguage.
	 * @param OnComplete Called with (bSuccess, Value).
	 */
	void GetLocalization(const FString& Key, const FString& Lang, FOnLocalizationComplete OnComplete);

	/**
	 * Get all translations for a language as key-value pairs.
	 * @param Lang       Two-letter language code; empty uses CurrentLanguage.
	 * @param OnComplete Called with (bSuccess, Translations).
	 */
	void GetAllLocalizations(const FString& Lang, FOnAllLocalizationsComplete OnComplete);

	/**
	 * Get the list of available language codes.
	 * @param OnComplete Called with (bSuccess, Languages).
	 */
	void GetAvailableLanguages(FOnLanguagesComplete OnComplete);

	/**
	 * Set the active language used when no explicit language is supplied.
	 * Clears the in-memory translation cache.
	 * @param Lang Two-letter language code.
	 */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Localization")
	void SetLanguage(const FString& Lang);

	/** The active language code (defaults to the system language if supported, else "en"). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Localization")
	FString CurrentLanguage = TEXT("en");

	/** Clear the in-memory translation cache. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Localization")
	void ClearCache();

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	/** In-memory cache of translations for CurrentLanguage (key -> value). */
	TMap<FString, FString> LocalizationCache;

	/** Whether the full translation set for CurrentLanguage has been fetched at least once. */
	bool bAllLocalizationsCached = false;

	/** Detect the system language, falling back to "en" when unsupported. */
	static FString DetectSystemLanguage();

	/** The 15 language codes supported by the backend. */
	static const TArray<FString>& GetSupportedLanguages();

	/** True if the given (case-insensitive) language code is supported. */
	static bool IsSupportedLanguage(const FString& Lang);
};
