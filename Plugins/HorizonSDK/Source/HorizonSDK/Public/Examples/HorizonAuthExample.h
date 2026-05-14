// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonAuthExample.generated.h"

/**
 * Minimal example: horizOn Authentication.
 *
 * What it does: connects to the backend, then signs up a new anonymous user
 * and logs whether the signed-in user data looks valid.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [AuthExample] Connecting...
 *   [AuthExample] Connected. Signing up anonymously...
 *   [AuthExample] SUCCESS: signed in as <DisplayName> (UserId set)
 */
UCLASS()
class HORIZONSDK_API AHorizonAuthExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonAuthExample();

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
