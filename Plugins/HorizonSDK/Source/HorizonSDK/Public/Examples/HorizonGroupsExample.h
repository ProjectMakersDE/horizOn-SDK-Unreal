#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonGroups.h"
#include "HorizonGroupsExample.generated.h"
class UHorizonSubsystem;
/** The exact original action survives an unknown outcome, actor recreation and regional host changes. */
UCLASS()
class HORIZONSDK_API UHorizonGroupPendingAction : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString Project;
    UPROPERTY() FString UserId;
    UPROPERTY() FString IntentId;
    UPROPERTY() FString Action;
    UPROPERTY() FString GroupId;
    UPROPERTY() FString TargetUserId;
    UPROPERTY() FString RequestId;
    UPROPERTY() FString Role;
    UPROPERTY() int64 ExpectedRevision=-1;
    UPROPERTY() int64 SendSerial=0;
    UPROPERTY() FHorizonGroupMetadata Metadata;
};
/** Existing player session only. For two players use separate game instances/clients. No signup, polling or automatic writes. */
UCLASS()
class HORIZONSDK_API AHorizonGroupsExample : public AActor
{
    GENERATED_BODY()
public:
    /** One explicit refresh: bounded group detail/pending plus own memberships. Empty group reads memberships only. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups Example") void Refresh(const FString& GroupId);
    /** create/update or one documented route action, using the revision/request ID the player reviewed. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups Example") void StartAction(const FString& Action,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,const FString& NewRole,const FHorizonGroupMetadata& Metadata);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups Example") void RetrySavedAction();
    /** Explicit human reconciliation of the EXACT original after refresh. Does not cancel an in-flight server operation. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups Example") bool AcknowledgePendingAfterRefresh();
    UFUNCTION(BlueprintPure,Category="horizOn|Groups Example") FHorizonMyGroupsView GetGroups() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Groups Example") FHorizonGroupView GetGroup() const;
private:
    friend struct FHorizonConfiguredNative;
    friend struct FHorizonGroupsNative;
    UHorizonSubsystem* Session() const;
    void ReadOwn(const FString& Project,const FString& User,const FString& Slot,const FString& Intent,int64 Serial,uint64 Context);
    void Send(UHorizonGroupPendingAction* Pending,const FString& Slot);
    bool bSending=false,bRefreshing=false,bRefreshed=false;
    uint64 SnapshotContext=0,SnapshotGeneration=0;
    FString RefreshedIntent;
    int64 RefreshedSerial=-1,SnapshotPlayerRevision=-1;
    FHorizonGroupView CurrentGroup;
    FHorizonGroupPendingSnapshot PendingSnapshot;
};
