// Copyright (c) 2025-2026 horizOn. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include "HorizonStatistics.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonStatisticUpdate
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "horizOn|Statistics")
	FString Key;
	UPROPERTY(BlueprintReadWrite, Category = "horizOn|Statistics")
	int64 Value = 0;
};

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonStatisticDefinition
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString Key;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString Mode;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString Reset;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 MaxPerUpdate = 0;
};

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonStatisticValue
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString Key;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 Value = 0;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	bool bHasValue = false;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString Period;
};

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonStatisticsOperationWindow
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 ServerTimeUnixMillis = 0;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 MaxAgeSeconds = 0;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 MaxFutureSkewSeconds = 0;
};

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonStatisticsCatalog
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 DefinitionVersion = -1;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	TArray<FHorizonStatisticDefinition> Definitions;
	bool bDefinitionsIncluded = false;
	bool bValid = false;
	static FHorizonStatisticsCatalog FromJson(const TSharedPtr<FJsonObject>& Json);
};

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonStatisticsSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString UserId;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 DefinitionVersion = -1;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	TArray<FHorizonStatisticDefinition> Definitions;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	int64 Revision = -1;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	TArray<FHorizonStatisticValue> Values;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FHorizonStatisticsOperationWindow OperationWindow;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	FString OperationId;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	bool bReplayed = false;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Statistics")
	TArray<FString> NewlyUnlocked;
	bool bDefinitionsIncluded = false;
	bool bValid = false;
	static FHorizonStatisticsSnapshot FromJson(const TSharedPtr<FJsonObject>& Json);
};

DECLARE_DELEGATE_FourParams(FOnStatisticsLoaded, bool, const FHorizonStatisticsSnapshot&, const FString&, const FString&);
DECLARE_DELEGATE_FourParams(FOnStatisticsDefinitionsLoaded, bool, const FHorizonStatisticsCatalog&, const FString&, const FString&);
