// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Transport/HorizonItemsTransportContract.h"
#include <cstdint>
#include <limits>
namespace HorizonSharedContract
{
constexpr long long MaxSafe=HorizonItemsContract::MaxSafe;
constexpr size_t MaxObjectBytes=1048576,MaxPageBytes=1048576;
inline bool Cursor(const std::string& S){if(S.empty()||S.size()>2048)return false;for(char C:S)if(!(C>='a'&&C<='z')&&!(C>='A'&&C<='Z')&&!(C>='0'&&C<='9')&&C!='_'&&C!='-')return false;return true;}
inline bool DecodeString(const std::string& S,std::string& Out)
{
    if(S.size()<2||S.front()!='"'||S.back()!='"')return false;std::string V;
    for(size_t I=1;I+1<S.size();++I)
    {
        char C=S[I];if(C!='\\'){V+=C;continue;}if(++I+1>=S.size())return false;
        switch(S[I]){case '"':V+='"';break;case '\\':V+='\\';break;case '/':V+='/';break;case 'b':V+='\b';break;case 'f':V+='\f';break;case 'n':V+='\n';break;case 'r':V+='\r';break;case 't':V+='\t';break;
        case 'u':{if(I+4>=S.size()||S[I+1]!='0'||S[I+2]!='0')return false;auto Hex=[](char X){return X>='0'&&X<='9'?X-'0':X>='a'&&X<='f'?X-'a'+10:-1;};int A=Hex(S[I+3]),B=Hex(S[I+4]);if(A<0||B<0||A*16+B>31)return false;V+=char(A*16+B);I+=4;break;}default:return false;}
    }Out=std::move(V);return true;
}
struct SearchValue {std::string Key;bool IsString=false;long long Integer=0;std::string String;};
inline bool Search(const std::string& Raw,std::vector<SearchValue>& Out,std::string& Canonical)
{
    std::string S;if(!HorizonGroupsContract::CompactObject(Raw,S))return false;size_t I=1;std::vector<SearchValue> Rows;std::set<std::string> Seen;
    if(S[I]!='}')while(true)
    {
        size_t K=I;if(!HorizonItemsContract::StringEnd(S,I))return false;SearchValue V;V.Key=S.substr(K+1,I-K-2);if(!HorizonItemsContract::Key(V.Key)||!Seen.insert(V.Key).second||Rows.size()>=3||S[I++]!=':')return false;
        size_t B=I;if(S[I]=='"'){if(!HorizonItemsContract::StringEnd(S,I)||!DecodeString(S.substr(B,I-B),V.String)||V.String.size()>96)return false;V.IsString=true;}
        else {while(I<S.size()&&S[I]!=','&&S[I]!='}')++I;if(!HorizonItemsContract::Integer(S.substr(B,I-B),V.Integer))return false;}
        Rows.push_back(std::move(V));if(I>=S.size())return false;if(S[I]=='}')break;if(S[I++]!=',')return false;
    }
    if(I+1!=S.size())return false;Out=std::move(Rows);Canonical=std::move(S);return true;
}
inline int Digit(char C){if(C>='A'&&C<='Z')return C-'A';if(C>='a'&&C<='z')return C-'a'+26;if(C>='0'&&C<='9')return C-'0'+52;return C=='+'?62:C=='/'?63:-1;}
inline bool DecodeBase64(const std::string& S,std::vector<uint8_t>& Out,size_t Maximum=MaxObjectBytes)
{
    if(Maximum>std::numeric_limits<size_t>::max()/4*3-2||S.size()%4||S.size()>((Maximum+2)/3)*4)return false;std::vector<uint8_t> Bytes;Bytes.reserve(S.size()/4*3);
    for(size_t I=0;I<S.size();I+=4)
    {
        int A=Digit(S[I]),B=Digit(S[I+1]),C=Digit(S[I+2]),D=Digit(S[I+3]);const bool Last=I+4==S.size();
        if(A<0||B<0)return false;Bytes.push_back(uint8_t((A<<2)|(B>>4)));
        if(S[I+2]=='='){if(!Last||S[I+3]!='='||(B&15))return false;}
        else {if(C<0)return false;Bytes.push_back(uint8_t((B<<4)|(C>>2)));if(S[I+3]=='='){if(!Last||(C&3))return false;}else {if(D<0)return false;Bytes.push_back(uint8_t((C<<6)|D));}}
        if(Bytes.size()>Maximum)return false;
    }Out=std::move(Bytes);return true;
}
}
