#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Dom/JsonObject.h"
#include "HorizonNetworkResponse.generated.h"

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonNetworkResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    int32 StatusCode = 0;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    EHorizonErrorCode ErrorCode = EHorizonErrorCode::None;

    /** JSON response data (C++ only) */
    TSharedPtr<FJsonObject> JsonData;

    /** Binary response data */
    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    TArray<uint8> BinaryData;

    /** Map HTTP status code to SDK error code */
    static EHorizonErrorCode StatusToErrorCode(int32 HttpStatus);
};
