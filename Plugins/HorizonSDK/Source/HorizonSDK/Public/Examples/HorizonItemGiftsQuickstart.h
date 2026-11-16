// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Examples/HorizonItemGiftsExample.h"
#include "HorizonItemGiftsQuickstart.generated.h"
/** Use an ItemGiftsExample already bound to the existing session and stable project UUID. */
UCLASS()
class HORIZONSDK_API UHorizonItemGiftsQuickstart : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Quickstart") static void ReadOneGift(UHorizonItemGiftsExample* Screen,const FString& GiftId)
    {if(Screen)Screen->ReadGift(GiftId);}
    /** Call only from the explicit Send confirmation after showing the reviewed item, recipient and fee. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Quickstart") static bool SendReviewedGift(UHorizonItemGiftsExample* Screen,const FString& RecipientId,const FString& InstanceId)
    {if(!Screen||!Screen->ReviewSend(RecipientId,InstanceId))return false;Screen->ConfirmReviewedSend();return Screen->Busy;}
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts Quickstart") static void ClaimGift(UHorizonItemGiftsExample* Screen,const FString& EntryId)
    {if(Screen)Screen->ClaimGiftEntry(EntryId);}
    // After uncertainty call Screen->RetryPending() / RecoverOriginalOperation(), never prepare a new send.
};
