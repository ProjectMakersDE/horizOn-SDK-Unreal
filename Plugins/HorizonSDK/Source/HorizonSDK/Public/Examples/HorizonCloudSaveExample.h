// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonCloudSaveExample.generated.h"

/**
 * Minimal example: horizOn Cloud Save.
 *
 * What it does: connects, signs up anonymously (cloud save needs a signed-in
 * user), saves a small JSON string, then loads it back and logs the result.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [CloudSaveExample] Saving data...
 *   [CloudSaveExample] Saved. Loading it back...
 *   [CloudSaveExample] SUCCESS: loaded <data>
 */
UCLASS()
class HORIZONSDK_API AHorizonCloudSaveExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonCloudSaveExample();

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
