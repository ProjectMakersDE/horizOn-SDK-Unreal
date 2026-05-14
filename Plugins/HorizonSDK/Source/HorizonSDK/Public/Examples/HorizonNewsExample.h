// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonNewsExample.generated.h"

/**
 * Minimal example: horizOn News.
 *
 * What it does: connects (news needs no sign-in), then loads up to 5 English
 * news entries and logs each title.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [NewsExample] Loading up to 5 news entries...
 *   [NewsExample] SUCCESS: <n> entries
 *   [NewsExample]  - <title>
 */
UCLASS()
class HORIZONSDK_API AHorizonNewsExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonNewsExample();

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
