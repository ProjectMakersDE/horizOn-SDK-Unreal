// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonEmailSendingManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonEmailSendingManager::Initialize(UHorizonHttpClient* InHttpClient)
{
	HttpClient = InHttpClient;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonEmailSendingManager initialized."));
}

// ============================================================
// SendEmail
// ============================================================

void UHorizonEmailSendingManager::SendEmail(const FString& UserId, const FString& TemplateSlug,
	const TMap<FString, FString>& Variables, const FString& Language,
	const FString& ScheduledAt, FOnSendEmailComplete OnComplete)
{
	// Validation
	if (UserId.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("EmailSending::SendEmail -- UserId is required."));
		OnComplete.ExecuteIfBound(false, FSendEmailResponse());
		return;
	}

	if (TemplateSlug.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("EmailSending::SendEmail -- TemplateSlug is required."));
		OnComplete.ExecuteIfBound(false, FSendEmailResponse());
		return;
	}

	if (Language.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("EmailSending::SendEmail -- Language is required."));
		OnComplete.ExecuteIfBound(false, FSendEmailResponse());
		return;
	}

	// Build request body
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);
	Body->SetStringField(TEXT("templateSlug"), TemplateSlug);
	Body->SetStringField(TEXT("language"), Language);

	// Variables as JSON object
	TSharedRef<FJsonObject> VarsObj = MakeShared<FJsonObject>();
	for (const auto& Pair : Variables)
	{
		VarsObj->SetStringField(Pair.Key, Pair.Value);
	}
	Body->SetObjectField(TEXT("variables"), VarsObj);

	if (!ScheduledAt.IsEmpty())
	{
		Body->SetStringField(TEXT("scheduledAt"), ScheduledAt);
	}

	TWeakObjectPtr<UHorizonEmailSendingManager> WeakSelf(this);
	FOnSendEmailComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/email-sending/send"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("EmailSending::SendEmail -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, FSendEmailResponse());
					return;
				}

				FSendEmailResponse Result;
				if (Response.JsonData.IsValid())
				{
					Response.JsonData->TryGetStringField(TEXT("id"), Result.Id);
					Response.JsonData->TryGetStringField(TEXT("status"), Result.Status);
					Response.JsonData->TryGetStringField(TEXT("scheduledAt"), Result.ScheduledAt);
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("EmailSending::SendEmail -- Email queued: %s"), *Result.Id);
				CapturedOnComplete.ExecuteIfBound(true, Result);
			}
		));
}

void UHorizonEmailSendingManager::SendEmail(const FString& UserId, const FString& TemplateSlug,
	const TMap<FString, FString>& Variables, const FString& Language,
	FOnSendEmailComplete OnComplete)
{
	SendEmail(UserId, TemplateSlug, Variables, Language, FString(), OnComplete);
}

// ============================================================
// CancelEmail
// ============================================================

void UHorizonEmailSendingManager::CancelEmail(const FString& EmailId, FOnCancelEmailComplete OnComplete)
{
	if (EmailId.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("EmailSending::CancelEmail -- EmailId is required."));
		OnComplete.ExecuteIfBound(false, FCancelEmailResponse());
		return;
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/email-sending/%s"), *EmailId);

	TWeakObjectPtr<UHorizonEmailSendingManager> WeakSelf(this);
	FOnCancelEmailComplete CapturedOnComplete = OnComplete;

	HttpClient->Delete(Endpoint, false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, EmailId](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("EmailSending::CancelEmail -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, FCancelEmailResponse());
					return;
				}

				FCancelEmailResponse Result;
				if (Response.JsonData.IsValid())
				{
					Response.JsonData->TryGetStringField(TEXT("message"), Result.Message);
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("EmailSending::CancelEmail -- Cancelled: %s"), *EmailId);
				CapturedOnComplete.ExecuteIfBound(true, Result);
			}
		));
}

// ============================================================
// GetEmailStatus
// ============================================================

void UHorizonEmailSendingManager::GetEmailStatus(const FString& EmailId, FOnEmailStatusComplete OnComplete)
{
	if (EmailId.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("EmailSending::GetEmailStatus -- EmailId is required."));
		OnComplete.ExecuteIfBound(false, FEmailStatusResponse());
		return;
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/email-sending/%s"), *EmailId);

	TWeakObjectPtr<UHorizonEmailSendingManager> WeakSelf(this);
	FOnEmailStatusComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(Endpoint, false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, EmailId](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("EmailSending::GetEmailStatus -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, FEmailStatusResponse());
					return;
				}

				FEmailStatusResponse Result;
				if (Response.JsonData.IsValid())
				{
					Response.JsonData->TryGetStringField(TEXT("id"), Result.Id);
					Response.JsonData->TryGetStringField(TEXT("status"), Result.Status);
					Response.JsonData->TryGetStringField(TEXT("templateSlug"), Result.TemplateSlug);
					Response.JsonData->TryGetStringField(TEXT("userId"), Result.UserId);
					Response.JsonData->TryGetStringField(TEXT("language"), Result.Language);
					Response.JsonData->TryGetStringField(TEXT("scheduledAt"), Result.ScheduledAt);
					Response.JsonData->TryGetStringField(TEXT("processedAt"), Result.ProcessedAt);
					Response.JsonData->TryGetStringField(TEXT("errorReason"), Result.ErrorReason);
					Response.JsonData->TryGetStringField(TEXT("createdAt"), Result.CreatedAt);
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("EmailSending::GetEmailStatus -- %s = %s"), *EmailId, *Result.Status);
				CapturedOnComplete.ExecuteIfBound(true, Result);
			}
		));
}
