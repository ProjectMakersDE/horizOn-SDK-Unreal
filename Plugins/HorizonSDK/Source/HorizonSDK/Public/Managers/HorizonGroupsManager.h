// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonGroups.h"
#include "HorizonGroupsManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonGroupsResult,bool,bSuccess,const FHorizonMyGroupsView&,Result,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonGroupResult,bool,bSuccess,const FHorizonGroupView&,Result,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonGroupMembersResult,bool,bSuccess,const FHorizonGroupMembersView&,Result,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonGroupPendingResult,bool,bSuccess,const FHorizonGroupPendingSnapshot&,Result,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonGroupMutationResult,bool,bSuccess,const FHorizonGroupMutationView&,Result,const FString&,Code,const FString&,Message);
/** Bounded, explicit reads. Every write sends one HTTP attempt with the caller's original revision. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonGroupsManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth);
    void GetGroups(const FString& UserId,FOnGroupsComplete Complete);
    void GetGroup(const FString& UserId,const FString& GroupId,FOnGroupComplete Complete);
    void GetMembers(const FString& UserId,const FString& GroupId,FOnGroupMembersComplete Complete,const FString& AfterUserId=TEXT(""),int32 Limit=50);
    void GetPending(const FString& UserId,const FString& GroupId,FOnGroupPendingComplete Complete);
    void CreateGroup(const FString& UserId,int64 ExpectedPlayerRevision,const FHorizonGroupMetadata& Metadata,FOnGroupMutationComplete Complete);
    void UpdateGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FHorizonGroupMetadata& Metadata,FOnGroupMutationComplete Complete);
    void JoinGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    void RequestJoin(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    void InvitePlayer(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    void AcceptInvite(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete);
    void DeclineInvite(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete);
    void CancelInvite(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete);
    void AcceptRequest(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete);
    void DeclineRequest(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete);
    void CancelRequest(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId,FOnGroupMutationComplete Complete);
    void LeaveGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    void RemoveMember(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    void SetMemberRole(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& Role,FOnGroupMutationComplete Complete);
    void TransferLeadership(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    void DeleteGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,FOnGroupMutationComplete Complete);
    UFUNCTION(BlueprintPure,Category="horizOn|Groups") FHorizonMyGroupsView GetCurrentGroups();
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void ClearCache();
    uint64 GetCacheGeneration();
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") static bool ParseGroupData(const FString& Json,FHorizonGroupJsonData& Data);
    UFUNCTION(BlueprintPure,Category="horizOn|Groups") static FString GroupDataJson(const FHorizonGroupJsonData& Data);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void RequestGroups(const FString& UserId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void RequestGroup(const FString& UserId,const FString& GroupId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void RequestGroupMembers(const FString& UserId,const FString& GroupId,const FString& AfterUserId,int32 Limit);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void RequestGroupPending(const FString& UserId,const FString& GroupId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void CreatePlayerGroup(const FString& UserId,int64 ExpectedPlayerRevision,const FHorizonGroupMetadata& Metadata);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void UpdatePlayerGroup(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FHorizonGroupMetadata& Metadata);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void JoinGroupAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void RequestJoinAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void InvitePlayerAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void AcceptInviteAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void DeclineInviteAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void CancelInviteAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void AcceptRequestAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void DeclineRequestAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& OriginalRequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void CancelRequestAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision,const FString& OriginalRequestId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void LeaveGroupAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void RemoveMemberAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void SetMemberRoleAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision,const FString& Role);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void TransferLeadershipAction(const FString& UserId,const FString& GroupId,const FString& TargetUserId,int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Groups") void DeleteGroupAction(const FString& UserId,const FString& GroupId,int64 ExpectedRevision);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Groups") FOnHorizonGroupsResult OnGroupsResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Groups") FOnHorizonGroupResult OnGroupResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Groups") FOnHorizonGroupMembersResult OnGroupMembersResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Groups") FOnHorizonGroupPendingResult OnGroupPendingResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Groups") FOnHorizonGroupMutationResult OnGroupMutationResult;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    FString Identity;
    uint64 Context=0,Epoch=0,Ticket=0;
    bool bBusy=false;
    int64 PlayerRevision=-1,ServerMillis=-1;
    TMap<FString,int64> GroupRevisions;
    FHorizonMyGroupsView Current;
    void Sync();
    void Call(const FString& User,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Complete,bool Put=false);
    bool CheckGroup(const FHorizonGroupView& Group) const;
    bool CheckSnapshot(const FString& User,const FHorizonMyGroupsView& Snapshot) const;
    bool CheckRevision(const FString& Group,int64 Revision) const;
    void Remember(const FHorizonGroupView& Group);
    void RecordRevision(const FString& Group,int64 Revision);
    void Adopt(const FHorizonMyGroupsView& Snapshot);
    void ReceiveMutation(const FString& User,const FString& GroupId,const FHorizonNetworkResponse& Response,FOnGroupMutationComplete Complete);
    void Metadata(const FString& User,const FString& Group,int64 Expected,const FHorizonGroupMetadata& Value,bool Create,FOnGroupMutationComplete Complete);
    void Action(const FString& User,const FString& Group,const FString& Target,int64 Expected,const FString& Request,const FString& Role,const FString& Action,FOnGroupMutationComplete Complete);
    FOnGroupMutationComplete MutationEvent();
    static FHorizonNetworkResponse Error(const FString& Code);
};
