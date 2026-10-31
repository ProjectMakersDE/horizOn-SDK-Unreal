// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Models/HorizonItems.h"
#include "Models/HorizonItemsWire.h"
#include "Http/HorizonLosslessJson.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "String/LexFromString.h"
namespace HorizonItemsJson
{
bool Key(const FString& S){const FTCHARToUTF8 B(*S,S.Len());return HorizonItemsContract::Key(std::string(B.Get(),B.Length()));}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Text(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S,int32 Bytes,bool Empty=false){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::String&&V->TryGetString(S)&&FTCHARToUTF8(*S,S.Len()).Length()<=Bytes&&(Empty||!S.IsEmpty());}
bool Id(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S,36)&&Uuid(S);}
bool ReadKey(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S,64)&&Key(S);}
bool Number(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){auto V=J?J->TryGetField(K):nullptr;FString S;if(!V||V->Type!=EJson::Number||!V->TryGetString(S))return false;const FTCHARToUTF8 Bytes(*S,S.Len());long long Parsed;if(!HorizonItemsContract::Integer(std::string(Bytes.Get(),Bytes.Length()),Parsed))return false;N=Parsed;return true;}
bool ReadRevision(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){return Number(J,K,N)&&N>=0;}
bool Small(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int32& N,int32 Max,bool Zero=false){int64 V;if(!ReadRevision(J,K,V)||V>Max||(!Zero&&V==0))return false;N=int32(V);return true;}
bool Bool(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& B){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Boolean&&V->TryGetBool(B);}
bool Time(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){FDateTime T;return Text(J,K,S,40)&&FDateTime::ParseIso8601(*S,T);}
// Operator quotas are positive Int32 values, independent of retained inventory size.
bool ReadLimits(const TSharedPtr<FJsonObject>& J,FHorizonItemLimits& L){const TSharedPtr<FJsonObject>* O;return J&&J->TryGetObjectField(TEXT("limits"),O)&&Small(*O,TEXT("catalog"),L.Catalog,MAX_int32)&&Small(*O,TEXT("inventory"),L.Inventory,MAX_int32)&&Small(*O,TEXT("receipts"),L.Receipts,MAX_int32);}
bool Properties(const TSharedPtr<FJsonValue>& V,TArray<FHorizonItemProperty>& Out)
{
    if(!V||V->Type!=EJson::Object)return false;FString Raw;
    if(V->PreferStringRepresentation()){if(!V->TryGetString(Raw))return false;}
    else if(!V->AsObject()||!HorizonLosslessJson::Object(V->AsObject().ToSharedRef(),Raw))return false;
    const FTCHARToUTF8 Bytes(*Raw,Raw.Len());std::vector<HorizonItemsContract::Property> Rows;std::string Canonical;if(!HorizonItemsContract::Properties(std::string(Bytes.Get(),Bytes.Length()),Rows,Canonical))return false;
    for(const auto& Row:Rows)
    {
        FHorizonItemProperty P;P.Key=UTF8_TO_TCHAR(Row.Key.c_str());long long N;
        if(HorizonItemsContract::Integer(Row.Token,N)){P.Type=EHorizonItemPropertyType::Integer;P.IntegerValue=N;}
        else if(Row.Token=="true"||Row.Token=="false"){P.Type=EHorizonItemPropertyType::Boolean;P.BooleanValue=Row.Token=="true";}
        else {P.Type=EHorizonItemPropertyType::String;TSharedPtr<FJsonObject> Single;const FString Json=TEXT("{\"value\":")+FString(UTF8_TO_TCHAR(Row.Token.c_str()))+TEXT("}");if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Single)||!Single->TryGetStringField(TEXT("value"),P.StringValue))return false;}
        Out.Add(P);
    }return true;
}
bool Definition(const TSharedPtr<FJsonObject>& J,FHorizonItemDefinition& D)
{
    const TArray<TSharedPtr<FJsonValue>>* Rules=nullptr;
    if(!ReadKey(J,TEXT("key"),D.Key)||!Text(J,TEXT("name"),D.Name,120)||D.Name.TrimStartAndEnd().IsEmpty()||!ReadKey(J,TEXT("type"),D.Type)||!ReadKey(J,TEXT("rarity"),D.Rarity)||!Bool(J,TEXT("active"),D.bActive)||!Properties(J->TryGetField(TEXT("baseProperties")),D.BaseProperties)||!J->TryGetArrayField(TEXT("propertyRules"),Rules)||Rules->Num()>16)return false;
    TSet<FString> Seen;for(auto V:*Rules)
    {
        if(!V||V->Type!=EJson::Object)return false;auto O=V->AsObject();FHorizonItemPropertyRule R;
        if(!ReadKey(O,TEXT("key"),R.Key)||Seen.Contains(R.Key)||!Number(O,TEXT("min"),R.Min)||!Number(O,TEXT("max"),R.Max)||!Number(O,TEXT("maxChangePerCall"),R.MaxChangePerCall)||R.Min>R.Max||R.MaxChangePerCall<=0)return false;
        const auto* Base=D.BaseProperties.FindByPredicate([&](const FHorizonItemProperty& P){return P.Key==R.Key;});if(!Base||Base->Type!=EHorizonItemPropertyType::Integer||Base->IntegerValue<R.Min||Base->IntegerValue>R.Max)return false;
        Seen.Add(R.Key);D.PropertyRules.Add(R);
    }
    FString Json;return HorizonLosslessJson::Object(J.ToSharedRef(),Json)&&FTCHARToUTF8(*Json,Json.Len()).Length()<=2048;
}
}
bool FHorizonItemInstance::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonItemInstance& Out)
{
    using namespace HorizonItemsJson;FHorizonItemInstance I;
    if(!Id(J,TEXT("instanceId"),I.InstanceId)||!ReadKey(J,TEXT("itemKey"),I.ItemKey)||!HorizonItemsJson::Properties(J->TryGetField(TEXT("properties")),I.Properties)||!Bool(J,TEXT("equipped"),I.bEquipped)||!Time(J,TEXT("createdAt"),I.CreatedAt))return false;
    Out=MoveTemp(I);return true;
}
bool FHorizonInventorySnapshot::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonInventorySnapshot& Out)
{
    using namespace HorizonItemsJson;FHorizonInventorySnapshot S;const TArray<TSharedPtr<FJsonValue>>* Rows;
    if(!Id(J,TEXT("userId"),S.UserId)||!ReadRevision(J,TEXT("revision"),S.Revision)||!Time(J,TEXT("serverTime"),S.ServerTime)||!ReadLimits(J,S.Limits)||!J->TryGetArrayField(TEXT("instances"),Rows))return false;
    TSet<FString> Seen;for(auto V:*Rows)
    {
        if(!V||V->Type!=EJson::Object)return false;auto O=V->AsObject();FHorizonItemInstance I;
        if(!FHorizonItemInstance::TryFromJson(O,I)||Seen.Contains(I.InstanceId.ToLower()))return false;
        Seen.Add(I.InstanceId.ToLower());S.Instances.Add(I);
    }S.bPresent=true;Out=MoveTemp(S);return true;
}
bool FHorizonItemCatalogPage::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonItemCatalogPage& Out)
{
    using namespace HorizonItemsJson;FHorizonItemCatalogPage P;const TArray<TSharedPtr<FJsonValue>>* Rows;
    // 65536 permanent UInt16 template slots is a storage-format bound; pages remain at most 50 rows.
    if(!ReadRevision(J,TEXT("revision"),P.Revision)||!Time(J,TEXT("serverTime"),P.ServerTime)||!ReadLimits(J,P.Limits)||!Small(J,TEXT("total"),P.Total,65536,true)||!J->TryGetArrayField(TEXT("definitions"),Rows)||Rows->Num()>50||Rows->Num()>P.Total)return false;
    FString Last;for(auto V:*Rows){FHorizonItemDefinition D;if(!V||V->Type!=EJson::Object||!Definition(V->AsObject(),D)||(!Last.IsEmpty()&&D.Key.Compare(Last,ESearchCase::CaseSensitive)<=0))return false;Last=D.Key;P.Definitions.Add(D);}
    auto Next=J->TryGetField(TEXT("nextAfterKey"));P.bHasNextAfterKey=Next&&!Next->IsNull();if(P.bHasNextAfterKey&&(!ReadKey(J,TEXT("nextAfterKey"),P.NextAfterKey)||P.Definitions.IsEmpty()||P.NextAfterKey!=Last))return false;
    P.bPresent=true;Out=MoveTemp(P);return true;
}
bool FHorizonItemReward::TryArrayFromJson(const TSharedPtr<FJsonObject>& J,const TCHAR* Field,TArray<FHorizonItemReward>& Out)
{
    using namespace HorizonItemsJson;auto Value=J?J->TryGetField(Field):nullptr;if(!Value)return true;if(Value->Type!=EJson::Array||Value->AsArray().Num()>4)return false;
    TArray<FHorizonItemReward> Rows;TSet<FString> Keys;int32 Total=0;
    for(auto V:Value->AsArray()){FHorizonItemReward R;if(!V||V->Type!=EJson::Object||!ReadKey(V->AsObject(),TEXT("itemKey"),R.ItemKey)||Keys.Contains(R.ItemKey)||!Small(V->AsObject(),TEXT("quantity"),R.Quantity,4))return false;Keys.Add(R.ItemKey);Total+=R.Quantity;Rows.Add(R);}if(Total>4)return false;Out=MoveTemp(Rows);return true;
}
bool FHorizonItemReference::TryArrayFromJson(const TSharedPtr<FJsonObject>& J,const TCHAR* Field,TArray<FHorizonItemReference>& Out)
{
    using namespace HorizonItemsJson;auto Value=J?J->TryGetField(Field):nullptr;if(!Value)return true;if(Value->Type!=EJson::Array||Value->AsArray().Num()>4)return false;
    TArray<FHorizonItemReference> Rows;TSet<FString> Ids;for(auto V:Value->AsArray()){FHorizonItemReference R;if(!V||V->Type!=EJson::Object||!Id(V->AsObject(),TEXT("instanceId"),R.InstanceId)||Ids.Contains(R.InstanceId.ToLower())||!ReadKey(V->AsObject(),TEXT("itemKey"),R.ItemKey))return false;Ids.Add(R.InstanceId.ToLower());Rows.Add(R);}Out=MoveTemp(Rows);return true;
}
