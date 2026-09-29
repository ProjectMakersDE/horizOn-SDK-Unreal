// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonValidatedActionsExample.generated.h"

/**
 * Minimal example: horizOn Validated Actions (server-checked runs).
 *
 * What it does: connects, signs up anonymously (runs need a signed-in player),
 * starts a run bound to the leaderboard LeaderboardKey, seeds a random stream with
 * the server seed, records a tiny input log, and submits score plus log hash.
 * A rejected run logs the rule code (for example DURATION_TOO_SHORT).
 *
 * Before running: nothing is required. Without a rule set the server accepts every
 * run with its default rules. To see a rejection, set "minDurationSeconds" above a
 * few seconds in the Validated Actions rules of your API key in the horizOn Dashboard.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [ValidatedActionsExample] Starting run...
 *   [ValidatedActionsExample] Run <id> started, seed <n>
 *   [ValidatedActionsExample] Submitting score <n>, input log hash <hash>
 *   [ValidatedActionsExample] SUCCESS: best <n>, rank <n>, measured <n> s
 *   or
 *   [ValidatedActionsExample] REJECTED (<code>): <message>
 */
UCLASS()
class HORIZONSDK_API AHorizonValidatedActionsExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonValidatedActionsExample();

	/** Board to bind the run to; empty runs without a leaderboard. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	FString LeaderboardKey = TEXT("default");

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
