// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonLocalizationExample.generated.h"

/**
 * Minimal example: horizOn Localization.
 *
 * What it does: connects (localization needs no sign-in), reads one translated
 * string for the active language, then fetches the full translation map and
 * logs every key. The active language defaults to the system language when it
 * is one of the supported codes, otherwise "en".
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [LocalizationExample] welcome_message = <value>
 *   [LocalizationExample] SUCCESS: <n> translation entries
 */
UCLASS()
class HORIZONSDK_API AHorizonLocalizationExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonLocalizationExample();

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
