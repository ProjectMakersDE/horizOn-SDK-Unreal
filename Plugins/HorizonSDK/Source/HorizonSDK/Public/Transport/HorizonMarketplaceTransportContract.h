// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Transport/HorizonItemsTransportContract.h"
#include <limits>
namespace HorizonMarketplaceContract
{
// The transport frame is independent of operator quotas: 8 MiB UTF-8 and 64 JSON nesting levels.
constexpr size_t MaxBytes=8*1024*1024;
class Unique
{
    const std::string& S; size_t I=0;
    bool Value(int Depth)
    {
        if(Depth>64||I>=S.size())return false;
        if(S[I]=='"')return HorizonItemsContract::StringEnd(S,I);
        if(S[I]=='{'||S[I]=='[')
        {
            const bool Object=S[I++]=='{';const char End=Object?'}':']';std::set<std::string> Keys;
            if(I<S.size()&&S[I]==End){++I;return true;}
            while(I<S.size())
            {
                if(Object){const size_t Start=I;if(!HorizonItemsContract::StringEnd(S,I)||!Keys.insert(S.substr(Start,I-Start)).second||I>=S.size()||S[I++]!=':')return false;}
                if(!Value(Depth+1)||I>=S.size())return false;if(S[I]==End){++I;return true;}if(S[I++]!=',')return false;
            }return false;
        }
        size_t Start=I;while(I<S.size()&&S[I]!=','&&S[I]!='}'&&S[I]!=']')++I;const auto Token=S.substr(Start,I-Start);
        if(Token=="true"||Token=="false"||Token=="null")return true;long long N;return HorizonItemsContract::Integer(Token,N);
    }
public:explicit Unique(const std::string& Input):S(Input){}bool Valid(){return Value(0)&&I==S.size();}
};
inline bool Mask(const std::string& Raw,const std::string& Prefix,std::vector<std::string>& Properties,std::string& Masked)
{
    return Raw.size()<=MaxBytes&&HorizonGroupsContract::JsonObject(Raw).MaskObjects(Prefix,Properties,Masked,{"properties"},std::numeric_limits<size_t>::max())&&Unique(Masked).Valid();
}
inline long long Fee(long long Price,int Bps){return Price/10000*Bps+Price%10000*Bps/10000;}
inline bool Currency(const std::string& S){if(S.empty()||S.size()>24)return false;for(size_t I=0;I<S.size();++I){char C=S[I];if(!(C>='a'&&C<='z')&&!(C>='0'&&C<='9')&&(I==0||(C!='.'&&C!='_'&&C!='-')))return false;}return true;}
}
