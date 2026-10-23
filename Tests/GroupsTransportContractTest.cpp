#include "Transport/HorizonGroupsTransportContract.h"
#include <cassert>
#include <iostream>
#include <vector>
int main()
{
    std::string Out; const std::string Exact=R"({"":null,".$":{"large":9007199254740993,"precise":0.12345678901234567890123456789,"x":[true,false,"\ud83d\ude00"]}})";
    assert(HorizonGroupsContract::CompactObject(" \n"+Exact+" \t",Out) && Out==R"({"":null,".$":{"large":9007199254740993,"precise":0.12345678901234567890123456789,"x":[true,false,"😀"]}})");
    for(const std::string Bad:std::vector<std::string>{"[]","{\"x\":NaN}","{\"x\":1e309}","{\"x\":01}","{\"x\":1.}","{\"x\":+1}","{\"x\":true,}","{\"x\":\"\\ud800\"}","{\"x\":\"\\udc00\"}","{\"x\":\"\n\"}","{}garbage",std::string("{\"x\":\"")+char(0xc0)+char(0xaf)+"\"}"}) assert(!HorizonGroupsContract::CompactObject(Bad,Out));
    std::string Max="0";for(int I=0;I<15;++I)Max="["+Max+"]";assert(HorizonGroupsContract::CompactObject("{\"x\":"+Max+"}",Out));
    Max="["+Max+"]";assert(!HorizonGroupsContract::CompactObject("{\"x\":"+Max+"}",Out));
    assert(HorizonGroupsContract::CompactObject("{ \"emoji\" : \"😀\", \"underflow\":1e-9999 }",Out));
    const std::string Data=R"({"a\u0000b":1,"a\u0000c":2,"A":3,"a":4})";
    std::vector<std::string> Raw;std::string Masked;
    const std::string Body="{\"group\":{\"data\":"+Data+"}}";
    assert(HorizonGroupsContract::JsonObject(Body).MaskObjects("opaque-",Raw,Masked));
    assert(Raw.size()==1 && Raw[0]==Data && Masked=="{\"group\":{\"data\":\"opaque-0\"}}");
    assert(HorizonGroupsContract::CompactObject("{\"\\u0061\":\"\\/\"}",Out) && Out=="{\"a\":\"/\"}");
    const std::string Large="{\"x\":\""+std::string(9*1024*1024,'x')+"\",\"n\":9007199254740993}";
    assert(HorizonGroupsContract::CompactObject(Large,Out,1024)&&Out==Large);
    std::string Deep="0";for(int I=0;I<31;++I)Deep="["+Deep+"]";
    assert(HorizonGroupsContract::CompactObject("{\"x\":"+Deep+"}",Out,1024));
    std::string Rows="{\"inventory\":[";for(int I=0;I<2051;++I){if(I)Rows+=",";Rows+="{\"properties\":{\"n\":9007199254740993}}";}Rows+="]}";
    std::vector<std::string> Inventory;assert(HorizonGroupsContract::JsonObject(Rows).MaskObjects("opaque-",Inventory,Masked,{"properties"})&&Inventory.size()==2051);
    std::cout<<"Groups strict UTF-8 JSON grammar, depth and lossless number tokens passed\n";
}
