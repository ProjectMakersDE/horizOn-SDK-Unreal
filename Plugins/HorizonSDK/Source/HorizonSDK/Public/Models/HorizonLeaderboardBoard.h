#pragma once

#include "CoreMinimal.h"
#include "HorizonLeaderboardBoard.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonLeaderboardBoard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString ApiKeyId;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString Key;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString SortOrder;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	int64 ScoreCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString UpdatedAt;

	static FHorizonLeaderboardBoard FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
