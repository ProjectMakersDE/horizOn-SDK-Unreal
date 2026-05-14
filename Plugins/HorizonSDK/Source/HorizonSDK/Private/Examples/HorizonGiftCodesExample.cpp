// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonGiftCodesExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Engine/GameInstance.h"

AHorizonGiftCodesExample::AHorizonGiftCodesExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonGiftCodesExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[GiftCodesExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonGiftCodesExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonGiftCodesExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[GiftCodesExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonGiftCodesExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[GiftCodesExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonGiftCodesExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->GiftCodes)
	{
		UE_LOG(LogTemp, Error, TEXT("[GiftCodesExample] Auth or GiftCodes manager not available."));
		return;
	}

	if (GiftCode == TEXT("REPLACE_WITH_GIFT_CODE"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GiftCodesExample] Set GiftCode on the actor before running."));
		return;
	}

	UHorizonGiftCodeManager* GiftCodes = Horizon->GiftCodes;
	const FString Code = GiftCode;

	// Redeeming requires a signed-in user.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[GiftCodes, Code](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[GiftCodesExample] FAILED: sign-up did not complete."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[GiftCodesExample] Validating code..."));

			GiftCodes->Validate(Code, FOnGiftCodeValidateComplete::CreateLambda(
				[GiftCodes, Code](bool bRequestSuccess, bool bValid)
				{
					if (!bRequestSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[GiftCodesExample] FAILED to validate code."));
						return;
					}
					if (!bValid)
					{
						UE_LOG(LogTemp, Warning, TEXT("[GiftCodesExample] Code is not valid; skipping redeem."));
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[GiftCodesExample] Code is valid. Redeeming..."));

					GiftCodes->Redeem(Code, FOnGiftCodeRedeemComplete::CreateLambda(
						[](bool bSuccess, const FString& GiftData, const FString& Message)
						{
							if (!bSuccess)
							{
								UE_LOG(LogTemp, Error, TEXT("[GiftCodesExample] FAILED to redeem: %s"), *Message);
								return;
							}

							UE_LOG(LogTemp, Log, TEXT("[GiftCodesExample] SUCCESS: %s data %s"),
								*Message, *GiftData);
						}));
				}));
		}));
}
