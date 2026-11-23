// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "HorizonItems.generated.h"
class FJsonObject;
UENUM(BlueprintType)
enum class EHorizonItemPropertyType : uint8 { Integer,Boolean,String };
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemProperty
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") EHorizonItemPropertyType Type=EHorizonItemPropertyType::Integer;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 IntegerValue=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool BooleanValue=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString StringValue;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemPropertyDelta
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Items") FString Key;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Items") int64 Amount=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemPropertyRule
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 Min=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 Max=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 MaxChangePerCall=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemReward
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ItemKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Quantity=0;
    static bool TryArrayFromJson(const TSharedPtr<FJsonObject>& Json,const TCHAR* Field,TArray<FHorizonItemReward>& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemReference
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString InstanceId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ItemKey;
    static bool TryArrayFromJson(const TSharedPtr<FJsonObject>& Json,const TCHAR* Field,TArray<FHorizonItemReference>& Out);
};
/** One stack per stack template: only a count, its values always come from the catalog template. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemStack
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ItemKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 Count=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bEquipped=false;
};
/** Quantity a purchase added to a stack and the resulting stack total. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemStackGrant
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ItemKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Quantity=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 Count=0;
    static bool TryArrayFromJson(const TSharedPtr<FJsonObject>& Json,const TCHAR* Field,TArray<FHorizonItemStackGrant>& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemInstance
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString InstanceId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ItemKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") TArray<FHorizonItemProperty> Properties;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bEquipped=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString CreatedAt;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonItemInstance& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemDefinition
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Name;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Type;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Rarity;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bActive=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") TArray<FHorizonItemProperty> BaseProperties;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") TArray<FHorizonItemPropertyRule> PropertyRules;
    /** "unique" (own instances) or "stack" (one count per player). Missing on old servers means "unique". */
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString Mode=TEXT("unique");
    /** Highest stack count, only for stack templates; 0 when absent. */
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 MaxStack=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bHasMaxStack=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Catalog=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Inventory=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Receipts=0;
    /** Stack slots per player, one per stack template. 0 when an old server omits it. */
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Stacks=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonInventorySnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") TArray<FHorizonItemInstance> Instances;
    /** Missing on old servers means empty. */
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") TArray<FHorizonItemStack> Stacks;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FHorizonItemLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonInventorySnapshot& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonItemCatalogPage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") TArray<FHorizonItemDefinition> Definitions;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString NextAfterKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") bool bHasNextAfterKey=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") int32 Total=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FHorizonItemLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Items") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonItemCatalogPage& Out);
};
DECLARE_DELEGATE_FourParams(FOnInventoryComplete,bool,const FHorizonInventorySnapshot&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnItemCatalogComplete,bool,const FHorizonItemCatalogPage&,const FString&,const FString&);
