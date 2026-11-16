// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonItemGifts.h"
#include "HorizonItemGiftsManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonItemGiftResult,bool,Success,const FHorizonItemGiftResponse&,Response,const FString&,Code,const FString&,Message);
/** Six explicit App routes. Claiming an offer or a return stays the existing Inbox claim. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonItemGiftsManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth);
    void GetConfig(const FString& User,FOnItemGiftComplete Done);
    void GetState(const FString& User,FOnItemGiftComplete Done);
    void SubmitPrepared(const FHorizonItemGiftIntent& Original,FOnItemGiftComplete Done);
    void GetOperation(const FString& User,const FString& OperationId,FOnItemGiftComplete Done);
    void GetGift(const FString& User,const FString& GiftId,FOnItemGiftComplete Done);
    void Decline(const FString& User,const FString& GiftId,int64 ExpectedGiftRevision,FOnItemGiftComplete Done);
    UFUNCTION(BlueprintPure,Category="horizOn|Item Gifts") static FHorizonItemGiftIntent PrepareSend(const FString& User,const FString& OperationId,const FString& IssuedAt,const FString& RecipientId,const FString& InstanceId,int64 InventoryRevision,int64 ConfigRevision);
    UFUNCTION(BlueprintPure,Category="horizOn|Item Gifts") static bool ValidOriginal(const FHorizonItemGiftIntent& Original){return Original.IsValid();}
    UFUNCTION(BlueprintPure,Category="horizOn|Item Gifts") FHorizonItemGiftResponse GetCurrent(const FString& Kind);
    UFUNCTION(BlueprintPure,Category="horizOn|Item Gifts") int64 GetServerNow();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void ClearCache();
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void ReadConfig(const FString& User);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void ReadState(const FString& User);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void ReadGift(const FString& User,const FString& GiftId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void SubmitOriginal(const FHorizonItemGiftIntent& Original);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void LookupOriginal(const FString& User,const FString& OperationId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Item Gifts") void DeclineGift(const FString& User,const FString& GiftId,int64 ExpectedGiftRevision);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Item Gifts") FOnHorizonItemGiftResult OnResult;
    DECLARE_EVENT(UHorizonItemGiftsManager,FGiftStateInvalidated);
    FGiftStateInvalidated GiftStateInvalidated;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    TMap<FString,FHorizonItemGiftResponse> Cache;
    FString Identity;
    uint64 Context=0,Epoch=0,Ticket=0;
    int64 ServerMillis=-1,StateRevision=-1,ConfigRevision=-1;
    double Clock=0;
    bool Busy=false;
    void Sync();
    void Call(const FString& User,const FString& Route,const FString& Kind,const FString& OriginalJson,FOnItemGiftComplete Done,TFunction<bool(const FHorizonItemGiftResponse&)> Valid);
    FOnItemGiftComplete Event();
    static void Fail(FOnItemGiftComplete Done,const FString& Code);
};
