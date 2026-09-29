// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonPlayerProfile.h"
#include "HorizonAsync_PlayerProfile.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerProfileAsyncSuccess, const FHorizonPlayerProfileResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerProfileAsyncFailure, const FString&, ErrorCode, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Load profile, unlocks and cosmetics catalog of the signed-in player.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_GetPlayerProfile : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerProfileAsyncSuccess OnSuccess;

	/** ErrorCode is the server code (for example COSMETIC_LOCKED) or SESSION_REQUIRED, RATE_LIMITED, ... */
	UPROPERTY(BlueprintAssignable)
	FOnPlayerProfileAsyncFailure OnFailure;

	/** Load the signed-in player's profile, unlocks and the cosmetics catalog. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Player Profile"), Category = "horizOn|PlayerProfile")
	static UHorizonAsync_GetPlayerProfile* GetPlayerProfile(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;

	void HandleResult(bool bSuccess, const FHorizonPlayerProfileResult& Result, const FString& ErrorCode, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node: Replace the signed-in player's avatar, frame and badges.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_SetPlayerProfile : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerProfileAsyncSuccess OnSuccess;

	/** ErrorCode is the server code (for example COSMETIC_LOCKED) or SESSION_REQUIRED, INVALID_BADGES, ... */
	UPROPERTY(BlueprintAssignable)
	FOnPlayerProfileAsyncFailure OnFailure;

	/**
	 * Replace the whole visible profile. Pass the current values for slots you keep.
	 * An empty AvatarId or FrameId clears the slot, an empty Badges array clears the badges (max 3).
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Set Player Profile"), Category = "horizOn|PlayerProfile")
	static UHorizonAsync_SetPlayerProfile* SetPlayerProfile(const UObject* WorldContextObject, const FString& AvatarId, const FString& FrameId, const TArray<FString>& Badges);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString AvatarIdStr;
	FString FrameIdStr;
	TArray<FString> BadgeIds;

	void HandleResult(bool bSuccess, const FHorizonPlayerProfileResult& Result, const FString& ErrorCode, const FString& ErrorMessage);
};
