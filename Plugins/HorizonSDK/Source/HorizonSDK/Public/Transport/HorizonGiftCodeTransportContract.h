// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Transport/HorizonLeaderboardTransportContract.h"
#include <string>
namespace HorizonTransportContract
{
inline bool GiftUuid(const std::string& V)
{
    if (V.size()!=36) return false;
    for(size_t I=0;I<V.size();++I) { const char C=V[I]; if(I==8 || I==13 || I==18 || I==23) { if(C!='-') return false; } else if(!((C>='0' && C<='9') || (C>='a' && C<='f') || (C>='A' && C<='F'))) return false; }
    return true;
}
inline bool GiftCount(const std::string& Value)
{
    if (Value.empty() || Value.size()>19 || (Value.size()>1 && Value[0]=='0')) return false;
    for (char C:Value) if (C<'0' || C>'9') return false;
    return Value.size()<19 || Value<="9223372036854775807";
}
struct FGiftCodeRedeemPlan { bool bShouldSend=false; bool bUseSessionToken=true; std::string Endpoint,BodyJson; };
// Plain validation needs no session, but the server requires the player it checks the per-user limit for.
inline FGiftCodeRedeemPlan BuildGiftValidatePlan(const std::string& User,const std::string& Code)
{
    FGiftCodeRedeemPlan P;
    if (User.empty() || Code.empty() || Code.size()>50) return P;
    P.bShouldSend=true; P.bUseSessionToken=false; P.Endpoint="/api/v1/app/gift-codes/validate";
    P.BodyJson="{\"code\":\""+EscapeJson(Code)+"\",\"userId\":\""+EscapeJson(User)+"\"}";
    return P;
}
inline FGiftCodeRedeemPlan BuildGiftCodeRedeemPlan(const std::string&,const std::string&,const std::string&) { return {}; }
inline FGiftCodeRedeemPlan BuildGiftPreparedPlan(const std::string& User,const std::string& Token,const std::string& Code,const std::string& Campaign,const std::string& Count,const std::string& Revision)
{
    FGiftCodeRedeemPlan P;
    if (User.empty() || Token.empty() || Code.empty() || !GiftUuid(Campaign) || !GiftCount(Count) || !GiftCount(Revision)) return P;
    P.bShouldSend=true; P.Endpoint="/api/v1/app/gift-codes/redeem";
    P.BodyJson="{\"code\":\""+EscapeJson(Code)+"\",\"userId\":\""+EscapeJson(User)+"\",\"protocolVersion\":2,\"campaignId\":\""+EscapeJson(Campaign)+"\",\"expectedCount\":\""+Count+"\",\"configRevision\":\""+Revision+"\"}";
    return P;
}
}
