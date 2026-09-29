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

    /**
     * Machine readable error code from the server body (`code` field, for example
     * "COSMETIC_LOCKED"). Empty when the server sent none (validation errors, 429).
     */
    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    FString ServerErrorCode;

    /** JSON response data (C++ only) */
    TSharedPtr<FJsonObject> JsonData;

    /** Binary response data */
    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    TArray<uint8> BinaryData;

    /** Map HTTP status code to SDK error code */
    static EHorizonErrorCode StatusToErrorCode(int32 HttpStatus);

    /** Stable string for an SDK error code, for example "RATE_LIMITED" or "CONNECTION_FAILED". */
    static FString ErrorCodeToString(EHorizonErrorCode Code);

    /**
     * The error code to show for a failed request: ServerErrorCode when the server sent
     * one, otherwise the mapping of the HTTP status (ErrorCodeToString(ErrorCode)).
     * Empty for a successful response.
     */
    FString GetErrorCodeString() const;
};
