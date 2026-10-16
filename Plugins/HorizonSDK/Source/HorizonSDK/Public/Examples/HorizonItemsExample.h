#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonItems.h"
#include "Models/HorizonPlayerValues.h"
#include "HorizonItemsExample.generated.h"
class UHorizonSubsystem;
UCLASS()
class HORIZONSDK_API UHorizonItemOriginal : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString Project;
    UPROPERTY() FString UserId;
    UPROPERTY() FString IntentId;
    UPROPERTY() FString Action;
    UPROPERTY() FString InstanceId;
    UPROPERTY() int64 ExpectedRevision=-1;
    UPROPERTY() TArray<FHorizonItemPropertyDelta> Changes;
    UPROPERTY() bool Equipped=false;
    UPROPERTY() FString OfferKey;
    UPROPERTY() FString OperationId;
    UPROPERTY() FString OfferFingerprint;
    UPROPERTY() int64 SendSerial=0;
    UPROPERTY() bool Confirmed=false;
};
/** Place in a world with an already signed-in SDK. Every action is explicit; nothing runs on BeginPlay.
 * Unknown mutations keep their original tuple. Confirmed purchases only retry inventory reads.
 */
UCLASS()
class HORIZONSDK_API AHorizonItemsExample : public AActor
{
    GENERATED_BODY()
public:
    /** One bounded catalog page, offers and inventory. No polling or automatic continuation. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Items Example") void Refresh();
    /** Item offer must be visible and active in the explicitly refreshed offers. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Items Example") void PurchaseItemOffer(const FString& OfferKey);
    UFUNCTION(BlueprintCallable,Category="horizOn|Items Example") void StartItemAction(const FString& Action,const FString& InstanceId,int64 ExpectedRevision,const TArray<FHorizonItemPropertyDelta>& Changes,bool Equipped);
    /** Purchase retry first reads current offers. Changed price requires a second explicit confirmation. Original ID never changes. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Items Example") void RetrySavedAction(bool ConfirmReviewedOfferChange=false);
    /** Only non-purchase CAS intents can be explicitly reconciled after a fresh authorized inventory read. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Items Example") bool AcknowledgeItemActionAfterRefresh();
    UFUNCTION(BlueprintPure,Category="horizOn|Items Example") FHorizonInventorySnapshot GetInventory() const;
private:
    friend struct FHorizonItemsNative;
    UHorizonSubsystem* Session() const;
    void ReadInventory(const FString& Slot,const FString& OriginalSignature,uint64 Context,bool FinishConfirmed);
    void Send(UHorizonItemOriginal* Original,const FString& Slot);
    bool Busy=false,Refreshed=false;
    FString RefreshedSignature,ReviewedOfferChange;
    uint64 SnapshotContext=0,SnapshotGeneration=0;
    int64 SnapshotRevision=-1;
};
