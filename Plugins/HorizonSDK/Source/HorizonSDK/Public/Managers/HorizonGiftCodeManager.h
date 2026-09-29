// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "HorizonGiftCodeManager.generated.h"

class UHorizonPlayerProfileManager;

DECLARE_DELEGATE_ThreeParams(FOnGiftCodeRedeemComplete, bool /*bSuccess*/, const FString& /*GiftData*/, const FString& /*Message*/);
DECLARE_DELEGATE_TwoParams(FOnGiftCodeValidateComplete, bool /*bRequestSuccess*/, bool /*bValid*/);

/**
 * Gift Code Manager for the horizOn SDK.
 *
 * Allows players to validate and redeem gift codes.
 * Requires the user to be signed in.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonGiftCodeManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/**
	 * Redeem a gift code for the current user.
	 * @param Code       The gift code to redeem.
	 * @param OnComplete Called with (bSuccess, GiftData, Message).
	 */
	void Redeem(const FString& Code, FOnGiftCodeRedeemComplete OnComplete);

	/**
	 * Cosmetic IDs the player owns through the last successful redeem (`grantedUnlocks`,
	 * TASK-881): the `grants` of the code's giftData. Empty when the code had no grants.
	 * Read it in the Redeem callback; after a non-empty grant the player profile cache
	 * is cleared, so the next PlayerProfile->GetProfile shows the unlock.
	 */
	const TArray<FString>& GetLastGrantedUnlocks() const { return LastGrantedUnlocks; }

	/** Called by UHorizonSubsystem: lets a redeem with grants clear the profile cache. */
	void SetPlayerProfileManager(UHorizonPlayerProfileManager* InPlayerProfileManager);

	/**
	 * Validate a gift code without redeeming it.
	 * @param Code       The gift code to validate.
	 * @param OnComplete Called with (bRequestSuccess, bValid).
	 */
	void Validate(const FString& Code, FOnGiftCodeValidateComplete OnComplete);

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;

	UPROPERTY()
	UHorizonPlayerProfileManager* PlayerProfileManager = nullptr;

	TArray<FString> LastGrantedUnlocks;
};
