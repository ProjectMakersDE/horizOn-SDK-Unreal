// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Transport/HorizonSharedObjectsTransportContract.h"
#include <set>
namespace HorizonMatchesContract
{
constexpr long long MaxSafe=HorizonSharedContract::MaxSafe;
constexpr size_t MaxStateBytes=14648*1024,MaxResultBytes=256;
inline bool Safe(long long N){return N>=-MaxSafe&&N<=MaxSafe;}
inline bool Revision(long long N){return N>=0&&N<=MaxSafe;}
inline bool Cursor(const std::string& S)
{
    if(S.empty()||S.size()>256)return false;std::vector<uint8_t> Bytes;
    if(!HorizonSharedContract::DecodeBase64(S,Bytes,192))return false;
    for(uint8_t C:Bytes)if(C>127)return false;return true;
}
inline bool Status(const std::string& S){return S=="lobby"||S=="active"||S=="reporting"||S=="confirmed"||S=="disputed"||S=="unconfirmed"||S=="cancelled";}
inline bool Whitespace(uint32_t C){return (C>=9&&C<=13)||(C>=28&&C<=32)||C==0x1680||(C>=0x2000&&C<=0x2006)||(C>=0x2008&&C<=0x200a)||C==0x2028||C==0x2029||C==0x205f||C==0x3000;}
// CompactObject has already normalized escape spellings. Reject duplicate keys before the DOM
// could silently collapse distinct source fields; nested match/report objects are checked too.
class UniqueObjectKeys
{
    const std::string& S;size_t I=0;
    bool String(std::string* Out=nullptr){if(I>=S.size()||S[I]!='"')return false;const size_t Start=I++;while(I<S.size()){const char C=S[I++];if(C=='"'){if(Out)*Out=S.substr(Start,I-Start);return true;}if(C=='\\'&&I<S.size())++I;}return false;}
    bool Value(int Depth=0)
    {
        if(Depth>16||I>=S.size())return false;const char C=S[I];if(C=='"')return String();
        if(C=='{'||C=='['){const bool Object=C=='{';const char End=Object?'}':']';++I;std::set<std::string> Keys;if(I<S.size()&&S[I]==End){++I;return true;}
            while(I<S.size()){if(Object){std::string Key;if(!String(&Key)||!Keys.insert(Key).second||I>=S.size()||S[I++]!=':')return false;}if(!Value(Depth+1)||I>=S.size())return false;if(S[I]==End){++I;return true;}if(S[I++]!=',')return false;}return false;}
        const size_t Start=I;while(I<S.size()&&S[I]!=','&&S[I]!='}'&&S[I]!=']')++I;return I>Start;
    }
public: explicit UniqueObjectKeys(const std::string& Canonical):S(Canonical){}bool Parse(){return Value()&&I==S.size();}
};
inline bool Object(const std::string& Raw,std::string& Canonical){return HorizonGroupsContract::CompactObject(Raw,Canonical)&&UniqueObjectKeys(Canonical).Parse();}

}
