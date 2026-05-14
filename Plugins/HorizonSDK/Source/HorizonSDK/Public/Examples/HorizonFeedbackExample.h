// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonFeedbackExample.generated.h"

/**
 * Minimal example: horizOn Feedback.
 *
 * What it does: connects (feedback needs no sign-in, but attaches the user ID
 * if one is signed in), then submits one general feedback entry and logs the
 * result.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [FeedbackExample] Submitting general feedback...
 *   [FeedbackExample] SUCCESS: feedback submitted
 */
UCLASS()
class HORIZONSDK_API AHorizonFeedbackExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonFeedbackExample();

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
