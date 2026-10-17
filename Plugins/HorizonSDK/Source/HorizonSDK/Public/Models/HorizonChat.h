#pragma once
#include "CoreMinimal.h"
#include "HorizonChat.generated.h"
class FJsonObject;
/** Last completed HTTP metadata only; no message payload or session token. Read it inside the completion callback. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatResponseMetadata
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 StatusCode=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ErrorCode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ErrorMessage;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") TArray<FString> Headers;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 RetainedMessagesPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 RetainedMessagesPerChannel=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 RetentionSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 PrivateRetainedMessagesPerChannel=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 PrivateRetentionSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 OperationWindowSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 OperationFutureToleranceSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 MessageCharacters=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 MessageUtf8Bytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 ChannelsPerAccount=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 SendsPerAccountMinute=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 PlayerSendIntervalMillis=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 ChannelSendIntervalMillis=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 ReportsPerProject=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 ReceiptCount=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 ReportReceiptCount=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int32 WordFilterTerms=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatCapabilities
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bChat=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bRealtime=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bIncrementalSync=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bLocalHistory=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatModeration
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bHasMuteUntil=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString MuteUntil;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bHasBanUntil=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString BanUntil;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatChannel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString Epoch;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString Kind;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") TArray<FString> PeerIds;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bHasGroupId=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString GroupId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ExpiresAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatMessage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ChannelId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString AuthorId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString Text;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ExpiresAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatReceipt
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString OperationId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString IssuedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ChannelId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString MessageId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ExpiresAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatHistoryPage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bSuccess=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") TArray<FHorizonChatMessage> Messages;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString NextCursor;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ErrorCode;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatLimitsView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatCapabilities Capabilities;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatModeration Moderation;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonChatLimitsView& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatOpened
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatChannel Channel;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonChatOpened& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatSynced
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatChannel Channel;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") TArray<FString> VisibleMessageIds;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") TArray<FHorizonChatMessage> Messages;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatModeration Moderation;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonChatSynced& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatSent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatReceipt Receipt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bReplayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonChatSent& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatOperation
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatReceipt Receipt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonChatOperation& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatReported
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ReportId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString Status;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bReplayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonChatReported& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatFeed
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatChannel Channel;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") TArray<FHorizonChatMessage> Messages;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FHorizonChatModeration Moderation;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat") FString ServerTime;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonChatOpenInput
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Chat") FString Kind=TEXT("project");
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Chat") FString PeerId;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Chat") FString GroupId;
};
DECLARE_DELEGATE_FourParams(FOnChatLimitsViewComplete,bool,const FHorizonChatLimitsView&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnChatOpenedComplete,bool,const FHorizonChatOpened&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnChatSyncedComplete,bool,const FHorizonChatSynced&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnChatSentComplete,bool,const FHorizonChatSent&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnChatOperationComplete,bool,const FHorizonChatOperation&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnChatReportedComplete,bool,const FHorizonChatReported&,const FString&,const FString&);
