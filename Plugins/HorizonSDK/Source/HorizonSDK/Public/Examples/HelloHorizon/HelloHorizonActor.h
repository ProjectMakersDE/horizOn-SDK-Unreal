// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HelloHorizonActor.generated.h"

/**
 * Hello horizOn: the minimal front door to the horizOn SDK.
 *
 * This actor runs the shortest end-to-end flow on BeginPlay:
 *   1. Connect to the backend.
 *   2. Sign up an anonymous user.
 *   3. Submit a leaderboard score.
 *   4. Show the result in the log and as an on-screen debug message.
 *
 * Three steps to run it:
 *   1. Enable the horizOn SDK plugin (Edit > Plugins).
 *   2. Set your API key in Project Settings > Plugins > horizOn SDK.
 *   3. Drop AHelloHorizonActor into a level and press Play.
 *
 * Expected log output (channel LogTemp):
 *   [HelloHorizon] Connecting...
 *   [HelloHorizon] Connected. Signing up...
 *   [HelloHorizon] Signed up. Submitting score...
 *   [HelloHorizon] Done: score submitted. You are live on horizOn.
 */
UCLASS()
class HORIZONSDK_API AHelloHorizonActor : public AActor
{
	GENERATED_BODY()

public:
	AHelloHorizonActor();

	/** Display name used for the anonymous sign-up. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Hello")
	FString DisplayName = TEXT("HelloPlayer");

	/** Score submitted to the leaderboard once signed in. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Hello")
	int64 Score = 1000;

protected:
	virtual void BeginPlay() override;

private:
	/** Bound to UHorizonSubsystem::OnConnected. */
	UFUNCTION()
	void HandleConnected();

	/** Bound to UHorizonSubsystem::OnConnectionFailed. */
	UFUNCTION()
	void HandleConnectionFailed(const FString& ErrorMessage);

	/** Print to the log and, if available, to the screen. */
	void Report(const FString& Message);
};
