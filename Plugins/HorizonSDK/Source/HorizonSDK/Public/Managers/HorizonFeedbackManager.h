// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "HorizonFeedbackManager.generated.h"

/**
 * Feedback Manager for the horizOn SDK.
 *
 * Allows players to submit bug reports, feature requests, and general feedback.
 * Does NOT require a session token but attaches the userId if the user is signed in.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonFeedbackManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/**
	 * Submit feedback to the server.
	 * @param Title            Short title for the feedback.
	 * @param Category         One of: "BUG", "FEATURE", "GENERAL".
	 * @param Message          The feedback message body.
	 * @param Email            Optional contact email.
	 * @param bIncludeDeviceInfo If true, auto-collected device information is included.
	 * @param OnComplete       Called with (bSuccess, ErrorMessage).
	 */
	void Submit(const FString& Title, const FString& Category, const FString& Message,
		const FString& Email, bool bIncludeDeviceInfo, FOnRequestComplete OnComplete);

	/**
	 * Shorthand: report a bug (category = "BUG", device info included).
	 * @param Title      Short title.
	 * @param Message    Bug description.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 * @param Email      Optional contact email.
	 */
	void ReportBug(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email = TEXT(""));

	/**
	 * Shorthand: request a feature (category = "FEATURE", device info excluded).
	 * @param Title      Short title.
	 * @param Message    Feature description.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 * @param Email      Optional contact email.
	 */
	void RequestFeature(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email = TEXT(""));

	/**
	 * Shorthand: send general feedback (category = "GENERAL", device info excluded).
	 * @param Title      Short title.
	 * @param Message    Feedback body.
	 * @param OnComplete Called with (bSuccess, ErrorMessage).
	 * @param Email      Optional contact email.
	 */
	void SendGeneral(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email = TEXT(""));

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;

	/** Collect device and platform information as a single string. */
	FString CollectDeviceInfo() const;
};
