// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonFriends.h"
#include "HorizonFriendsManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonFriendsResult,bool,bSuccess,const FHorizonFriendsSnapshot&,Snapshot,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonFriendLookupResult,bool,bSuccess,const FHorizonFriendLookup&,Result,const FString&,Code,const FString&,Message);
/** Explicit revision/request identity. A timeout never causes a newer revision to be substituted. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonFriendsManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth);
    void GetFriends(const FString& UserId,FOnFriendsComplete Complete);
    void FindPlayer(const FString& UserId,const FString& Query,FOnFriendLookupComplete Complete);
    void SendRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,FOnFriendsComplete Complete);
    void AcceptRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId,FOnFriendsComplete Complete);
    void DeclineRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId,FOnFriendsComplete Complete);
    void CancelRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId,FOnFriendsComplete Complete);
    void RemoveFriend(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,FOnFriendsComplete Complete);
    void BlockPlayer(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,FOnFriendsComplete Complete);
    void UnblockPlayer(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,FOnFriendsComplete Complete);
    UFUNCTION(BlueprintPure,Category="horizOn|Friends") FHorizonFriendsSnapshot GetCurrentFriends();
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void ClearCache();
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void RequestFriends(const FString& UserId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void FindFriendPlayer(const FString& UserId,const FString& Query);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void SendFriendRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void AcceptFriendRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void DeclineFriendRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void CancelFriendRequest(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision,const FString& RequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void RemoveFriendPlayer(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void BlockFriendPlayer(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Friends") void UnblockFriendPlayer(const FString& UserId,const FString& TargetUserId,int64 ExpectedRevision);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Friends") FOnHorizonFriendsResult OnFriendsResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Friends") FOnHorizonFriendLookupResult OnLookupResult;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    FString Identity;
    uint64 ContextEpoch=0,Ticket=0;
    int64 Epoch=0,Revision=-1,ServerMillis=-1;
    bool bBusy=false;
    FHorizonFriendsSnapshot Current;
    void SyncIdentity();
    FString Begin(const FString& UserId);
    void Call(const FString& UserId,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Complete,bool Mutation=false);
    void Mutate(const FString& UserId,const FString& Target,int64 ExpectedRevision,const FString& RequestId,const FString& Action,FOnFriendsComplete Complete);
    void Receive(const FString& UserId,const FHorizonNetworkResponse& Response,FOnFriendsComplete Complete);
    bool Adopt(const FString& UserId,const FHorizonFriendsSnapshot& Snapshot);
    static int64 TimeMillis(const FString& Value);
    static FHorizonNetworkResponse Error(const FString& Code,const FString& Message);
};
