// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Models/HorizonItemGifts.h"
#include "HorizonItemGiftsExample.generated.h"
class UHorizonSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHorizonItemGiftAction,bool,Success,const FString&,Code);
/** Create this object for the game's gift screen and Bind its existing authenticated subsystem. No automatic HTTP. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonItemGiftsExample : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") bool Bind(UHorizonSubsystem* Session,const FString& StableProjectId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void ReadConfig();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void ReadState();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void ReadInventory();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void ReadGift(const FString& GiftId);
    /** Needs a current config read and inventory read. Shows the fee before the player confirms. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") bool ReviewSend(const FString& RecipientId,const FString& InstanceId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void ConfirmReviewedSend();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void RetryPending();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void RecoverOriginalOperation();
    /** Uses the revision of the offer from the current state or point read. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") bool DeclineGift(const FString& GiftId);
    /** Offers and returns are claimed through the existing Inbox claim of the same entry. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Example") void ClaimGiftEntry(const FString& EntryId);
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts Example") int64 ReviewedFee=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts Example") FString ReviewedCurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts Example") FString LastErrorCode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts Example") bool Busy=false;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Item Gifts Example") FHorizonItemGiftAction OnAction;
private:
    friend struct FHorizonItemGiftsNative;
    UPROPERTY() UHorizonSubsystem* Bound=nullptr;
    FString Project,ReviewRecipient,ReviewInstance,ReviewUser;
    int64 ReviewInventory=0,ReviewConfig=0;
    uint64 ReviewContext=0;
    bool Reviewed=false;
    UHorizonSubsystem* Session() const;
    void Resume(bool Lookup);
    void Apply(const TSharedPtr<struct FHorizonItemGiftFlow>& Flow,bool Lookup);
};
