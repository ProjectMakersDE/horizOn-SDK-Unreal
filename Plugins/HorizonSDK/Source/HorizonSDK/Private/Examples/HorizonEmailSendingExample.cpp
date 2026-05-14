// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonEmailSendingExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonEmailSendingManager.h"
#include "Engine/GameInstance.h"

AHorizonEmailSendingExample::AHorizonEmailSendingExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonEmailSendingExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmailExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonEmailSendingExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonEmailSendingExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[EmailExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonEmailSendingExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[EmailExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonEmailSendingExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->EmailSending)
	{
		UE_LOG(LogTemp, Error, TEXT("[EmailExample] EmailSending manager not available."));
		return;
	}

	if (RecipientUserId == TEXT("REPLACE_WITH_USER_ID"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EmailExample] Set RecipientUserId on the actor before running."));
		return;
	}

	// Email sending uses API key auth, so no sign-in is required.
	TMap<FString, FString> Variables;
	Variables.Add(TEXT("playerName"), TEXT("Example Player"));

	UE_LOG(LogTemp, Log, TEXT("[EmailExample] Sending email via template '%s'..."), *TemplateSlug);

	// Immediate send: this overload omits the ScheduledAt parameter.
	Horizon->EmailSending->SendEmail(
		RecipientUserId,
		TemplateSlug,
		Variables,
		TEXT("en"),
		FOnSendEmailComplete::CreateLambda(
			[](bool bSuccess, const FSendEmailResponse& Response)
			{
				if (!bSuccess)
				{
					UE_LOG(LogTemp, Error, TEXT("[EmailExample] FAILED to send email."));
					return;
				}

				UE_LOG(LogTemp, Log, TEXT("[EmailExample] SUCCESS: email %s status %s"),
					*Response.Id, *Response.Status);
			}));
}
