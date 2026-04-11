// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonEmailSendingManager.generated.h"

// ---- Data Models ----

USTRUCT(BlueprintType)
struct FSendEmailResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString ScheduledAt;
};

USTRUCT(BlueprintType)
struct FCancelEmailResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString Message;
};

USTRUCT(BlueprintType)
struct FEmailStatusResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString TemplateSlug;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString UserId;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString Language;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString ScheduledAt;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString ProcessedAt;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString ErrorReason;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|EmailSending")
	FString CreatedAt;
};

// ---- Delegates ----

DECLARE_DELEGATE_TwoParams(FOnSendEmailComplete, bool /*bSuccess*/, const FSendEmailResponse& /*Response*/);
DECLARE_DELEGATE_TwoParams(FOnCancelEmailComplete, bool /*bSuccess*/, const FCancelEmailResponse& /*Response*/);
DECLARE_DELEGATE_TwoParams(FOnEmailStatusComplete, bool /*bSuccess*/, const FEmailStatusResponse& /*Response*/);

// ---- Manager ----

/**
 * Email Sending Manager for the horizOn SDK.
 *
 * Sends transactional emails to registered users via pre-defined templates.
 * Supports immediate and scheduled delivery.
 * Does NOT require a session token -- uses API key auth.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonEmailSendingManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client reference. */
	void Initialize(UHorizonHttpClient* InHttpClient);

	/**
	 * Send an email to a registered user using a pre-defined template.
	 * @param UserId        The horizOn user ID of the recipient.
	 * @param TemplateSlug  Template slug defined in Dashboard.
	 * @param Variables     Variable values for the template.
	 * @param Language      Language code (e.g., "en", "de").
	 * @param ScheduledAt   Optional ISO 8601 timestamp. Empty string for immediate.
	 * @param OnComplete    Called with (bSuccess, Response).
	 */
	void SendEmail(const FString& UserId, const FString& TemplateSlug,
		const TMap<FString, FString>& Variables, const FString& Language,
		const FString& ScheduledAt, FOnSendEmailComplete OnComplete);

	/** Overload: send immediate email (no ScheduledAt). */
	void SendEmail(const FString& UserId, const FString& TemplateSlug,
		const TMap<FString, FString>& Variables, const FString& Language,
		FOnSendEmailComplete OnComplete);

	/**
	 * Cancel a pending or scheduled email.
	 * @param EmailId    The email ID returned by SendEmail.
	 * @param OnComplete Called with (bSuccess, Response).
	 */
	void CancelEmail(const FString& EmailId, FOnCancelEmailComplete OnComplete);

	/**
	 * Get the current status of a specific email.
	 * @param EmailId    The email ID returned by SendEmail.
	 * @param OnComplete Called with (bSuccess, Response).
	 */
	void GetEmailStatus(const FString& EmailId, FOnEmailStatusComplete OnComplete);

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;
};
