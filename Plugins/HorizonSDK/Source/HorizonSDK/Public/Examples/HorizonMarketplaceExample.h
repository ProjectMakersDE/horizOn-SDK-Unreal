#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Models/HorizonMarketplace.h"
#include "HorizonMarketplaceExample.generated.h"
class UHorizonSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHorizonMarketplaceAction,bool,Success,const FString&,Code);
/** Create this object for the game's marketplace screen and Bind its existing authenticated subsystem. No automatic HTTP. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonMarketplaceExample : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") bool Bind(UHorizonSubsystem* Session,const FString& StableProjectId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ReadLimits();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ReadRulesPage(const FString& After,int64 CatalogRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ReadMarketplace();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ReadListings(const FString& ItemKey,int64 MinPrice,int64 MaxPrice,const FString& Cursor);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ReadListing(const FString& ListingId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ReadInventory();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") bool ReviewCreate(const FString& InstanceId,int64 Price,int64 DurationSeconds);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") bool ReviewBuy(const FString& ListingId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") bool ReviewCancel(const FString& ListingId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ConfirmReviewedTrade();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void RetryPending();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void RecoverOriginalOperation();
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void RetryReturnDelivery(const FString& ListingId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Example") void ClaimReturn(const FString& EntryId);
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace Example") int64 ReviewedPrice=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace Example") int64 ReviewedFee=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace Example") int64 ReviewedSellerNet=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace Example") FString LastErrorCode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace Example") bool Busy=false;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Marketplace Example") FHorizonMarketplaceAction OnAction;
private:
    friend struct FHorizonMarketplaceNative;
    UPROPERTY() UHorizonSubsystem* Bound=nullptr;
    FString Project,ReviewKind,ReviewListingId,ReviewInstance,ReviewCurrency,ReviewUser;
    int64 ReviewInventory=0,ReviewConfig=0,ReviewRevision=0,ReviewPrice=0,ReviewDuration=0;
    uint64 ReviewContext=0;
    UHorizonSubsystem* Session() const;
    bool ReviewListing(const FString& Id,bool Cancel);
    void Resume(bool Lookup);
    void Apply(const TSharedPtr<struct FHorizonMarketplaceFlow>& Flow,bool Lookup);
};
