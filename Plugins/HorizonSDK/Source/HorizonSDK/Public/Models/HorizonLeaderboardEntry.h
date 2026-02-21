#pragma once

#include "CoreMinimal.h"
#include "HorizonLeaderboardEntry.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonLeaderboardEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    int32 Position = 0;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    FString Username;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    int64 Score = 0;

    static FHorizonLeaderboardEntry FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
