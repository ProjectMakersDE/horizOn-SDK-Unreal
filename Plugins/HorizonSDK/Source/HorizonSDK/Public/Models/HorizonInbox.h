#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonPlayerValues.h"
#include "HorizonInbox.generated.h"
class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInboxAttachment
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") bool bPresent = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Inbox") TArray<FHorizonPlayerValueAmount> Values;
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
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonInboxClaim& Out);
};
DECLARE_DELEGATE_FourParams(FOnInboxComplete,bool,const FHorizonInboxSnapshot&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnInboxClaimComplete,bool,const FHorizonInboxClaim&,const FString&,const FString&);
