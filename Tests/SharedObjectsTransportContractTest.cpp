#include "Transport/HorizonSharedObjectsTransportContract.h"
#include <cassert>
#include <iostream>
int main()
{
 using namespace HorizonSharedContract;std::vector<SearchValue> V;std::string S;
 assert(Search(R"({"power":9007199254740991,"zone":"雪/😀\u0000tail","low":-9007199254740991})",V,S));
 assert(V.size()==3&&V[0].Integer==9007199254740991LL&&V[1].IsString&&V[1].String.size()==13&&V[1].String[8]==0&&V[1].String.substr(9)=="tail");
 for(const std::string& Bad:{R"({"x":null})",R"({"x":true})",R"({"x":1.0})",R"({"x":1e3})",R"({"x":9007199254740991.1})",R"({"x":9007199254740992})",R"({"x":-0})",R"({"x":1,"x":2})",R"({"x":1,"\u0078":2})",R"({"a":0,"b":0,"c":0,"d":0})",R"({"A":1})",R"({"x":"\ud800"})",R"({"x":{}})"})assert(!Search(Bad,V,S));
 assert(Search("{\"x\":\""+std::string(96,'a')+"\"}",V,S));assert(!Search("{\"x\":\""+std::string(97,'a')+"\"}",V,S));
 std::vector<uint8_t> Bytes;assert(DecodeBase64("",Bytes)&&Bytes.empty());assert(DecodeBase64("AP8A",Bytes)&&Bytes==std::vector<uint8_t>({0,255,0}));assert(DecodeBase64("/w==",Bytes)&&Bytes==std::vector<uint8_t>({255}));
 for(const std::string& Bad:{"A","AA=","AA==\n","AB==","AAF=","AA=A","====","AA__","éé","AAAA===="})assert(!DecodeBase64(Bad,Bytes));
 const std::string Max=std::string(1398100,'A')+"AA==";assert(DecodeBase64(Max,Bytes)&&Bytes.size()==1048576);assert(!DecodeBase64(Max+"AAAA",Bytes));assert(!DecodeBase64("AA==",Bytes,0));
 assert(Cursor("aZ09_-")&&!Cursor("")&&!Cursor("a=")&&!Cursor(std::string(2049,'a')));
 std::cout<<"Shared Objects canonical Base64, bounded opaque bytes, safe search integers, Unicode/NUL and strict cursors passed\n";
}
