#include "Models/HorizonNetworkResponse.h"

EHorizonErrorCode FHorizonNetworkResponse::StatusToErrorCode(int32 HttpStatus)
{
    if (HttpStatus >= 200 && HttpStatus < 300)
    {
        return EHorizonErrorCode::None;
    }

    switch (HttpStatus)
    {
    case 400: return EHorizonErrorCode::InvalidRequest;
    case 401: return EHorizonErrorCode::Unauthorized;
    case 403: return EHorizonErrorCode::Forbidden;
    case 404: return EHorizonErrorCode::NotFound;
    case 409: return EHorizonErrorCode::Conflict;
    case 429: return EHorizonErrorCode::RateLimited;
    default:
        if (HttpStatus >= 500)
        {
            return EHorizonErrorCode::ServerError;
        }
        return EHorizonErrorCode::Unknown;
    }
}

FString FHorizonNetworkResponse::ErrorCodeToString(EHorizonErrorCode Code)
{
    switch (Code)
    {
    case EHorizonErrorCode::None:             return FString();
    case EHorizonErrorCode::InvalidRequest:   return TEXT("INVALID_REQUEST");
    case EHorizonErrorCode::Unauthorized:     return TEXT("UNAUTHORIZED");
    case EHorizonErrorCode::Forbidden:        return TEXT("FORBIDDEN");
    case EHorizonErrorCode::NotFound:         return TEXT("NOT_FOUND");
    case EHorizonErrorCode::Conflict:         return TEXT("CONFLICT");
    case EHorizonErrorCode::RateLimited:      return TEXT("RATE_LIMITED");
    case EHorizonErrorCode::ServerError:      return TEXT("SERVER_ERROR");
    case EHorizonErrorCode::ConnectionFailed: return TEXT("CONNECTION_FAILED");
    default:                                  return TEXT("UNKNOWN");
    }
}

FString FHorizonNetworkResponse::GetErrorCodeString() const
{
    if (bSuccess)
    {
        return FString();
    }
    if (!ServerErrorCode.IsEmpty())
    {
        return ServerErrorCode;
    }
    const FString Mapped = ErrorCodeToString(ErrorCode);
    return Mapped.IsEmpty() ? FString(TEXT("UNKNOWN")) : Mapped;
}
