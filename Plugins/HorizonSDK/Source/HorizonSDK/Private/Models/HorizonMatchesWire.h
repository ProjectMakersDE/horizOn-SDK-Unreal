#pragma once
#include "CoreMinimal.h"
#include "Transport/HorizonMatchesTransportContract.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace HorizonMatchesWire
{
inline bool TextValue(const FString& S,int32 Maximum,bool Nonblank=false)
{
    if(S.Len()>Maximum)return false;bool Blank=true;
    for(int32 I=0;I<S.Len();++I){uint32 C=uint16(S[I]);if(C>=0xd800&&C<=0xdbff){if(++I>=S.Len()||S[I]<0xdc00||S[I]>0xdfff)return false;C=0x10000+((C-0xd800)<<10)+(uint16(S[I])-0xdc00);}else if(C>=0xdc00&&C<=0xdfff)return false;Blank=Blank&&HorizonMatchesContract::Whitespace(C);}
    return FTCHARToUTF8(*S,S.Len()+1).Length()<=Maximum&&(!Nonblank||!Blank);
}

inline bool Parse(const FString& Body,TSharedPtr<FJsonObject>& Out)
{
    const FTCHARToUTF8 Bytes(*Body,Body.Len()+1);std::string Canonical;
    if(!HorizonMatchesContract::Object(std::string(Bytes.Get(),Bytes.Length()),Canonical))return false;
    TSharedPtr<FJsonObject> Json;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Canonical.c_str())),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings)||!Json)return false;Out=MoveTemp(Json);return true;
}
}
