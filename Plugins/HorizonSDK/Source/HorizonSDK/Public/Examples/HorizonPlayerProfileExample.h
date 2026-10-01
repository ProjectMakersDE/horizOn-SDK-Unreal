// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonPlayerProfileExample.generated.h"

/**
 * Minimal example: horizOn Player Profile (avatar, frame, badges).
 *
 * What it does: connects, signs up anonymously (the profile needs a signed-in
 * player), loads the profile with the cosmetics catalog, picks the first
 * available avatar and saves it while keeping frame and badges.
 *
 * Before running: add at least one free avatar (type "avatar", not locked) to
 * the cosmetics catalog of your API key in the horizOn Dashboard.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [PlayerProfileExample] Loading profile...
 *   [PlayerProfileExample] Catalog has <n> avatars, <n> frames, <n> badges.
 *   [PlayerProfileExample] Selecting avatar <id>...
 *   [PlayerProfileExample] SUCCESS: avatar <id>, frame <id>, <n> badges
 */
UCLASS()
class HORIZONSDK_API AHorizonPlayerProfileExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonPlayerProfileExample();

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
