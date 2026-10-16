#pragma once
#include <string>
namespace HorizonTransportContract
{
/** Signup creates a server identity without a reusable idempotency key. */
inline bool AllowSignupRetry(std::string Endpoint)
{
    Endpoint=Endpoint.substr(0,Endpoint.find_first_of("?#"));
    while (!Endpoint.empty() && Endpoint.front()=='/') Endpoint.erase(Endpoint.begin());
    while (!Endpoint.empty() && Endpoint.back()=='/') Endpoint.pop_back();
    return Endpoint!="api/v1/app/user-management/signup";
}
}
