// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_Localization.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalizationValueResult, const FString&, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLocalizationAllSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalizationLanguagesResult, const TArray<FString>&, Languages);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalizationAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Get a single translated value by key.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LocalizationGet : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLocalizationValueResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLocalizationAsyncFailure OnFailure;

	/** Get a single translated value by key (empty Lang uses the active language). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Localization"), Category = "horizOn|Localization")
	static UHorizonAsync_LocalizationGet* GetLocalization(const UObject* WorldContextObject, const FString& Key, const FString& Lang);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString LocalizationKey;
	FString LanguageCode;

	void HandleResult(bool bSuccess, const FString& Value);
};

// ============================================================

/**
 * Async Blueprint node: Fetch all translations for a language.
 *
 * After success, call GetAllLocalizations on the Localization manager
 * to access the cached values, or use GetLocalization for individual keys.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LocalizationGetAll : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLocalizationAllSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLocalizationAsyncFailure OnFailure;

	/** Fetch all translation key-value pairs for a language (empty Lang uses the active language). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get All Localizations"), Category = "horizOn|Localization")
	static UHorizonAsync_LocalizationGetAll* GetAllLocalizations(const UObject* WorldContextObject, const FString& Lang);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString LanguageCode;

	void HandleResult(bool bSuccess, const TMap<FString, FString>& Translations);
};

// ============================================================

/**
 * Async Blueprint node: Get the list of available language codes.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_LocalizationGetLanguages : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnLocalizationLanguagesResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnLocalizationAsyncFailure OnFailure;

	/** Get the list of available language codes. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Available Languages"), Category = "horizOn|Localization")
	static UHorizonAsync_LocalizationGetLanguages* GetAvailableLanguages(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;

	void HandleResult(bool bSuccess, const TArray<FString>& Languages);
};
