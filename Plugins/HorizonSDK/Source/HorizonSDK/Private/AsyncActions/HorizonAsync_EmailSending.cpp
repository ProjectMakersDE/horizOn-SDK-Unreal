// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_EmailSending.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonEmailSendingManager.h"

// ============================================================
// Send Email
// ============================================================

UHorizonAsync_SendEmail* UHorizonAsync_SendEmail::SendEmail(
	const UObject* WorldContextObject,
	const FString& InUserId, const FString& InTemplateSlug,
	const TMap<FString, FString>& InVariables, const FString& InLanguage,
	const FString& InScheduledAt)
{
	UHorizonAsync_SendEmail* Action = NewObject<UHorizonAsync_SendEmail>();
	Action->WorldContext = WorldContextObject;
	Action->UserId = InUserId;
	Action->TemplateSlug = InTemplateSlug;
	Action->Variables = InVariables;
	Action->Language = InLanguage;
	Action->ScheduledAt = InScheduledAt;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_SendEmail::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->EmailSending)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or EmailSending manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->EmailSending->SendEmail(
		UserId, TemplateSlug, Variables, Language, ScheduledAt,
		FOnSendEmailComplete::CreateUObject(this, &UHorizonAsync_SendEmail::HandleResult)
	);
}

void UHorizonAsync_SendEmail::HandleResult(bool bSuccess, const FSendEmailResponse& Response)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Response.Id, Response.Status);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to send email."));
	}
	SetReadyToDestroy();
}

// ============================================================
// Cancel Email
// ============================================================

UHorizonAsync_CancelEmail* UHorizonAsync_CancelEmail::CancelEmail(
	const UObject* WorldContextObject, const FString& InEmailId)
{
	UHorizonAsync_CancelEmail* Action = NewObject<UHorizonAsync_CancelEmail>();
	Action->WorldContext = WorldContextObject;
	Action->EmailId = InEmailId;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CancelEmail::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->EmailSending)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or EmailSending manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->EmailSending->CancelEmail(
		EmailId,
		FOnCancelEmailComplete::CreateUObject(this, &UHorizonAsync_CancelEmail::HandleResult)
	);
}

void UHorizonAsync_CancelEmail::HandleResult(bool bSuccess, const FCancelEmailResponse& Response)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Response.Message);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to cancel email."));
	}
	SetReadyToDestroy();
}

// ============================================================
// Get Email Status
// ============================================================

UHorizonAsync_GetEmailStatus* UHorizonAsync_GetEmailStatus::GetEmailStatus(
	const UObject* WorldContextObject, const FString& InEmailId)
{
	UHorizonAsync_GetEmailStatus* Action = NewObject<UHorizonAsync_GetEmailStatus>();
	Action->WorldContext = WorldContextObject;
	Action->EmailId = InEmailId;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_GetEmailStatus::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->EmailSending)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or EmailSending manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->EmailSending->GetEmailStatus(
		EmailId,
		FOnEmailStatusComplete::CreateUObject(this, &UHorizonAsync_GetEmailStatus::HandleResult)
	);
}

void UHorizonAsync_GetEmailStatus::HandleResult(bool bSuccess, const FEmailStatusResponse& Response)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Response);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to get email status."));
	}
	SetReadyToDestroy();
}
