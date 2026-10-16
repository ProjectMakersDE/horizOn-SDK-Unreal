#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonItems.h"
#include "HorizonAsync_Items.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnItemsAsyncResult,bool,bSuccess,const FHorizonInventorySnapshot&,Snapshot,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnItemCatalogAsyncResult,bool,bSuccess,const FHorizonItemCatalogPage&,Page,const FString&,ErrorCode,const FString&,ErrorMessage);
/** Factories freeze every original argument; Activate sends at most once. Existing session only. */
UCLASS()
class HORIZONSDK_API UHorizonAsync_ItemsInventory : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnItemsAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Items") static UHorizonAsync_ItemsInventory* GetInventory(const UObject* WorldContextObject);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Items") static UHorizonAsync_ItemsInventory* AdjustProperties(const UObject* WorldContextObject,const FString& InstanceId,int64 ExpectedRevision,const TArray<FHorizonItemPropertyDelta>& Changes);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Items") static UHorizonAsync_ItemsInventory* SetEquipped(const UObject* WorldContextObject,const FString& InstanceId,int64 ExpectedRevision,bool Equipped);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Items") static UHorizonAsync_ItemsInventory* DiscardItem(const UObject* WorldContextObject,const FString& InstanceId,int64 ExpectedRevision);
    virtual void Activate() override;
private:
    static UHorizonAsync_ItemsInventory* Create(const UObject* World,const FString& Action,const FString& Instance,int64 Revision,const TArray<FHorizonItemPropertyDelta>& Changes,bool Equipped);
    TWeakObjectPtr<const UObject> WorldContext;
    FString User,Action,Instance;uint64 Context=0;
    int64 Revision=0;bool Equipped=false,bActivated=false;
    TArray<FHorizonItemPropertyDelta> Changes;
    void Complete(bool Ok,const FHorizonInventorySnapshot& S,const FString& C,const FString& M);
};
UCLASS()
class HORIZONSDK_API UHorizonAsync_ItemCatalog : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnItemCatalogAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Items") static UHorizonAsync_ItemCatalog* GetCatalog(const UObject* WorldContextObject,const FString& AfterKey="",int32 Limit=50,int64 CatalogRevision=-1);
    virtual void Activate() override;
private:
    TWeakObjectPtr<const UObject> WorldContext;
    FString User,After;uint64 Context=0;int32 Limit=50;int64 Revision=-1;bool bActivated=false;
    void Complete(bool Ok,const FHorizonItemCatalogPage& P,const FString& C,const FString& M);
};
