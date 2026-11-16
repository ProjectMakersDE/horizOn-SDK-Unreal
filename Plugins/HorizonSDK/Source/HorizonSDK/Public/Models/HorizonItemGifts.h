// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonItems.h"
#include "HorizonItemGifts.generated.h"
/** Effective operator bounds. Lowering a limit never removes retained gifts. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGiftLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int32 OpenGiftsPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int32 RetainedGiftsPerProject=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int32 ProjectWritesPerMinute=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int32 HistoryPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int32 ReceivedPageSize=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 PlayerWriteIntervalMillis=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 ExpirySeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 TerminalRetentionSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 OperationRetentionSeconds=0;
};
/** CurrencyKey is empty for a free gift (wire null). The fee is never refunded. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGiftConfig
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") bool Enabled=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString CurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 Fee=0;
};
/** Status: offered, claimed, return_pending, return_delivered, returned. Empty strings stand for wire null. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGift
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString SenderId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString RecipientId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString ReturnOwnerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemReference ItemReference;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString CurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 Fee=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 ConfigRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString Status;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString ExpiresAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString TerminalAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString DeclinedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString OfferEntryId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString ReturnEntryId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") bool OwnerUnavailable=false;
    bool IsTerminal() const{return Status==TEXT("claimed")||Status==TEXT("returned");}
};
/** The receipt is the fee proof of one send. No second value history is written. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGiftReceipt
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString OperationId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString IssuedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString GiftId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 GiftRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString RecordedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString RecipientId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemReference ItemReference;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString CurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 Fee=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 ConfigRevision=0;
};
/** Sent: open gifts and pending returns. History: recent terminal gifts. Received: open offers (one bounded page). */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGiftState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") TArray<FHorizonItemGift> Sent;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") TArray<FHorizonItemGift> History;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") TArray<FHorizonItemGift> Received;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString WindowServerTime;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString AcceptAfter;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemGiftLimits Limits;
};
/** Kind selects the typed branch: config, state, written, operation, point or declined. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGiftResponse
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") bool Present=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString Kind;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString ServerTime;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemGiftConfig Config;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemGiftLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemGiftState State;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemGiftReceipt Receipt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") bool HasGift=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FHorizonItemGift Gift;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") bool Replayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString ReturnDelivery;
    static bool TryFromJson(const FString& Body,const FString& Kind,FHorizonItemGiftResponse& Out);
};
/** Exact original send JSON with all seven fields. Restore validates without renewing ID, time or revisions. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemGiftIntent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Item Gifts") FString Json;
    bool IsValid() const;
    bool Matches(const FHorizonItemGiftReceipt& Receipt) const;
    FString User() const;
    FString Operation() const;
};
DECLARE_DELEGATE_FourParams(FOnItemGiftComplete,bool,const FHorizonItemGiftResponse&,const FString&,const FString&);
