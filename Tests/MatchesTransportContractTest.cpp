#include "Transport/HorizonMatchesTransportContract.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace HorizonMatchesContract;
    assert(Safe(MaxSafe)&&Safe(-MaxSafe)&&!Safe(MaxSafe+1)&&!Safe(-MaxSafe-1));assert(Revision(0)&&!Revision(-1));
    for(const std::string S:{"lobby","active","reporting","confirmed","disputed","unconfirmed","cancelled"})assert(Status(S));for(const std::string S:{"online","won","draw","finished",""})assert(!Status(S));
    assert(Cursor("bTF8c2NvcGV8MHwwMDAwMDAwMC0wMDAwLTQwMDAtODAwMC0wMDAwMDAwMDAwMDE="));assert(Cursor(std::string(256,'A')));for(const std::string S:{"","AB==","YQ","YQ==\n","YQ== ","/w=="})assert(!Cursor(S));assert(!Cursor(std::string(260,'A')));
    for(uint32_t C:{9u,13u,28u,32u,0x1680u,0x2000u,0x2006u,0x2008u,0x200au,0x2028u,0x2029u,0x205fu,0x3000u})assert(Whitespace(C));for(uint32_t C:{0u,0xa0u,0x2007u,0x202fu,0x96eau})assert(!Whitespace(C));
    std::vector<uint8_t> B;assert(HorizonSharedContract::DecodeBase64("AP8A",B,MaxStateBytes)&&B==std::vector<uint8_t>({0,255,0}));std::string Large((MaxStateBytes+2)/3*4,'A');assert(!HorizonSharedContract::DecodeBase64(Large,B,MaxStateBytes));
    std::cout<<"Matches safe integers, canonical opaque cursors, seven statuses and JVM blank rules passed\n";
    std::string Canonical;assert(HorizonMatchesContract::Object(R"({"name":"a","nested":{"ok":1}})",Canonical));
    assert(!HorizonMatchesContract::Object(R"({"name":"a","na\u006de":"b"})",Canonical));
    assert(!HorizonMatchesContract::Object(R"({"reports":[{"revision":1,"revis\u0069on":2}]})",Canonical));
}
