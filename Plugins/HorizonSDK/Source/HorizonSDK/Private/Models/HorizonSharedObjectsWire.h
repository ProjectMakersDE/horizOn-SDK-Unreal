// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Models/HorizonSharedObjects.h"
#include "Transport/HorizonSharedObjectsTransportContract.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
class FHorizonSharedRawSearchJsonValue final : public FJsonValue
{
    FString Json;
public:
    explicit FHorizonSharedRawSearchJsonValue(const FString& Value):Json(Value){Type=EJson::Object;}
    virtual bool PreferStringRepresentation() const override {return true;}
    virtual bool TryGetString(FString& Out) const override {Out=Json;return true;}
    virtual SIZE_T GetMemoryFootprint() const override {return sizeof(*this)+Json.GetAllocatedSize();}
protected: virtual FString GetType() const override {return TEXT("Object");}
};
namespace HorizonSharedObjectsWire
{
inline bool Restore(const TSharedPtr<FJsonValue>& V,const FString& Prefix,const std::vector<std::string>& Raw,int32& Count,int32 Depth=0)
{
    if(!V||Depth>64)return false;
    if(V->Type==EJson::Array){for(const auto& Item:V->AsArray())if(!Restore(Item,Prefix,Raw,Count,Depth+1))return false;}
    else if(V->Type==EJson::Object)for(auto& Pair:V->AsObject()->Values)
    {
        const FStringView Key(*Pair.Key,Pair.Key.Len());const bool Property=Key.Equals(TEXTVIEW("search"),ESearchCase::CaseSensitive);
        if(Property && Pair.Value->Type==EJson::String && Pair.Value->AsString().StartsWith(Prefix,ESearchCase::CaseSensitive))
        {
            const FString Index=Pair.Value->AsString().Mid(Prefix.Len());int32 N=-1;if(Index.IsEmpty()||!LexTryParseString(N,*Index)||N<0||size_t(N)>=Raw.size()||Index!=LexToString(N))return false;
            std::vector<HorizonSharedContract::SearchValue> Rows;std::string Canonical;if(!HorizonSharedContract::Search(Raw[N],Rows,Canonical))return false;
            Pair.Value=MakeShared<FHorizonSharedRawSearchJsonValue>(UTF8_TO_TCHAR(Canonical.c_str()));++Count;
        }
        else if(!Restore(Pair.Value,Prefix,Raw,Count,Depth+1))return false;
    }
    return true;
}
inline bool Parse(const FString& Body,TSharedPtr<FJsonObject>& Out)
{
    const FTCHARToUTF8 Bytes(*Body,Body.Len()+1);const FString Prefix=TEXT("horizon-shared-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("-");std::vector<std::string> Raw;std::string Masked;
    if(!HorizonGroupsContract::JsonObject(std::string(Bytes.Get(),Bytes.Length())).MaskObjects(TCHAR_TO_UTF8(*Prefix),Raw,Masked,{"search"},128))return false;
    TSharedPtr<FJsonObject> Json;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Masked.c_str())),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings)||!Json)return false;
    int32 Count=0;if(!Restore(MakeShared<FJsonValueObject>(Json),Prefix,Raw,Count)||size_t(Count)!=Raw.size())return false;Out=MoveTemp(Json);return true;
}
}
