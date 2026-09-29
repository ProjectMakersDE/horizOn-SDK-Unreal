// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Models/HorizonValidatedActions.h"
#include "HorizonValidatedActionsExample.generated.h"

class UHorizonSubsystem;

/**
 * Minimal example: horizOn Validated Actions (server-checked runs and server-owned state).
 *
 * What it does: connects, signs up anonymously (runs need a signed-in player), reads the
 * player's server-owned values (GetState), starts a run bound to the leaderboard
 * LeaderboardKey, seeds a random stream with the server seed, records a tiny input log, and
 * submits score plus log hash. With ValueKey set, the run also earns EarnedAmount of that
 * value, and the state after the run is logged (Requested, Credited, EarnedToday / DailyCap).
 * Finally it shows how to mirror the state into the cloud save (log only, unless
 * bMirrorToCloudSave is on). A rejected run logs the code (for example DURATION_TOO_SHORT).
 *
 * Before running: nothing is required. Without a rule set the server accepts every run with
 * its default rules. To see server-owned values, define a value (for example "gold" with
 * "maxPerRun": 100) under "values" in the Validated Actions rules of your API key in the
 * horizOn Dashboard and set ValueKey to that key. A ValueKey the rules do not define rejects
 * the run with UNKNOWN_VALUE_KEY. To see a rule rejection, set "minDurationSeconds" above a
 * few seconds.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [ValidatedActionsExample] State on <day>: <n> value(s)
 *   [ValidatedActionsExample] Starting run...
 *   [ValidatedActionsExample] Run <id> started, seed <n>
 *   [ValidatedActionsExample] Submitting score <n>, input log hash <hash>
 *   [ValidatedActionsExample] SUCCESS: best <n>, rank <n>, measured <n> s
 *   [ValidatedActionsExample]   gold: balance <n>, requested <n>, credited <n>, today <n> / <cap>
 *   [ValidatedActionsExample] Cloud save mirror: {"day":"<day>","balances":{"gold":<n>}}
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

	/** Server-owned value the run earns (must be defined in the rules); empty sends no earned values. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	FString ValueKey;

	/** Amount of ValueKey the run earns; negative spends. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	int64 EarnedAmount = 10;

	/**
	 * Write the mirror to the cloud save. Off by default: SaveObject replaces the whole cloud
	 * save of the player, a real game merges the mirror into its own save data instead.
	 */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	bool bMirrorToCloudSave = false;

	/**
	 * Cloud save mirror of the server-owned values: one JSON string with day and balances.
	 * The copy is for display and offline start only. Never send it back as a balance: values
	 * change only through `earned` of a validated run, and GetState overwrites the copy.
	 */
	static FString BuildCloudSaveMirror(const FHorizonPlayerState& State);

protected:
	virtual void BeginPlay() override;

private:
	/** Bound to UHorizonSubsystem::OnConnected; runs the feature flow. */
	UFUNCTION()
	void HandleConnected();

	/** Bound to UHorizonSubsystem::OnConnectionFailed. */
	UFUNCTION()
	void HandleConnectionFailed(const FString& ErrorMessage);

	/** Step 2: start a run, play, submit (with ValueKey as earned value). */
	void StartAndSubmitRun();

	/** Step 3: log the state after the run and mirror it into the cloud save. */
	void HandleStateAfterRun(const FHorizonPlayerState& State);

	UHorizonSubsystem* GetHorizon() const;
};
