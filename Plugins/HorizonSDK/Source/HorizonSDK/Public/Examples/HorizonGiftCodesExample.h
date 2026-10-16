// Copyright (c) 2025-2026 horizOn. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "HorizonGiftCodesExample.generated.h"
class UHorizonSubsystem;
UCLASS()
class HORIZONSDK_API UHorizonGiftCodePendingRedeem : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString ProjectScope;
    UPROPERTY() FString UserId;
    UPROPERTY() FString Code;
    UPROPERTY() FString IdempotencyId;
};
/** Restore an existing session first. Redeeming is an explicit player action, never BeginPlay. */
UCLASS()
class HORIZONSDK_API AHorizonGiftCodesExample : public AActor
{
    GENERATED_BODY()
public:
    AHorizonGiftCodesExample();
    UPROPERTY(EditAnywhere,Category="horizOn|Example") FString GiftCode=TEXT("REPLACE_WITH_GIFT_CODE");
    UFUNCTION(BlueprintCallable,Category="horizOn|Example") void RedeemGiftCode();
    /** Reuses the original persisted code and ID after timeout/restart. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Example") void RetryPendingRedemption();
    UFUNCTION(BlueprintPure,Category="horizOn|Example") FString GetPendingIdempotencyId() const;
private:
    UHorizonSubsystem* Session() const;
    void Send(bool bRetry);
    bool bSending=false;
};
