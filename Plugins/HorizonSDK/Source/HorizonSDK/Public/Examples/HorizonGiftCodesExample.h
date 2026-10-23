// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonGiftCodes.h"
#include "HorizonGiftCodesExample.generated.h"
class UHorizonSubsystem;
UCLASS()
class HORIZONSDK_API UHorizonGiftCodePendingRedeem : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FHorizonGiftCodeIntent Intent;
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
    /** Reuses the original persisted V2 tuple after timeout/restart. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Example") void RetryPendingRedemption();
    UFUNCTION(BlueprintPure,Category="horizOn|Example") FHorizonGiftCodeIntent GetPendingIntent() const;
private:
    UHorizonSubsystem* Session() const;
    void Send(bool bRetry);
    void Dispatch(UHorizonSubsystem* SDK,const FHorizonGiftCodeIntent& Intent,const FString& Slot);
    bool bSending=false;
};
