// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonGiftCodesExample.generated.h"

/**
 * Minimal example: horizOn Gift Codes.
 *
 * What it does: connects, signs up anonymously (redeeming needs a signed-in
 * user), validates a gift code, then redeems it and logs the granted data.
 *
 * Before running: set GiftCode below to a real code from your horizOn
 * Dashboard.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [GiftCodesExample] Validating code...
 *   [GiftCodesExample] Code is valid. Redeeming...
 *   [GiftCodesExample] SUCCESS: <message> data <data>
 */
UCLASS()
class HORIZONSDK_API AHorizonGiftCodesExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonGiftCodesExample();

	/** Gift code to validate and redeem. Set this to a real code. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	FString GiftCode = TEXT("REPLACE_WITH_GIFT_CODE");

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
