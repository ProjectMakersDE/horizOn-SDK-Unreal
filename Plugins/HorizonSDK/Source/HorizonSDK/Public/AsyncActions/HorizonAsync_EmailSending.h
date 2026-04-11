// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Managers/HorizonEmailSendingManager.h"
#include "HorizonAsync_EmailSending.generated.h"

// ---- Send Email ----

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSendEmailResult, const FString&, EmailId, const FString&, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmailSendingAsyncFailure, const FString&, ErrorMessage);

UCLASS()
class HORIZONSDK_API UHorizonAsync_SendEmail : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnSendEmailResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnEmailSendingAsyncFailure OnFailure;

	/** Send an email to a user via a template. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Send Email"), Category = "horizOn|EmailSending")
	static UHorizonAsync_SendEmail* SendEmail(const UObject* WorldContextObject,
		const FString& UserId, const FString& TemplateSlug,
		const TMap<FString, FString>& Variables, const FString& Language,
		const FString& ScheduledAt);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString UserId;
	FString TemplateSlug;
	TMap<FString, FString> Variables;
	FString Language;
	FString ScheduledAt;

	void HandleResult(bool bSuccess, const FSendEmailResponse& Response);
};

// ---- Cancel Email ----

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCancelEmailResult, const FString&, Message);

UCLASS()
class HORIZONSDK_API UHorizonAsync_CancelEmail : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCancelEmailResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnEmailSendingAsyncFailure OnFailure;

	/** Cancel a pending or scheduled email. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Cancel Email"), Category = "horizOn|EmailSending")
	static UHorizonAsync_CancelEmail* CancelEmail(const UObject* WorldContextObject, const FString& EmailId);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString EmailId;

	void HandleResult(bool bSuccess, const FCancelEmailResponse& Response);
};

// ---- Get Email Status ----

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmailStatusResult, const FEmailStatusResponse&, Response);

UCLASS()
class HORIZONSDK_API UHorizonAsync_GetEmailStatus : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnEmailStatusResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnEmailSendingAsyncFailure OnFailure;

	/** Get the current status of an email. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Email Status"), Category = "horizOn|EmailSending")
	static UHorizonAsync_GetEmailStatus* GetEmailStatus(const UObject* WorldContextObject, const FString& EmailId);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString EmailId;

	void HandleResult(bool bSuccess, const FEmailStatusResponse& Response);
};
