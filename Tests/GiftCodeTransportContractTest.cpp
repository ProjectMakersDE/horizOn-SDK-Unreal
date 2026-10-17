#include "Transport/HorizonGiftCodeTransportContract.h"
#include <cassert>
#include <iostream>
int main()
{
 using namespace HorizonTransportContract;
 assert(!BuildGiftCodeRedeemPlan("user","session","code").bShouldSend);
 auto P=BuildGiftPreparedPlan("user","session","A\"B","00000000-0000-4000-8000-000000001007","9007199254740993","9223372036854775807");
 assert(P.bShouldSend && P.bUseSessionToken && P.BodyJson.find("\"expectedCount\":\"9007199254740993\"")!=std::string::npos && P.BodyJson.find("A\\\"B")!=std::string::npos);
 for(const auto Bad:{"","00","01","+1","-1","1.0","1e2","9223372036854775808"}) assert(!GiftCount(Bad));
 assert(GiftCount("0") && GiftCount("9223372036854775807"));
 assert(!BuildGiftPreparedPlan("user","","code","00000000-0000-4000-8000-000000001007","0","1").bShouldSend);
 std::cout<<"Unreal V2 Gift tuple exact strings, V1 closure, session gate and Int64 overflow passed\n";
}
