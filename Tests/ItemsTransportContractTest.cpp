#include "Transport/HorizonItemsTransportContract.h"
#include <cassert>
#include <iostream>
int main()
{
    std::vector<HorizonItemsContract::Property> Rows;std::string Canonical;
    assert(HorizonItemsContract::Properties(R"({"z":"ü/😀\u0000after","a":9007199254740991,"b":true,"neg":-9007199254740991})",Rows,Canonical));
    assert(Canonical==R"({"a":9007199254740991,"b":true,"neg":-9007199254740991,"z":"ü/😀\u0000after"})");
    for(const std::string Bad:{"[]","{\"a\":null}","{\"a\":1.0}","{\"a\":1e3}","{\"a\":9007199254740992}","{\"a\":-0}","{\"a\":0,\"a\":1}","{\"A\":0}","{\"a.b\":0}","{\"a\":{}}","{\"a\":[]}","{\"a\":\"\\ud800\"}"})assert(!HorizonItemsContract::Properties(Bad,Rows,Canonical));
    assert(HorizonItemsContract::Properties("{\"a\":\""+std::string(248,'x')+"\"}",Rows,Canonical)&&Canonical.size()==256);
    assert(!HorizonItemsContract::Properties("{\"a\":\""+std::string(249,'x')+"\"}",Rows,Canonical));
    assert(HorizonItemsContract::Properties(R"({"\u0061":"\u00fc\/"})",Rows,Canonical)&&Canonical=="{\"a\":\"ü/\"}");
    std::string Raw="{";for(int I=0;I<17;++I){if(I)Raw+=',';Raw+="\"k"+std::to_string(I)+"\":0";}Raw+='}';assert(!HorizonItemsContract::Properties(Raw,Rows,Canonical));
    long long N;assert(HorizonItemsContract::Integer("-9007199254740991",N)&&N==-9007199254740991LL);assert(!HorizonItemsContract::Integer("-9007199254740992",N));
    std::vector<std::string> Objects;std::string Masked;assert(HorizonGroupsContract::JsonObject(R"({"properties":{"x":1},"baseProperties":{"s":"a\u0000b"},"data":{"ordinary":true}})").MaskObjects("opaque-",Objects,Masked,{"properties","baseProperties"},256)&&Objects.size()==2&&Masked.find("\"data\":{\"ordinary\":true}")!=std::string::npos);
    std::cout<<"Items canonical UTF-8, ASCII keys, strict safe integers, duplicates and 256-byte bounds passed\n";
}
