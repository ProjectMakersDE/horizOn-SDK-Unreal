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

	/**
	 * True when the board accepts validated runs only (Validated Actions, TASK-883):
	 * SubmitScore is rejected with VALIDATED_SUBMIT_REQUIRED, use
	 * ValidatedActions->SubmitValidated instead.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	bool bValidatedOnly = false;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
	FString UpdatedAt;

	static FHorizonLeaderboardBoard FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
