#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Examples/HorizonMarketplaceExample.h"
#include "HorizonMarketplaceQuickstart.generated.h"
/** Use a MarketplaceExample already bound to the existing session and stable project UUID. */
UCLASS()
class HORIZONSDK_API UHorizonMarketplaceQuickstart : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Quickstart") static void ReadOneListing(UHorizonMarketplaceExample* Screen,const FString& ListingId)
    {if(Screen)Screen->ReadListing(ListingId);}
    /** Call only from the explicit Buy confirmation after displaying a successful point read. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Marketplace Quickstart") static bool BuyReviewedListing(UHorizonMarketplaceExample* Screen,const FString& ListingId)
    {if(!Screen||!Screen->ReviewBuy(ListingId))return false;Screen->ConfirmReviewedTrade();return Screen->Busy;}
    // After uncertainty call Screen->RetryPending() / RecoverOriginalOperation(), never prepare a new trade.
};
