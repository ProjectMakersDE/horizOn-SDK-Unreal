// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonCrashReportingExample.generated.h"

/**
 * Minimal example: horizOn Crash Reporting.
 *
 * What it does: connects, starts crash capture, records a couple of
 * breadcrumbs and a custom key, then reports a non-fatal exception and
 * logs the returned report id.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [CrashExample] Capture started. Recording a non-fatal exception...
 *   [CrashExample] SUCCESS: report <id> group <id>
 */
UCLASS()
class HORIZONSDK_API AHorizonCrashReportingExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonCrashReportingExample();

protected:
	virtual void BeginPlay() override;

private:
	/** Bound to UHorizonSubsystem::OnConnected; runs the feature flow. */
	UFUNCTION()
	void HandleConnected();

	/** Bound to UHorizonSubsystem::OnConnectionFailed. */
	UFUNCTION()
	void HandleConnectionFailed(const FString& ErrorMessage);
};
