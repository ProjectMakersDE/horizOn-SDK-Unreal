// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_GiftCode.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonGiftCodeManager.h"

// ============================================================
// RedeemGiftCode
// ============================================================

UHorizonAsync_GiftCodeRedeem* UHorizonAsync_GiftCodeRedeem::RedeemGiftCode(
	const UObject* WorldContextObject, const FString& Code)
{
	UHorizonAsync_GiftCodeRedeem* Action = NewObject<UHorizonAsync_GiftCodeRedeem>();
	Action->WorldContext = WorldContextObject;
	Action->GiftCode = Code;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_GiftCodeRedeem::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->GiftCodes)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or GiftCode manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->GiftCodes->Redeem(
		GiftCode,
		FOnGiftCodeRedeemComplete::CreateUObject(this, &UHorizonAsync_GiftCodeRedeem::HandleResult)
	);
}

void UHorizonAsync_GiftCodeRedeem::HandleResult(bool bSuccess, const FString& GiftData, const FString& Message)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(GiftData, Message);
	}
	else
	{
		OnFailure.Broadcast(Message);
	}
	SetReadyToDestroy();
}

// ============================================================
// ValidateGiftCode
// ============================================================

UHorizonAsync_GiftCodeValidate* UHorizonAsync_GiftCodeValidate::ValidateGiftCode(
	const UObject* WorldContextObject, const FString& Code)
{
	UHorizonAsync_GiftCodeValidate* Action = NewObject<UHorizonAsync_GiftCodeValidate>();
	Action->WorldContext = WorldContextObject;
	Action->GiftCode = Code;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_GiftCodeValidate::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->GiftCodes)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or GiftCode manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->GiftCodes->Validate(
		GiftCode,
		FOnGiftCodeValidateComplete::CreateUObject(this, &UHorizonAsync_GiftCodeValidate::HandleResult)
	);
}

void UHorizonAsync_GiftCodeValidate::HandleResult(bool bRequestSuccess, bool bValid)
{
	if (bRequestSuccess)
	{
		OnSuccess.Broadcast(bValid);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to validate gift code."));
	}
	SetReadyToDestroy();
}
