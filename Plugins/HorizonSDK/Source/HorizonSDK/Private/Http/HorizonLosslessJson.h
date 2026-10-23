// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"
namespace HorizonLosslessJson
{
using FWriter=TJsonWriter<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>;
inline bool Value(const TSharedPtr<FJsonValue>& V,FWriter& Writer,int32 Depth=0)
{
    if(!V||Depth>64)return false;
    switch(V->Type)
    {
    case EJson::Null:Writer.WriteNull();return true;
    case EJson::Boolean:Writer.WriteValue(V->AsBool());return true;
    case EJson::String:Writer.WriteValue(V->AsString());return true;
    case EJson::Number:
        if(V->PreferStringRepresentation())Writer.WriteRawJSONValue(V->AsString());
        else {if(!FMath::IsFinite(V->AsNumber()))return false;Writer.WriteValue(V->AsNumber());}return true;
    case EJson::Array:
        Writer.WriteArrayStart();for(const auto& Item:V->AsArray())if(!Value(Item,Writer,Depth+1))return false;Writer.WriteArrayEnd();return true;
    case EJson::Object:
        if(V->PreferStringRepresentation()){FString Raw;if(!V->TryGetString(Raw))return false;Writer.WriteRawJSONValue(Raw);return true;}
        if(!V->AsObject())return false;Writer.WriteObjectStart();
        for(const auto& Pair:V->AsObject()->Values)
        {
            // FJsonSerializer builds its identifier from *Key, truncating embedded NULs.
            // Both legacy FString and UE5.8 shared-string keys expose an explicit length.
            Writer.WriteIdentifierPrefix(FStringView(*Pair.Key,Pair.Key.Len()));
            if(!Value(Pair.Value,Writer,Depth+1))return false;
        }
        Writer.WriteObjectEnd();return true;
    default:return false;
    }
}
inline bool Object(const TSharedRef<FJsonObject>& J,FString& Out)
{auto Writer=TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);return Value(MakeShared<FJsonValueObject>(J),*Writer)&&Writer->Close();}
}
