// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonUserLogsExample.generated.h"

/**
 * Minimal example: horizOn User Logs.
 *
 * What it does: connects, signs up anonymously (user logs need a signed-in
 * user), then sends one INFO log entry to the server and logs the returned
 * log id.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [UserLogsExample] Sending an INFO log entry...
 *   [UserLogsExample] SUCCESS: log <id> created at <timestamp>
 */
UCLASS()
class HORIZONSDK_API AHorizonUserLogsExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonUserLogsExample();

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
