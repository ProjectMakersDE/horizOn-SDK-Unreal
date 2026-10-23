// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Transport/HorizonGroupsTransportContract.h"
#include <algorithm>
#include <set>
namespace HorizonItemsContract
{
constexpr long long MaxSafe=9007199254740991LL;
inline bool Key(const std::string& S){if(S.empty()||S.size()>64||S[0]<'a'||S[0]>'z')return false;for(char C:S)if(!(C>='a'&&C<='z')&&!(C>='0'&&C<='9')&&C!='_')return false;return true;}
inline bool Integer(const std::string& S,long long& Out)
{
    if(S.empty()||S.size()>17)return false;size_t I=S[0]=='-'?1:0;if(I==S.size()||(S[I]=='0'&&S.size()-I>1)||(I==1&&S.substr(I)=="0"))return false;
    long long N=0;for(;I<S.size();++I){if(S[I]<'0'||S[I]>'9')return false;int D=S[I]-'0';if(N>(MaxSafe-D)/10)return false;N=N*10+D;}Out=S[0]=='-'?-N:N;return true;
}
struct Property {std::string Key,Token;};
inline bool StringEnd(const std::string& S,size_t& I)
{if(I>=S.size()||S[I++]!='"')return false;while(I<S.size()){char C=S[I++];if(C=='"')return true;if(C=='\\'){if(I>=S.size())return false;++I;}}return false;}
// Property values never pass through a floating-point or null-terminated string conversion.
inline bool Properties(const std::string& Raw,std::vector<Property>& Rows,std::string& Canonical)
{
    std::string S;if(!HorizonGroupsContract::CompactObject(Raw,S))return false;size_t I=1;std::vector<Property> Parsed;std::set<std::string> Keys;
    if(S[I]!='}')while(true)
    {
        const size_t K=I;if(!StringEnd(S,I))return false;std::string Key=S.substr(K+1,I-K-2);if(!HorizonItemsContract::Key(Key)||!Keys.insert(Key).second||Parsed.size()>=16||S[I++]!=':')return false;
        const size_t V=I;if(S[I]=='"'){if(!StringEnd(S,I))return false;}
        else {while(I<S.size()&&S[I]!=','&&S[I]!='}')++I;const std::string T=S.substr(V,I-V);long long Number;if(T!="true"&&T!="false"&&!Integer(T,Number))return false;}
        Parsed.push_back({Key,S.substr(V,I-V)});if(I>=S.size())return false;if(S[I]=='}')break;if(S[I++]!=',')return false;
    }
    if(I+1!=S.size())return false;std::sort(Parsed.begin(),Parsed.end(),[](const Property& A,const Property& B){return A.Key<B.Key;});std::string Result="{";
    for(const auto& P:Parsed){if(Result.size()>1)Result+=',';Result+='"';Result+=P.Key;Result+="\":";Result+=P.Token;}Result+='}';if(Result.size()>256)return false;Rows=std::move(Parsed);Canonical=std::move(Result);return true;
}
}
