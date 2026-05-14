// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonRemoteConfigExample.generated.h"

/**
 * Minimal example: horizOn Remote Config.
 *
 * What it does: connects (remote config needs no sign-in), reads one string
 * config value with a default fallback, then fetches the full config map and
 * logs every key.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [RemoteConfigExample] welcome_message = <value>
 *   [RemoteConfigExample] SUCCESS: <n> config entries
 */
UCLASS()
class HORIZONSDK_API AHorizonRemoteConfigExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonRemoteConfigExample();

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
