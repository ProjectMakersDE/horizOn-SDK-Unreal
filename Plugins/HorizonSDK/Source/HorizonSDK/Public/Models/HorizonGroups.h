#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonFriends.h"
#include "HorizonGroups.generated.h"
class FJsonObject;
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupJsonData
{
    GENERATED_BODY()
public:
    static bool TryFromJson(const FString& Raw,FHorizonGroupJsonData& Out);
    const FString& GetJson() const { return Json; }
private:
    UPROPERTY() FString Json=TEXT("{}");
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupMetadata
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Groups") FString Name;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Groups") FString Description;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Groups") FString JoinMode=TEXT("open");
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Groups") FHorizonGroupJsonData Data;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString Name;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString Description;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString JoinMode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString LeaderId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MemberCount=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FHorizonGroupJsonData Data;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasData=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString MyRole;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasMyRole=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString UpdatedAt;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGroupView& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupPending
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString RequestId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString IssuerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasCreatedAt=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupPendingView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString GroupId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString RequestId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString IssuerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasCreatedAt=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FHorizonGroupView Group;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasGroup=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxMembers=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxGroupsPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxInvitations=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxJoinRequests=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxDataBytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MemberPageSize=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxPlayerInvitations=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxPlayerRequests=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxNameBytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxDescriptionBytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int32 MaxDataDepth=0;

};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMyGroupsView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") TArray<FHorizonGroupView> Groups;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") TArray<FHorizonGroupPendingView> IncomingInvites;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") TArray<FHorizonGroupPendingView> OutgoingRequests;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FHorizonGroupLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMyGroupsView& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupMutationView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FHorizonMyGroupsView Groups;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FHorizonGroupView Group;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasGroup=false;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGroupMutationView& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupMemberView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString Role;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString JoinedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FHorizonFriendDisplay Display;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupMembersView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString GroupId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") TArray<FHorizonGroupMemberView> Members;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString NextAfterUserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") bool bHasNextAfterUserId=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGroupMembersView& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGroupPendingSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString GroupId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") TArray<FHorizonGroupPending> Invitations;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") TArray<FHorizonGroupPending> JoinRequests;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Groups") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGroupPendingSnapshot& Out);
};
DECLARE_DELEGATE_FourParams(FOnGroupsComplete,bool,const FHorizonMyGroupsView& ,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnGroupComplete,bool,const FHorizonGroupView& ,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnGroupMembersComplete,bool,const FHorizonGroupMembersView& ,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnGroupPendingComplete,bool,const FHorizonGroupPendingSnapshot& ,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnGroupMutationComplete,bool,const FHorizonGroupMutationView& ,const FString&,const FString&);
