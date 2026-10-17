#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonPlayerValues.h"
#include "HorizonGiftCodes.generated.h"

/** Persist every field before the first send. No bearer token or raw API key is stored. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGiftCodeIntent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") int32 ProtocolVersion=2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") FString ProjectScope;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") FString UserId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") FString Code;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") FString CampaignId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") FString ExpectedCount;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,SaveGame,Category="horizOn|GiftCodes") FString ConfigRevision;
    bool IsValidFor(const FString& Scope,const FString& User) const;
    bool SameIdentity(const FHorizonGiftCodeIntent& Other) const;
    static FString Fingerprint(const FString& ApiKey);
    static bool TryPrepare(const TSharedPtr<FJsonObject>& Json,const FString& Scope,const FString& User,const FString& Code,FHorizonGiftCodeIntent& Out);
};
/** Additive redeem receipt. GiftData stays the original opaque string. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonGiftCodeRedeemResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") bool bSuccess=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString Message;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString GiftData;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") TArray<FString> GrantedUnlocks;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") TArray<FHorizonPlayerValueAmount> GrantedValues;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FHorizonPlayerValuesState PlayerValues;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") bool bReplayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") bool bAlreadyApplied=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString CampaignId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString Count;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|GiftCodes") FString ConfigRevision;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonGiftCodeRedeemResult& Out);
};
