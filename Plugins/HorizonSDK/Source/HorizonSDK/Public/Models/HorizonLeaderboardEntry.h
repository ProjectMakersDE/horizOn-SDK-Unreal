#pragma once

#include "CoreMinimal.h"
#include "Models/HorizonPlayerProfile.h"
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

    /**
     * Avatar, frame and badges of the player (TASK-881). Always filled by the server;
     * empty values when the player has no profile. Treat unknown IDs as "not set".
     */
    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    FHorizonPlayerProfile Profile;

    static FHorizonLeaderboardEntry FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
