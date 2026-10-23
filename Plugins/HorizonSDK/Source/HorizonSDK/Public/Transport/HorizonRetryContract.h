// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Transport/HorizonSignupRetryContract.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace HorizonTransportContract
{
/** A feature owns every nonempty code, including codes added by future features. */
inline bool IsFeatureRateLimitCode(const std::string& Code) { return !Code.empty(); }

/** Match Kotlin/JVM blank IDs independently of the process locale (notably NBSP). */
inline bool HasNonBlankIdempotencyId(const std::string& Id)
{
    for (size_t Index=0;Index<Id.size();)
    {
        const unsigned char First=static_cast<unsigned char>(Id[Index]);
        size_t Width=1; uint32_t Code=First;
        if (First>=0xc2 && First<=0xdf) { Width=2; Code=First&0x1f; }
        else if (First>=0xe0 && First<=0xef) { Width=3; Code=First&0x0f; }
        else if (First>=0xf0 && First<=0xf4) { Width=4; Code=First&0x07; }
        else if (First>=0x80) return false;
        if (Index+Width>Id.size()) return false;
        for (size_t Offset=1;Offset<Width;Offset++)
        {
            const unsigned char Next=static_cast<unsigned char>(Id[Index+Offset]);
            if ((Next&0xc0)!=0x80) return false;
            Code=(Code<<6)|(Next&0x3f);
        }
        const bool Blank=(Code>=9 && Code<=13) || (Code>=0x1c && Code<=0x20) || Code==0xa0 || Code==0x1680 || (Code>=0x2000 && Code<=0x200a) || Code==0x2028 || Code==0x2029 || Code==0x202f || Code==0x205f || Code==0x3000;
        if (!Blank) return true;
        Index+=Width;
    }
    return false;
}

/** Signup and legacy redemption create identities/intents without a reusable caller key. */
inline bool AllowJsonPostRetry(std::string Endpoint,bool HasExplicitIdempotencyId)
{
    if (!AllowSignupRetry(Endpoint)) return false;
    Endpoint=Endpoint.substr(0,Endpoint.find_first_of("?#"));
    while (!Endpoint.empty() && Endpoint.front()=='/') Endpoint.erase(Endpoint.begin());
    while (!Endpoint.empty() && Endpoint.back()=='/') Endpoint.pop_back();
    return Endpoint!="api/v1/app/gift-codes/redeem" || HasExplicitIdempotencyId;
}

/** Server seconds plus 0..20% jitter; never round below its minimum or overflow UE timers. */
inline float GlobalRateRetryDelay(double RetryAfter,double UnitRandom)
{
    const double Maximum=std::numeric_limits<float>::max();
    const double Minimum=std::isfinite(RetryAfter) && RetryAfter>0 ? std::clamp(RetryAfter,1.0,Maximum):1.0;
    const double Random=std::isfinite(UnitRandom)?std::clamp(UnitRandom,0.0,1.0):0.0;
    const double Delay=std::min(Maximum,Minimum*(1.0+.2*Random));
    float Rounded=static_cast<float>(Delay);
    if (Rounded<Minimum) Rounded=std::nextafter(Rounded,std::numeric_limits<float>::infinity());
    return Rounded;
}
}
