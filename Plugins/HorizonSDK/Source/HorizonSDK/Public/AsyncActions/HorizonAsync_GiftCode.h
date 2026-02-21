// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_GiftCode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGiftCodeRedeemResult, const FString&, GiftData, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGiftCodeAsyncFailure, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGiftCodeValidateResult, bool, bIsValid);

/**
 * Async Blueprint node: Redeem a gift code.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_GiftCodeRedeem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnGiftCodeRedeemResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnGiftCodeAsyncFailure OnFailure;

	/** Redeem a gift code for the current user. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Redeem Gift Code"), Category = "horizOn|GiftCode")
	static UHorizonAsync_GiftCodeRedeem* RedeemGiftCode(const UObject* WorldContextObject, const FString& Code);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString GiftCode;

	void HandleResult(bool bSuccess, const FString& GiftData, const FString& Message);
};

// ============================================================

/**
 * Async Blueprint node: Validate a gift code without redeeming it.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_GiftCodeValidate : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnGiftCodeValidateResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnGiftCodeAsyncFailure OnFailure;

	/** Validate a gift code without redeeming it. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Validate Gift Code"), Category = "horizOn|GiftCode")
	static UHorizonAsync_GiftCodeValidate* ValidateGiftCode(const UObject* WorldContextObject, const FString& Code);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString GiftCode;

	void HandleResult(bool bRequestSuccess, bool bValid);
};
