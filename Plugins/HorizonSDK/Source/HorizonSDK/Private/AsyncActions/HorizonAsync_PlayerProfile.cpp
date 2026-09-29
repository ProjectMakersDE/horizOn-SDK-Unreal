// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_PlayerProfile.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonPlayerProfileManager.h"

// ============================================================
// GetPlayerProfile
// ============================================================

UHorizonAsync_GetPlayerProfile* UHorizonAsync_GetPlayerProfile::GetPlayerProfile(const UObject* WorldContextObject)
{
	UHorizonAsync_GetPlayerProfile* Action = NewObject<UHorizonAsync_GetPlayerProfile>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_GetPlayerProfile::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->PlayerProfile)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or PlayerProfile manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->PlayerProfile->GetProfile(
		FOnPlayerProfileComplete::CreateUObject(this, &UHorizonAsync_GetPlayerProfile::HandleResult)
	);
}

void UHorizonAsync_GetPlayerProfile::HandleResult(bool bSuccess, const FHorizonPlayerProfileResult& Result,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// SetPlayerProfile
// ============================================================

UHorizonAsync_SetPlayerProfile* UHorizonAsync_SetPlayerProfile::SetPlayerProfile(const UObject* WorldContextObject,
	const FString& AvatarId, const FString& FrameId, const TArray<FString>& Badges)
{
	UHorizonAsync_SetPlayerProfile* Action = NewObject<UHorizonAsync_SetPlayerProfile>();
	Action->WorldContext = WorldContextObject;
	Action->AvatarIdStr = AvatarId;
	Action->FrameIdStr = FrameId;
	Action->BadgeIds = Badges;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_SetPlayerProfile::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->PlayerProfile)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or PlayerProfile manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->PlayerProfile->SetProfile(
		AvatarIdStr, FrameIdStr, BadgeIds,
		FOnPlayerProfileComplete::CreateUObject(this, &UHorizonAsync_SetPlayerProfile::HandleResult)
	);
}

void UHorizonAsync_SetPlayerProfile::HandleResult(bool bSuccess, const FHorizonPlayerProfileResult& Result,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}
