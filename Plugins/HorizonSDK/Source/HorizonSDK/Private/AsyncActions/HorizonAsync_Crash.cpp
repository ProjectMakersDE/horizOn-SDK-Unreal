// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Crash.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonCrashManager.h"

// ============================================================
// UHorizonAsync_CrashRecordException
// ============================================================

UHorizonAsync_CrashRecordException* UHorizonAsync_CrashRecordException::RecordException(
	const UObject* WorldContextObject, const FString& Message, const FString& StackTrace)
{
	UHorizonAsync_CrashRecordException* Action = NewObject<UHorizonAsync_CrashRecordException>();
	Action->WorldContext = WorldContextObject;
	Action->ParamMessage = Message;
	Action->ParamStackTrace = StackTrace;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CrashRecordException::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Crashes)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CrashManager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Crashes->RecordException(
		ParamMessage,
		ParamStackTrace,
		{},
		FOnCrashReportComplete::CreateUObject(this, &UHorizonAsync_CrashRecordException::HandleResult));
}

void UHorizonAsync_CrashRecordException::HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(ReportId, GroupId);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to submit crash report."));
	}
	SetReadyToDestroy();
}

// ============================================================
// UHorizonAsync_CrashReportCrash
// ============================================================

UHorizonAsync_CrashReportCrash* UHorizonAsync_CrashReportCrash::ReportCrash(
	const UObject* WorldContextObject, const FString& Message, const FString& StackTrace)
{
	UHorizonAsync_CrashReportCrash* Action = NewObject<UHorizonAsync_CrashReportCrash>();
	Action->WorldContext = WorldContextObject;
	Action->ParamMessage = Message;
	Action->ParamStackTrace = StackTrace;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CrashReportCrash::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Crashes)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CrashManager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Crashes->ReportCrash(
		ParamMessage,
		ParamStackTrace,
		FOnCrashReportComplete::CreateUObject(this, &UHorizonAsync_CrashReportCrash::HandleResult));
}

void UHorizonAsync_CrashReportCrash::HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(ReportId, GroupId);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to submit crash report."));
	}
	SetReadyToDestroy();
}
