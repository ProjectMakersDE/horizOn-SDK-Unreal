// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonMarketplace.h"
#include "HorizonMarketplaceManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMarketplaceResult,bool,Success,const FHorizonMarketplaceResponse&,Response,const FString&,Code,const FString&,Message);
/** Ten explicit App routes; listing-owned receipts never become local purchase history. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonMarketplaceManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth);
    void GetLimits(const FString& User,FOnMarketplaceComplete Done);
    void GetRules(const FString& User,FOnMarketplaceComplete Done,const FString& After=TEXT(""),int32 Limit=50,int64 Revision=-1);
    void GetMarketplace(const FString& User,FOnMarketplaceComplete Done);
    void QueryListings(const FString& User,FOnMarketplaceComplete Done,const FString& ItemKey=TEXT(""),int64 MinPrice=0,int64 MaxPrice=9007199254740991LL,const FString& Cursor=TEXT(""),int32 Limit=50);
    void GetListing(const FString& User,const FString& ListingId,FOnMarketplaceComplete Done);
    void SubmitPrepared(const FHorizonMarketplaceIntent& Original,FOnMarketplaceComplete Done);
    void GetOperation(const FString& User,const FString& OperationId,FOnMarketplaceComplete Done);
    void RetryReturn(const FString& User,const FString& ListingId,FOnMarketplaceComplete Done);
    UFUNCTION(BlueprintPure,Category="horizOn|Marketplace") static FHorizonMarketplaceIntent PrepareCreate(const FString& User,const FString& OperationId,const FString& IssuedAt,const FString& InstanceId,int64 InventoryRevision,int64 ConfigRevision,int64 Price,int64 DurationSeconds);
    UFUNCTION(BlueprintPure,Category="horizOn|Marketplace") static FHorizonMarketplaceIntent PrepareBuy(const FString& User,const FString& ListingId,const FString& OperationId,const FString& IssuedAt,int64 ListingRevision,int64 ExpectedPrice,const FString& ExpectedCurrency);
    UFUNCTION(BlueprintPure,Category="horizOn|Marketplace") static FHorizonMarketplaceIntent PrepareCancel(const FString& User,const FString& ListingId,const FString& OperationId,const FString& IssuedAt,int64 ListingRevision);
    UFUNCTION(BlueprintPure,Category="horizOn|Marketplace") static bool ValidOriginal(const FHorizonMarketplaceIntent& Original){return Original.IsValid();}
    UFUNCTION(BlueprintPure,Category="horizOn|Marketplace") FHorizonMarketplaceResponse GetCurrent(const FString& Kind);
    UFUNCTION(BlueprintPure,Category="horizOn|Marketplace") int64 GetServerNow();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void ClearCache();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void ReadLimits(const FString& User);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void ReadRules(const FString& User,const FString& After,int32 Limit,int64 CatalogRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void ReadMarketplace(const FString& User);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void ReadListings(const FString& User,const FString& ItemKey,int64 MinPrice,int64 MaxPrice,const FString& Cursor,int32 Limit);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void ReadListing(const FString& User,const FString& ListingId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void SubmitOriginal(const FHorizonMarketplaceIntent& Original);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void LookupOriginal(const FString& User,const FString& OperationId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace") void RetryReturnDelivery(const FString& User,const FString& ListingId);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Marketplace") FOnHorizonMarketplaceResult OnResult;
    DECLARE_EVENT(UHorizonMarketplaceManager,FTradingStateInvalidated);
    FTradingStateInvalidated TradingStateInvalidated;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    TMap<FString,FHorizonMarketplaceResponse> Cache;
    FString Identity;
    uint64 Context=0,Epoch=0,Ticket=0;
    int64 ServerMillis=-1,StateRevision=-1,ConfigRevision=-1;
    double Clock=0;
    bool Busy=false;
    void Sync();
    void Call(const FString& User,const FString& Route,const FString& Kind,const FString& OriginalJson,FOnMarketplaceComplete Done,TFunction<bool(const FHorizonMarketplaceResponse&)> Valid);
    FOnMarketplaceComplete Event();
    static void Fail(FOnMarketplaceComplete Done,const FString& Code);
};
