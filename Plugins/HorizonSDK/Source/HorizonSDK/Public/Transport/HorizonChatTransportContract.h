#pragma once
#include "Transport/HorizonMatchesTransportContract.h"
namespace HorizonChatContract
{
inline bool Uuid(const std::string& S)
{
    if(S.size()!=36)return false;for(size_t I=0;I<36;++I){const char C=S[I];if(I==8||I==13||I==18||I==23){if(C!='-')return false;}else if(!(C>='0'&&C<='9')&&!(C>='a'&&C<='f'))return false;}return true;
}
inline bool Text(const std::string& S,size_t MaxCharacters=500,size_t MaxBytes=2000)
{
    if(S.empty()||S.size()>MaxBytes)return false;size_t Count=0;bool Blank=true;
    for(size_t I=0;I<S.size();)
    {
        uint32_t C=uint8_t(S[I++]);int Extra=0;uint32_t Minimum=0;
        if(C>=0xc2&&C<=0xdf){Extra=1;C&=31;Minimum=0x80;}
        else if(C>=0xe0&&C<=0xef){Extra=2;C&=15;Minimum=0x800;}
        else if(C>=0xf0&&C<=0xf4){Extra=3;C&=7;Minimum=0x10000;}
        else if(C>=128)return false;
        for(int J=0;J<Extra;++J){if(I>=S.size()||(uint8_t(S[I])&0xc0)!=0x80)return false;C=(C<<6)|(uint8_t(S[I++])&63);}
        if(C<Minimum||C>0x10ffff||(C>=0xd800&&C<=0xdfff)||(C<32&&C!=9&&C!=10)||(C>=127&&C<=159)||++Count>MaxCharacters)return false;
        Blank=Blank&&(HorizonMatchesContract::Whitespace(C)||C==0xa0||C==0x2007||C==0x202f);
    }return !Blank;
}
inline bool Revision(long long N){return HorizonMatchesContract::Revision(N);}
}
