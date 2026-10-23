// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Models/HorizonMatchesWire.h"
#include "Transport/HorizonChatTransportContract.h"
namespace HorizonChatWire
{
inline std::string Utf8(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return std::string(B.Get(),B.Length());}
inline bool Id(const FString& S){return HorizonChatContract::Uuid(Utf8(S));}
inline bool Text(const FString& S,int32 Characters=500,int32 Bytes=2000)
{
    for(int32 I=0;I<S.Len();++I){const uint32 C=uint16(S[I]);if((C<32&&C!=9&&C!=10)||(C>=127&&C<=159))return false;if(C>=0xd800&&C<=0xdbff){if(++I>=S.Len()||S[I]<0xdc00||S[I]>0xdfff)return false;}else if(C>=0xdc00&&C<=0xdfff)return false;}
    return HorizonChatContract::Text(Utf8(S),Characters,Bytes);
}
inline bool Time(const FString& S,FDateTime* Out=nullptr)
{
    FDateTime T;if(S.Len()!=24||S[10]!='T'||S[19]!='.'||S[23]!='Z'||!FDateTime::ParseIso8601(*S,T)||T.ToIso8601()!=S)return false;if(Out)*Out=T;return true;
}
inline bool Parse(const FString& S,TSharedPtr<FJsonObject>& J){return S.Len()<=1048576&&HorizonMatchesWire::Parse(S,J);}
inline bool Exact(const FString& A,const FString& B){return A.Len()==B.Len()&&(!A.Len()||FMemory::Memcmp(*A,*B,A.Len()*sizeof(TCHAR))==0);}
}
