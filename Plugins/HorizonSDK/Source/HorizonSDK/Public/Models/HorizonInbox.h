#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonPlayerValues.h"
#include "Models/HorizonItems.h"
#include "HorizonInbox.generated.h"
class FJsonObject;

UENUM(BlueprintType)
enum class EHorizonInboxSourceKind : uint8 { Unknown, FriendRequest, FriendAccepted, GroupInvitation, GroupRequest, GroupInvitationAccepted, GroupRequestAccepted, MatchTurn, MarketReturn };
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxSource
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") EHorizonInboxSourceKind Kind = EHorizonInboxSourceKind::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString SourceId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString UserId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString GroupId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bHasRevision = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int64 Revision = 0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxItemReturn
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString ListingId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString InstanceId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString ItemKey;
};

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxAttachment
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") TArray<FHorizonPlayerValueAmount> Values;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FHorizonInboxItemReturn ItemReturn;
    bool HasReward() const { return !Values.IsEmpty() || ItemReturn.bPresent; }
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString Id;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString Kind;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString Type;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString Reference;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString TextKey;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") TMap<FString,FString> Parameters;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString ExpiresAt;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bOverdue = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bRead = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bClaimed = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FHorizonInboxAttachment Attachment;
    static bool IsEntryId(const FString& Value);
    EHorizonInboxSourceKind SourceKind() const;
    bool TryGetSource(FHorizonInboxSource& Out) const;
    bool TryGetFreeText(FString& Out) const;
    bool CanClaim() const { return !bClaimed && Attachment.bPresent && Attachment.HasReward(); }
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxCount
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 Total = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 Unread = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 Claimable = 0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 Targeted = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 Broadcasts = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 FreeTexts = 2;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 FreeTextBytes = 256;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 FreeTextRetentionDays = 7;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString UserId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString Cursor;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int64 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int64 BroadcastRevision = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString ServerTime;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") int32 PollAfterSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bUnchanged = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") TArray<FHorizonInboxEntry> Entries;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") TMap<FString,FHorizonInboxCount> Counts;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FHorizonInboxLimits Limits;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonInboxSnapshot& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxClaim
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FString EntryId;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bReplayed = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FHorizonInboxSnapshot Inbox;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") FHorizonPlayerValuesState PlayerValues;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") TArray<FHorizonItemReference> Items;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonInboxClaim& Out);
};
DECLARE_DELEGATE_FourParams(FOnInboxComplete,bool,const FHorizonInboxSnapshot&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnInboxClaimComplete,bool,const FHorizonInboxClaim&,const FString&,const FString&);
