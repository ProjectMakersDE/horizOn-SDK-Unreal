// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_Feedback.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFeedbackAsyncSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFeedbackAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint nodes for horizOn Feedback operations.
 *
 * Provides factory methods for submitting general feedback,
 * reporting bugs, and requesting features.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_Feedback : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnFeedbackAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnFeedbackAsyncFailure OnFailure;

	/** Submit general feedback. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Submit Feedback"), Category = "horizOn|Feedback")
	static UHorizonAsync_Feedback* SubmitFeedback(const UObject* WorldContextObject, const FString& Title, const FString& Message, const FString& Email);

	/** Report a bug (includes device info automatically). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Report Bug"), Category = "horizOn|Feedback")
	static UHorizonAsync_Feedback* ReportBug(const UObject* WorldContextObject, const FString& Title, const FString& Message, const FString& Email);

	/** Request a feature. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Request Feature"), Category = "horizOn|Feedback")
	static UHorizonAsync_Feedback* RequestFeature(const UObject* WorldContextObject, const FString& Title, const FString& Message, const FString& Email);

	virtual void Activate() override;

private:
	enum class EFeedbackOp : uint8
	{
		General,
		Bug,
		Feature
	};

	TWeakObjectPtr<const UObject> WorldContext;
	EFeedbackOp Operation;
	FString ParamTitle;
	FString ParamMessage;
	FString ParamEmail;

	void HandleResult(bool bSuccess, const FString& ErrorMessage);
};
