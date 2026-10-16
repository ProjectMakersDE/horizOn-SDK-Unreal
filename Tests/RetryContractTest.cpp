#include "Transport/HorizonRetryContract.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace HorizonTransportContract;
    for (const auto Code:{"RUN_RATE_LIMITED","RUN_CAPACITY_REACHED","INBOX_POLICY","FUTURE_FEATURE_CODE"," "}) assert(IsFeatureRateLimitCode(Code));
    assert(!IsFeatureRateLimitCode(""));
    for (const auto Id:{""," \t\r\n","\xc2\xa0","\xe2\x80\x87","\xe2\x80\xaf","\xe3\x80\x80"}) assert(!HasNonBlankIdempotencyId(Id));
    assert(HasNonBlankIdempotencyId("original-id"));
    assert(HasNonBlankIdempotencyId("\xe6\x97\xa5\xe6\x9c\xac"));
    assert(!AllowJsonPostRetry("api/v1/app/user-management/signup",true));
    assert(!AllowJsonPostRetry("/api/v1/app/gift-codes/redeem/?request=one",false));
    assert(AllowJsonPostRetry("/api/v1/app/gift-codes/redeem",true));
    assert(AllowJsonPostRetry("/api/v1/app/player-values/purchase",false));
    assert(GlobalRateRetryDelay(10,0)==10);
    assert(GlobalRateRetryDelay(10,1)==12);
    assert(GlobalRateRetryDelay(0,0)==1);
    assert(GlobalRateRetryDelay(-1,0)==1);
    assert(GlobalRateRetryDelay(std::numeric_limits<double>::quiet_NaN(),0)==1);
    assert(GlobalRateRetryDelay(std::numeric_limits<double>::infinity(),0)==1);
    for (double Minimum:{1.1,10.0,16777217.0,2147483647.0})
        for (double Random:{0.0,.1,.5,1.0})
        { const double Delay=GlobalRateRetryDelay(Minimum,Random); assert(std::isfinite(Delay) && Delay>=Minimum && Delay<=Minimum*1.2+Minimum*1e-7); }
    assert(std::isfinite(GlobalRateRetryDelay(std::numeric_limits<double>::max(),1)));
    std::cout<<"Unreal global/feature rate limits, finite positive jitter and legacy single-attempt policy passed\n";
}
