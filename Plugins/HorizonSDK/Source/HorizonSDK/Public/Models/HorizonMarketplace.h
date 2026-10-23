// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonItems.h"
#include "HorizonMarketplace.generated.h"
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 OpenListingsPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 RetainedListingsPerProject=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 ReceiptsPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 HistoryPerPlayer=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 ProjectWritesPerMinute=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 PlayerWriteIntervalMillis=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 MinDurationSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 MaxDurationSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 TerminalRetentionSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 OperationRetentionSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 QueryCandidates=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 PageSize=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceConfig
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool Enabled=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString CurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool HasFeeBps=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 FeeBps=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceRule
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ItemKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool Active=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 MinPrice=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 MaxPrice=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceListing
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString SellerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString BuyerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ReturnOwnerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool HasItem=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonItemInstance Item;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonItemReference ItemReference;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString CurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 Price=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int32 FeeBps=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 FeeAmount=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 SellerNet=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Status;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ExpiresAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString TerminalAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ReturnEntryId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool OwnerUnavailable=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceReceipt
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString OperationId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString IssuedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Kind;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ListingId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 ListingRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString RecordedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonItemReference ItemReference;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString CurrencyKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 Price=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 FeeAmount=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 SellerNet=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") TArray<FHorizonMarketplaceListing> OpenListings;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") TArray<FHorizonMarketplaceListing> History;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString WindowServerTime;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString AcceptAfter;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString BanUntil;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ModerationReason;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonMarketplaceLimits Limits;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceResponse
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool Present=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Kind;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ServerTime;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonMarketplaceLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonMarketplaceConfig Config;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool Realtime=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") int64 CatalogRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") TArray<FHorizonMarketplaceRule> Rules;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString NextCursor;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonMarketplaceState State;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") TArray<FHorizonMarketplaceListing> Listings;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool HasListing=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonMarketplaceListing Listing;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FHorizonMarketplaceReceipt Receipt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") bool Replayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Delivery;
    static bool TryFromJson(const FString& Body,const FString& Kind,FHorizonMarketplaceResponse& Out);
};
/** Exact original request JSON. Restore validates without renewing ID/time/revisions. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMarketplaceIntent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Kind;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString ListingId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Marketplace") FString Json;
    bool IsValid() const;
    bool Matches(const FHorizonMarketplaceReceipt& Receipt) const;
    FString User() const;
    FString Operation() const;
};
DECLARE_DELEGATE_FourParams(FOnMarketplaceComplete,bool,const FHorizonMarketplaceResponse&,const FString&,const FString&);
