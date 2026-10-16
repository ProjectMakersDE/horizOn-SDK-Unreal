#pragma once
#include "CoreMinimal.h"
#include "HorizonFriends.generated.h"
class FJsonObject;
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriendPublicProfile
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString AvatarId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString FrameId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") TArray<FString> Badges;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriendDisplay
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString Name;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FHorizonFriendPublicProfile Profile;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriend
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString Since;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FHorizonFriendDisplay Display;
    /** Retained latest login, never an online/offline signal. False means unknown. */
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") bool bHasLastActiveAt=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString LastActiveAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriendRequest
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString RequestId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FHorizonFriendDisplay Display;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonBlockedPlayer
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString BlockedAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriendsLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") int32 MaxFriends=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") int32 MaxIncomingRequests=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") int32 MaxOutgoingRequests=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") int32 MaxBlocked=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriendsSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString FriendCode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") TArray<FHorizonFriend> Friends;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") TArray<FHorizonFriendRequest> Incoming;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") TArray<FHorizonFriendRequest> Outgoing;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") TArray<FHorizonBlockedPlayer> Blocked;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FHorizonFriendsLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonFriendsSnapshot& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonFriendLookup
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FString FriendCode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Friends") FHorizonFriendDisplay Display;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonFriendLookup& Out);
    static bool IsPlayerQuery(const FString& Value);
    static FString FriendCodeFor(const FString& UserId);
};
DECLARE_DELEGATE_FourParams(FOnFriendsComplete,bool,const FHorizonFriendsSnapshot&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnFriendLookupComplete,bool,const FHorizonFriendLookup&,const FString&,const FString&);
