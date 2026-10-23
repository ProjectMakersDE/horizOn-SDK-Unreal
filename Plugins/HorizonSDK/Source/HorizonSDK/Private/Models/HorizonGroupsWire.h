// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Models/HorizonGroups.h"
#include "Transport/HorizonGroupsTransportContract.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
/** Opaque validated data. Never put arbitrary keys into UE's case-insensitive/interned DOM. */
class FHorizonGroupRawJsonValue final : public FJsonValue
{
    FString Json;
public:
    explicit FHorizonGroupRawJsonValue(const FString& Raw):Json(Raw){Type=EJson::Object;}
    virtual bool PreferStringRepresentation() const override {return true;}
    virtual bool TryGetString(FString& Out) const override {Out=Json;return true;}
    virtual SIZE_T GetMemoryFootprint() const override {return sizeof(*this)+Json.GetAllocatedSize();}
protected:
    virtual FString GetType() const override {return TEXT("Object");}
};
namespace HorizonGroupsWire
{
inline bool Restore(const TSharedPtr<FJsonValue>& V,const FString& Prefix,const std::vector<std::string>& Raw,int32& Count,int32 Depth=0)
{
    if(!V||Depth>64)return false;
    if(V->Type==EJson::Array){for(const auto& Item:V->AsArray())if(!Restore(Item,Prefix,Raw,Count,Depth+1))return false;}
    else if(V->Type==EJson::Object)
    {
        for(auto& Pair:V->AsObject()->Values)
        {
            const FStringView Key(*Pair.Key,Pair.Key.Len());
            if(Key==TEXTVIEW("data")&&Pair.Value->Type==EJson::String&&Pair.Value->AsString().StartsWith(Prefix,ESearchCase::CaseSensitive))
            {
                const FString Index=Pair.Value->AsString().Mid(Prefix.Len());int32 N=-1;
                if(Index.IsEmpty()||!LexTryParseString(N,*Index)||N<0||size_t(N)>=Raw.size()||Index!=LexToString(N))return false;
                FHorizonGroupJsonData Data;if(!FHorizonGroupJsonData::TryFromJson(UTF8_TO_TCHAR(Raw[N].c_str()),Data))return false;
                Pair.Value=MakeShared<FHorizonGroupRawJsonValue>(Data.GetJson());++Count;
            }
            else if(!Restore(Pair.Value,Prefix,Raw,Count,Depth+1))return false;
        }
    }
    return true;
}
inline bool Parse(const FString& Body,TSharedPtr<FJsonObject>& Out)
{
    const FTCHARToUTF8 Bytes(*Body);const FString Prefix=TEXT("horizon-opaque-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("-");
    // Only Groups budgets the already received aggregate from its actual raw byte length.
    std::vector<std::string> Data;std::string Masked;
    if(!HorizonGroupsContract::JsonObject(std::string(Bytes.Get(),Bytes.Length()),1024,size_t(Bytes.Length())).MaskObjects(TCHAR_TO_UTF8(*Prefix),Data,Masked))return false;
    TSharedPtr<FJsonObject> Json;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Masked.c_str())),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings)||!Json)return false;
    int32 Restored=0;if(!Restore(MakeShared<FJsonValueObject>(Json),Prefix,Data,Restored)||size_t(Restored)!=Data.size())return false;Out=MoveTemp(Json);return true;
}
}
