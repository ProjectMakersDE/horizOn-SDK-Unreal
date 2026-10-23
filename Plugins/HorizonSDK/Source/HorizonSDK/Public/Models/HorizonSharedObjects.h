// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "HorizonSharedObjects.generated.h"
class FJsonObject;
UENUM(BlueprintType)
enum class EHorizonSharedScalarType : uint8 { Integer,String };
UENUM(BlueprintType)
enum class EHorizonSharedQueryMode : uint8 { List,Owner,Field,Rectangle,Random };
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedScalar
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") EHorizonSharedScalarType Type=EHorizonSharedScalarType::Integer;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 IntegerValue=0;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FString StringValue;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedSearchValue
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FString Key;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FHorizonSharedScalar Value;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedPosition
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 X=0;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 Y=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedObjectInput
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") TArray<uint8> Data;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") TArray<FHorizonSharedSearchValue> Search;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") bool bHasPosition=false;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FHorizonSharedPosition Position;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedQuery
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") EHorizonSharedQueryMode Mode=EHorizonSharedQueryMode::List;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FString OwnerId;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FString GroupId;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int32 Limit=20;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FString Cursor;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FString Field;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") bool bUseEqual=false;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FHorizonSharedScalar Equal;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FHorizonSharedScalar Min;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") FHorizonSharedScalar Max;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 MinX=0;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 MaxX=0;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 MinY=0;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Shared Objects") int64 MaxY=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedSearchField
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") EHorizonSharedScalarType Type=EHorizonSharedScalarType::Integer;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 Collections=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int64 StorageBytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 ObjectBytes=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedDefinition
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString Key;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString Name;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bActive=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ReadPolicy;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString WritePolicy;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 MaxObjectBytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 MaxObjectsPerOwner=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bHasTtlSeconds=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int64 TtlSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") TArray<FHorizonSharedSearchField> SearchFields;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPositionEnabled=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bUsed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int64 Revision=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedObject
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString Collection;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString OwnerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bHasGroupId=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString GroupId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") TArray<uint8> Data;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") TArray<FHorizonSharedSearchValue> Search;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bHasPosition=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedPosition Position;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString UpdatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bHasExpiresAt=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ExpiresAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedCollectionPage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") TArray<FHorizonSharedDefinition> Definitions;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bHasNextAfterKey=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString NextAfterKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonSharedCollectionPage& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedUsage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString Collection;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 Count=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonSharedUsage& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedObjectRead
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedObject Object;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonSharedObjectRead& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedQueryResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") TArray<FHorizonSharedObject> Objects;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bHasNextCursor=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString NextCursor;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonSharedQueryResult& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedWriteResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedObject Object;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedUsage Usage;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonSharedWriteResult& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonSharedDeleteResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ObjectId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") bool bDeleted=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FHorizonSharedUsage Usage;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Shared Objects") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonSharedDeleteResult& Out);
};
DECLARE_DELEGATE_FourParams(FOnSharedCollectionsComplete,bool,const FHorizonSharedCollectionPage&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnSharedUsageComplete,bool,const FHorizonSharedUsage&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnSharedObjectComplete,bool,const FHorizonSharedObjectRead&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnSharedQueryComplete,bool,const FHorizonSharedQueryResult&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnSharedWriteComplete,bool,const FHorizonSharedWriteResult&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnSharedDeleteComplete,bool,const FHorizonSharedDeleteResult&,const FString&,const FString&);
