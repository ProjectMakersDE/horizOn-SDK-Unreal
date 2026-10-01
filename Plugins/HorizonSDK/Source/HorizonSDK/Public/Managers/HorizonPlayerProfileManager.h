// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonPlayerProfile.h"
#include "HorizonPlayerProfileManager.generated.h"

/**
 * Completion of GetProfile / SetProfile.
 * On failure ErrorCode is the server `code` (for example "COSMETIC_LOCKED"),
 * "SESSION_REQUIRED" / "INVALID_BADGES" / "INVALID_COSMETIC_ID" for a local
 * pre-check, or the HTTP status mapping ("RATE_LIMITED", "CONNECTION_FAILED", ...)
 * when the server sent no code. Both strings are empty on success.
 */
DECLARE_DELEGATE_FourParams(FOnPlayerProfileComplete, bool /*bSuccess*/, const FHorizonPlayerProfileResult& /*Result*/, const FString& /*ErrorCode*/, const FString& /*ErrorMessage*/);

/**
 * Player Profile Manager for the horizOn SDK (TASK-881).
 *
 * Reads and sets the signed-in player's avatar, frame and badges, and returns
 * the player's unlocks plus the cosmetics catalog of the API key. Both calls
 * need a signed-in player and send the player session (Authorization: Bearer).
 * The last result is cached in memory (no time based cache) and cleared on
 * sign-in, sign-out and after a gift code redeem that granted unlocks.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonPlayerProfileManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/**
	 * Load profile, unlocks and catalog of the signed-in player.
	 * Sends GET /api/v1/app/player-profile?userId=<current user>.
	 * @param OnComplete Called with (bSuccess, Result, ErrorCode, ErrorMessage).
	 */
	void GetProfile(FOnPlayerProfileComplete OnComplete);

	/**
	 * Replace the whole visible profile of the signed-in player.
	 * Pass the current values for slots you do not want to change.
	 * Sends PUT /api/v1/app/player-profile.
	 * @param AvatarId   Avatar to show; empty clears the slot.
	 * @param FrameId    Frame to show; empty clears the slot.
	 * @param Badges     Badges to show (at most 3, no duplicates, order kept); empty clears them.
	 * @param OnComplete Called with (bSuccess, Result, ErrorCode, ErrorMessage).
	 */
	void SetProfile(const FString& AvatarId, const FString& FrameId, const TArray<FString>& Badges, FOnPlayerProfileComplete OnComplete);

	/** Last successful result (empty when HasCurrentProfile() is false). */
	const FHorizonPlayerProfileResult& GetCurrentProfile() const { return CurrentProfile; }

	/** True once GetProfile or SetProfile succeeded for the signed-in player. */
	UFUNCTION(BlueprintPure, Category = "horizOn|PlayerProfile")
	bool HasCurrentProfile() const { return bHasCurrentProfile; }

	/** Drop the cached result. The next GetProfile loads fresh data. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|PlayerProfile")
	void ClearCache();

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;

	FHorizonPlayerProfileResult CurrentProfile;
	bool bHasCurrentProfile = false;

	/** Shared success / failure handling of both calls. */
	void HandleResponse(const FHorizonNetworkResponse& Response, const FString& RequestUserId,
		const TCHAR* Operation, const FOnPlayerProfileComplete& OnComplete);

	UFUNCTION()
	void HandleUserSignedIn();

	UFUNCTION()
	void HandleUserSignedOut();
};
