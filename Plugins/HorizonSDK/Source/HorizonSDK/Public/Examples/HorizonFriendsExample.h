// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonFriends.h"
#include "HorizonFriendsExample.generated.h"
class UHorizonSubsystem;
UCLASS()
class HORIZONSDK_API UHorizonFriendPendingAction : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString ProjectScope;
    UPROPERTY() FString UserId;
    UPROPERTY() FString Action;
    UPROPERTY() FString TargetUserId;
    UPROPERTY() int64 ExpectedRevision=-1;
    UPROPERTY() FString RequestId;
    UPROPERTY() FString IntentId;
    UPROPERTY() int64 SendSerial=0;
};
/** Existing session only. All reads/actions are explicit player UI events. */
UCLASS()
class HORIZONSDK_API AHorizonFriendsExample : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends Example") void LoadFriends();
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends Example") void FindPlayer(const FString& ExactQuery);
    /** Action=request/accept/decline/cancel/remove/block/unblock. Revision/request ID are reviewed UI inputs. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends Example") void StartAction(const FString& Action,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends Example") void RetrySavedAction();
    /** Explicit acknowledgement AFTER refresh; does not cancel a server operation still running. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends Example") bool AcknowledgePendingAfterRefresh();
    UFUNCTION(BlueprintPure,Category="horizOn|Friends Example") FHorizonFriendsSnapshot GetCurrentFriends() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Friends Example") FHorizonFriendLookup GetLastLookup() const;
private:
    friend struct FHorizonFriendsNative;
    UHorizonSubsystem* Session() const;
    void Send(UHorizonFriendPendingAction* Pending,const FString& Slot);
    bool bSending=false,bRefreshed=false;
    uint64 RefreshedContext=0,LookupContext=0;
    FString RefreshedIntentId;
    int64 RefreshedSendSerial=-1;
    FHorizonFriendLookup LastLookup;
};
