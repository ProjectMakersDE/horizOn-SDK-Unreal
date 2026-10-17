#include "Transport/HorizonMarketplaceTransportContract.h"
#include <iostream>
#include <stdexcept>
void Require(bool B,const char* S){if(!B)throw std::runtime_error(S);}
int main()
{
 using namespace HorizonMarketplaceContract;
 auto Parse=[](const std::string& S){std::vector<std::string> P;std::string M;return Mask(S,"market-mask-",P,M);};
 Require(Parse("{\"price\":9007199254740991,\"currency\":\"1coin.gold\"}"),"safe numbers");
 for(const auto& S:{"1.0","1e0","-0","9007199254740992","-9007199254740992"})Require(!Parse(std::string("{\"price\":")+S+"}"),"unsafe exact numeric token");
 Require(!Parse("{\"price\":1,\"pr\\u0069ce\":2}"),"escaped duplicate before engine DOM aliasing");
 Require(!Parse("{\"key\":\"\\ud800\"}"),"invalid Unicode");
 Require(Currency("1coin.gold")&&!Currency("Gold")&&!Currency(".gold"),"currency grammar");
 Require(Fee(9007199254740991LL,10000)==9007199254740991LL,"safe fee no overflow");
 std::string Rows="{\"listings\":[";for(int I=0;I<2051;++I){if(I)Rows+=',';Rows+="{\"properties\":{\"x\":1}}";}Rows+="]}";
 std::vector<std::string> Properties;std::string Masked;Require(Mask(Rows,"market-mask-",Properties,Masked)&&Properties.size()==2051,"no inherited 256 properties or 2048 collection quota");
 Require(!Parse("{\"x\":\""+std::string(MaxBytes,'x')+"\"}"),"8MiB frame independent of product quota");
 std::cout<<"Marketplace strict tokens/duplicates/2051 property objects/transport frame/safe fees passed\n";
}
