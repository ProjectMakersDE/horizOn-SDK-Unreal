// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Feedback.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonFeedbackManager.h"

// ============================================================
// Factory methods
// ============================================================

UHorizonAsync_Feedback* UHorizonAsync_Feedback::SubmitFeedback(
	const UObject* WorldContextObject, const FString& Title, const FString& Message, const FString& Email)
{
	UHorizonAsync_Feedback* Action = NewObject<UHorizonAsync_Feedback>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EFeedbackOp::General;
	Action->ParamTitle = Title;
	Action->ParamMessage = Message;
	Action->ParamEmail = Email;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_Feedback* UHorizonAsync_Feedback::ReportBug(
	const UObject* WorldContextObject, const FString& Title, const FString& Message, const FString& Email)
{
	UHorizonAsync_Feedback* Action = NewObject<UHorizonAsync_Feedback>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EFeedbackOp::Bug;
	Action->ParamTitle = Title;
	Action->ParamMessage = Message;
	Action->ParamEmail = Email;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_Feedback* UHorizonAsync_Feedback::RequestFeature(
	const UObject* WorldContextObject, const FString& Title, const FString& Message, const FString& Email)
{
	UHorizonAsync_Feedback* Action = NewObject<UHorizonAsync_Feedback>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EFeedbackOp::Feature;
	Action->ParamTitle = Title;
	Action->ParamMessage = Message;
	Action->ParamEmail = Email;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

// ============================================================
// Activate
// ============================================================

void UHorizonAsync_Feedback::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Feedback)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Feedback manager not found."));
		SetReadyToDestroy();
		return;
	}

	FOnRequestComplete Callback = FOnRequestComplete::CreateUObject(this, &UHorizonAsync_Feedback::HandleResult);

	switch (Operation)
	{
	case EFeedbackOp::General:
		Subsystem->Feedback->SendGeneral(ParamTitle, ParamMessage, Callback, ParamEmail);
		break;
	case EFeedbackOp::Bug:
		Subsystem->Feedback->ReportBug(ParamTitle, ParamMessage, Callback, ParamEmail);
		break;
	case EFeedbackOp::Feature:
		Subsystem->Feedback->RequestFeature(ParamTitle, ParamMessage, Callback, ParamEmail);
		break;
	}
}

void UHorizonAsync_Feedback::HandleResult(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast(ErrorMessage);
	}
	SetReadyToDestroy();
}
