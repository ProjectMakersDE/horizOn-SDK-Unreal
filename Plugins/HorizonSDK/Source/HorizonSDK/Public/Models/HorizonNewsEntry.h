#pragma once

#include "CoreMinimal.h"
#include "HorizonNewsEntry.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonNewsEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString Title;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString ReleaseDate;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString LanguageCode;

    static FHorizonNewsEntry FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
