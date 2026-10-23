// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonItems.h"
#include "HorizonItemsManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonInventoryResult,bool,bSuccess,const FHorizonInventorySnapshot&,Inventory,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonItemCatalogResult,bool,bSuccess,const FHorizonItemCatalogPage&,Page,const FString&,Code,const FString&,Message);
/** Explicit bounded reads and one-attempt revisioned mutations. Never grants or synthesizes an item. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonItemsManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth);
    void GetInventory(const FString& UserId,FOnInventoryComplete Complete);
    /** -1 omits the first-page catalog revision; a continuation requires its original revision. */
    void GetCatalog(const FString& UserId,FOnItemCatalogComplete Complete,const FString& AfterKey=TEXT(""),int32 Limit=50,int64 CatalogRevision=-1);
    void AdjustProperties(const FString& UserId,const FString& InstanceId,int64 ExpectedRevision,const TArray<FHorizonItemPropertyDelta>& Changes,FOnInventoryComplete Complete);
    void SetEquipped(const FString& UserId,const FString& InstanceId,int64 ExpectedRevision,bool bEquipped,FOnInventoryComplete Complete);
    void DiscardItem(const FString& UserId,const FString& InstanceId,int64 ExpectedRevision,FOnInventoryComplete Complete);
    UFUNCTION(BlueprintPure,Category="horizOn|Items") FHorizonInventorySnapshot GetCurrentInventory();
    UFUNCTION(BlueprintPure,Category="horizOn|Items") FHorizonItemCatalogPage GetCurrentCatalog();
    UFUNCTION(BlueprintCallable,Category="horizOn|Items") void ClearCache();
    uint64 GetCacheGeneration();
    UFUNCTION(BlueprintCallable,Category="horizOn|Items") void RequestInventory(const FString& UserId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Items") void RequestCatalog(const FString& UserId,const FString& AfterKey,int32 Limit,int64 CatalogRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Items") void AdjustItemProperties(const FString& UserId,const FString& InstanceId,int64 ExpectedRevision,const TArray<FHorizonItemPropertyDelta>& Changes);
    UFUNCTION(BlueprintCallable,Category="horizOn|Items") void EquipItem(const FString& UserId,const FString& InstanceId,int64 ExpectedRevision,bool bEquipped);
    /** Destructive removal; only invoke from an explicit reviewed player action. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Items") void DiscardInventoryItem(const FString& UserId,const FString& InstanceId,int64 ExpectedRevision);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Items") FOnHorizonInventoryResult OnInventoryResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Items") FOnHorizonItemCatalogResult OnCatalogResult;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    FString Identity;
    uint64 Context=0,Epoch=0,Ticket=0;
    int64 InventoryRevision=-1,CatalogRevisionFloor=-1,ServerMillis=-1;
    bool bBusy=false;
    FHorizonInventorySnapshot Inventory;
    FHorizonItemCatalogPage Catalog;
    void Sync();
    void Call(const FString& User,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Complete);
    void ReceiveInventory(const FString& User,const FHorizonNetworkResponse& Response,FOnInventoryComplete Complete);
    void Mutate(const FString& User,const FString& Instance,int64 Revision,const FString& Action,const TArray<FHorizonItemPropertyDelta>& Changes,bool Equipped,FOnInventoryComplete Complete);
    FOnInventoryComplete InventoryEvent();
    static FHorizonNetworkResponse Error(const FString& Code);
};
