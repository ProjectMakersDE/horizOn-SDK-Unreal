#include "Transport/HorizonSignupRetryContract.h"
#include <cassert>
#include <iostream>
int main()
{
    using HorizonTransportContract::AllowSignupRetry;
    assert(!AllowSignupRetry("api/v1/app/user-management/signup"));
    assert(!AllowSignupRetry("/api/v1/app/user-management/signup/?request=one"));
    assert(AllowSignupRetry("/api/v1/app/player-values/purchase"));
    assert(AllowSignupRetry("/api/v1/app/user-management/signin"));
    std::cout<<"Unreal non-idempotent signup single-attempt contract passed\n";
}
