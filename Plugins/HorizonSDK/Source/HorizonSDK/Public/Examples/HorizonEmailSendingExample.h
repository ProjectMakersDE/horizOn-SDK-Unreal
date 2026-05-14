// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorizonEmailSendingExample.generated.h"

/**
 * Minimal example: horizOn Email Sending.
 *
 * What it does: connects (email sending uses API key auth, no sign-in needed),
 * then sends one transactional email to a user via a template and logs the
 * returned email id.
 *
 * Before running: set RecipientUserId and TemplateSlug below to real values
 * from your horizOn Dashboard. The template slug must exist in the Dashboard.
 *
 * Where to set the API key: Project Settings > Plugins > horizOn SDK > API Key.
 * Drop this actor into a level and press Play to run the flow.
 *
 * Expected log output (channel LogTemp):
 *   [EmailExample] Sending email via template '<slug>'...
 *   [EmailExample] SUCCESS: email <id> status <status>
 */
UCLASS()
class HORIZONSDK_API AHorizonEmailSendingExample : public AActor
{
	GENERATED_BODY()

public:
	AHorizonEmailSendingExample();

	/** horizOn user ID of the recipient. Set this to a real user ID. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	FString RecipientUserId = TEXT("REPLACE_WITH_USER_ID");

	/** Template slug defined in the horizOn Dashboard. */
	UPROPERTY(EditAnywhere, Category = "horizOn|Example")
	FString TemplateSlug = TEXT("welcome");

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
