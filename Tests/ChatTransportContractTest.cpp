#include "Transport/HorizonChatTransportContract.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace HorizonChatContract;
    assert(Uuid("00000000-0000-4000-8000-000000001014"));assert(!Uuid("00000000-0000-4000-8000-00000000A014"));assert(!Uuid("0"));
    assert(Text("snow 雪 😀\n\t"));assert(!Text("   "));assert(!Text(" \n\t"));assert(!Text(std::string("a\0b",3)));assert(!Text("a\r"));assert(!Text("\xc2\x85"));assert(!Text("\xed\xa0\x80"));assert(!Text("\xc0\x80"));assert(!Text("\xf4\x90\x80\x80"));
    std::string Emoji;for(int I=0;I<500;++I)Emoji+="😀";assert(Text(Emoji));assert(!Text(Emoji+"a"));assert(!Text(std::string(501,'a')));assert(Text(std::string(240,'x'),240,960));assert(!Text(std::string(241,'x'),240,960));
    assert(Revision(9007199254740991LL));assert(!Revision(9007199254740992LL));assert(!Revision(-1));
    std::string J;assert(HorizonMatchesContract::Object("{\"text\":\"雪\\n😀\"}",J));assert(!HorizonMatchesContract::Object("{\"id\":1,\"i\\u0064\":2}",J));assert(!HorizonMatchesContract::Object("{\"text\":\"\\ud800\"}",J));
    std::cout<<"Chat transport: scalar/UTF-8/control/UUID/duplicate-key bounds passed\n";
}
