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
