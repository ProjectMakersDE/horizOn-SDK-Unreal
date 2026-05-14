// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonCrashReportingExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonCrashManager.h"
#include "Engine/GameInstance.h"

AHorizonCrashReportingExample::AHorizonCrashReportingExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonCrashReportingExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[CrashExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonCrashReportingExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonCrashReportingExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[CrashExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonCrashReportingExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[CrashExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonCrashReportingExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Crashes)
	{
		UE_LOG(LogTemp, Error, TEXT("[CrashExample] Crash manager not available."));
		return;
	}

	UHorizonCrashManager* Crashes = Horizon->Crashes;

	// Start capture, then add a little context before reporting a non-fatal exception.
	Crashes->StartCapture();
	Crashes->RecordBreadcrumb(TEXT("navigation"), TEXT("Entered example level"));
	Crashes->Log(TEXT("Crash example actor running"));
	Crashes->SetCustomKey(TEXT("example"), TEXT("crash-reporting"));

	UE_LOG(LogTemp, Log, TEXT("[CrashExample] Capture started. Recording a non-fatal exception..."));

	Crashes->RecordException(
		TEXT("Example non-fatal exception"),
		TEXT("AHorizonCrashReportingExample::HandleConnected (synthetic stack trace)"),
		{ { TEXT("severity"), TEXT("low") } },
		FOnCrashReportComplete::CreateLambda(
			[](bool bSuccess, const FString& ReportId, const FString& GroupId)
			{
				if (!bSuccess)
				{
					UE_LOG(LogTemp, Error, TEXT("[CrashExample] FAILED to record exception."));
					return;
				}

				UE_LOG(LogTemp, Log, TEXT("[CrashExample] SUCCESS: report %s group %s"), *ReportId, *GroupId);
			}));
}
