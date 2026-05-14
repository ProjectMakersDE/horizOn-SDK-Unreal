// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonLeaderboardExample.generated.h"

/**
 * Minimal example: horizOn Leaderboards.
 *
 * What it does: connects, signs up anonymously (scores need a signed-in user),
 * submits a score, then fetches the top 5 entries and logs them.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [LeaderboardExample] Connected. Signing in...
 *   [LeaderboardExample] Score submitted. Fetching top 5...
 *   [LeaderboardExample] #1 <name> <score> ...
 */
UCLASS()
class HORIZONSDK_API AHorizonLeaderboardExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonLeaderboardExample();

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
