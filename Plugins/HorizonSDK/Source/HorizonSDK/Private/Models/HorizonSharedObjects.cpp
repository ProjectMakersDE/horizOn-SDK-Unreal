#include "Models/HorizonSharedObjects.h"
#include "Models/HorizonSharedObjectsWire.h"
#include "Http/HorizonLosslessJson.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
namespace HorizonSharedJson
{
bool Key(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return HorizonItemsContract::Key(std::string(B.Get(),B.Length()));}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Text(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S,int32 Max,bool Empty=false){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::String&&V->TryGetString(S)&&FTCHARToUTF8(*S,S.Len()+1).Length()<=Max&&(Empty||!S.IsEmpty());}
bool ReadKey(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S,64)&&Key(S);}
bool Id(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S,36)&&Uuid(S);}
bool Number(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){auto V=J?J->TryGetField(K):nullptr;FString S;if(!V||V->Type!=EJson::Number||!V->TryGetString(S))return false;const FTCHARToUTF8 B(*S,S.Len()+1);long long P;if(!HorizonItemsContract::Integer(std::string(B.Get(),B.Length()),P))return false;N=P;return true;}
bool ReadRevision(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){return Number(J,K,N)&&N>=0;}
bool Small(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int32& N,int32 Max,bool Zero=false){int64 V;if(!ReadRevision(J,K,V)||V>Max||(!Zero&&V==0))return false;N=int32(V);return true;}
bool Bool(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& B){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Boolean&&V->TryGetBool(B);}
bool Time(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){FDateTime T;return Text(J,K,S,40)&&S.Len()==FCString::Strlen(*S)&&FDateTime::ParseIso8601(*S,T);}
bool Envelope(const TSharedPtr<FJsonObject>& J,int32& Poll,FString& Server){return Small(J,TEXT("pollAfterSeconds"),Poll,MAX_int32)&&Time(J,TEXT("serverTime"),Server);}
bool ReadLimits(const TSharedPtr<FJsonObject>& J,FHorizonSharedLimits& L){const TSharedPtr<FJsonObject>* O;return J&&J->TryGetObjectField(TEXT("limits"),O)&&Small(*O,TEXT("collections"),L.Collections,MAX_int32)&&ReadRevision(*O,TEXT("storageBytes"),L.StorageBytes)&&L.StorageBytes>0&&Small(*O,TEXT("objectBytes"),L.ObjectBytes,1048576);}
bool Null(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& Present){auto V=J?J->TryGetField(K):nullptr;if(!V)return false;Present=!V->IsNull();return true;}
bool ReadSearch(const TSharedPtr<FJsonValue>& V,TArray<FHorizonSharedSearchValue>& Out)
{
    if(!V||V->Type!=EJson::Object)return false;FString Raw;if(V->PreferStringRepresentation()){if(!V->TryGetString(Raw))return false;}else if(!V->AsObject()||!HorizonLosslessJson::Object(V->AsObject().ToSharedRef(),Raw))return false;
    const FTCHARToUTF8 B(*Raw,Raw.Len()+1);std::vector<HorizonSharedContract::SearchValue> Rows;std::string Canonical;if(!HorizonSharedContract::Search(std::string(B.Get(),B.Length()),Rows,Canonical))return false;
    for(const auto& Row:Rows)
    {
        FHorizonSharedSearchValue P;P.Key=UTF8_TO_TCHAR(Row.Key.c_str());P.Value.Type=Row.IsString?EHorizonSharedScalarType::String:EHorizonSharedScalarType::Integer;P.Value.IntegerValue=Row.Integer;
        if(Row.IsString)
        {
            // A sentinel prevents StringConv from trimming a terminal NUL. FString(count,ptr)
            // skips a leading NUL, so populate its explicit char array instead.
            std::string WithSentinel=Row.String;WithSentinel+='x';const FUTF8ToTCHAR T(WithSentinel.data(),int32(WithSentinel.size()));const int32 Count=T.Length()-1;
            auto& Chars=P.Value.StringValue.GetCharArray();Chars.SetNumUninitialized(Count+1);if(Count)FMemory::Memcpy(Chars.GetData(),T.Get(),Count*sizeof(TCHAR));Chars[Count]=0;
        }Out.Add(P);
    }return true;
}
bool Position(const TSharedPtr<FJsonObject>& J,FHorizonSharedPosition& P,bool& Has){if(!Null(J,TEXT("position"),Has))return false;if(!Has)return true;const TSharedPtr<FJsonObject>* O;return J->TryGetObjectField(TEXT("position"),O)&&Number(*O,TEXT("x"),P.X)&&Number(*O,TEXT("y"),P.Y);}
bool ReadObject(const TSharedPtr<FJsonObject>& J,FHorizonSharedObject& O)
{
    FString Encoded;
    if(!Id(J,TEXT("id"),O.Id)||!ReadKey(J,TEXT("collection"),O.Collection)||!Id(J,TEXT("ownerId"),O.OwnerId)||!Null(J,TEXT("groupId"),O.bHasGroupId)||(O.bHasGroupId&&!Id(J,TEXT("groupId"),O.GroupId))||!ReadRevision(J,TEXT("revision"),O.Revision)||O.Revision==0||!Text(J,TEXT("dataBase64"),Encoded,1398104,true)||!ReadSearch(J->TryGetField(TEXT("search")),O.Search)||!Position(J,O.Position,O.bHasPosition)||!Time(J,TEXT("createdAt"),O.CreatedAt)||!Time(J,TEXT("updatedAt"),O.UpdatedAt)||!Null(J,TEXT("expiresAt"),O.bHasExpiresAt)||(O.bHasExpiresAt&&!Time(J,TEXT("expiresAt"),O.ExpiresAt)))return false;
    const FTCHARToUTF8 B(*Encoded,Encoded.Len()+1);std::vector<uint8_t> Bytes;if(!HorizonSharedContract::DecodeBase64(std::string(B.Get(),B.Length()),Bytes))return false;if(!Bytes.empty())O.Data.Append(Bytes.data(),int32(Bytes.size()));O.bPresent=true;return true;
}
bool Definition(const TSharedPtr<FJsonObject>& J,FHorizonSharedDefinition& D)
{
    const TArray<TSharedPtr<FJsonValue>>* Fields;
    if(!ReadKey(J,TEXT("key"),D.Key)||!Text(J,TEXT("name"),D.Name,120)||D.Name.TrimStartAndEnd().IsEmpty()||!Bool(J,TEXT("active"),D.bActive)||!Text(J,TEXT("readPolicy"),D.ReadPolicy,16)||!Text(J,TEXT("writePolicy"),D.WritePolicy,16)||!Small(J,TEXT("maxObjectBytes"),D.MaxObjectBytes,1048576)||!Small(J,TEXT("maxObjectsPerOwner"),D.MaxObjectsPerOwner,MAX_int32)||!Null(J,TEXT("ttlSeconds"),D.bHasTtlSeconds)||(D.bHasTtlSeconds&&(!Number(J,TEXT("ttlSeconds"),D.TtlSeconds)||D.TtlSeconds<60||D.TtlSeconds>2592000))||!Bool(J,TEXT("positionEnabled"),D.bPositionEnabled)||!Bool(J,TEXT("used"),D.bUsed)||!ReadRevision(J,TEXT("revision"),D.Revision)||!J->TryGetArrayField(TEXT("searchFields"),Fields)||Fields->Num()>3)return false;
    if(D.ReadPolicy!=TEXT("owner")&&D.ReadPolicy!=TEXT("friends")&&D.ReadPolicy!=TEXT("group")&&D.ReadPolicy!=TEXT("all")&&D.ReadPolicy!=TEXT("dashboard"))return false;if(D.WritePolicy!=TEXT("owner")&&D.WritePolicy!=TEXT("group")&&D.WritePolicy!=TEXT("dashboard"))return false;if(D.ReadPolicy==TEXT("dashboard")&&D.WritePolicy!=TEXT("dashboard"))return false;
    TSet<FString> Keys;for(auto V:*Fields){FHorizonSharedSearchField F;FString Type;if(!V||V->Type!=EJson::Object||!ReadKey(V->AsObject(),TEXT("key"),F.Key)||Keys.Contains(F.Key)||!Text(V->AsObject(),TEXT("type"),Type,8)||(Type!=TEXT("integer")&&Type!=TEXT("string")))return false;Keys.Add(F.Key);F.Type=Type==TEXT("string")?EHorizonSharedScalarType::String:EHorizonSharedScalarType::Integer;D.SearchFields.Add(F);}return true;
}
}
bool FHorizonSharedCollectionPage::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonSharedCollectionPage& Out)
{
    using namespace HorizonSharedJson;FHorizonSharedCollectionPage P;const TArray<TSharedPtr<FJsonValue>>* Rows;
    if(!ReadRevision(J,TEXT("revision"),P.Revision)||!J->TryGetArrayField(TEXT("definitions"),Rows)||Rows->Num()>50||!ReadLimits(J,P.Limits)||!Time(J,TEXT("serverTime"),P.ServerTime)||!Null(J,TEXT("nextAfterKey"),P.bHasNextAfterKey)||(P.bHasNextAfterKey&&!ReadKey(J,TEXT("nextAfterKey"),P.NextAfterKey)))return false;
    FString Last;for(auto V:*Rows){FHorizonSharedDefinition D;if(!V||V->Type!=EJson::Object||!Definition(V->AsObject(),D)||(!Last.IsEmpty()&&Last.Compare(D.Key,ESearchCase::CaseSensitive)>=0))return false;Last=D.Key;P.Definitions.Add(D);}if(P.bHasNextAfterKey&&(P.Definitions.IsEmpty()||P.NextAfterKey!=Last))return false;P.bPresent=true;Out=MoveTemp(P);return true;
}
bool FHorizonSharedUsage::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonSharedUsage& Out)
{
    using namespace HorizonSharedJson;FHorizonSharedUsage U;if(!Id(J,TEXT("userId"),U.UserId)||!ReadKey(J,TEXT("collection"),U.Collection)||!ReadRevision(J,TEXT("revision"),U.Revision)||!Small(J,TEXT("count"),U.Count,MAX_int32,true)||!ReadLimits(J,U.Limits)||!Envelope(J,U.PollAfterSeconds,U.ServerTime))return false;U.bPresent=true;Out=MoveTemp(U);return true;
}
bool FHorizonSharedObjectRead::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonSharedObjectRead& Out)
{
    using namespace HorizonSharedJson;FHorizonSharedObjectRead V;const TSharedPtr<FJsonObject>* O;if(!J||!J->TryGetObjectField(TEXT("object"),O)||!ReadObject(*O,V.Object)||!Envelope(J,V.PollAfterSeconds,V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonSharedQueryResult::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonSharedQueryResult& Out)
{
    using namespace HorizonSharedJson;FHorizonSharedQueryResult R;const TArray<TSharedPtr<FJsonValue>>* Rows;if(!J||!J->TryGetArrayField(TEXT("objects"),Rows)||Rows->Num()>50||!Null(J,TEXT("nextCursor"),R.bHasNextCursor)||!Envelope(J,R.PollAfterSeconds,R.ServerTime))return false;
    if(R.bHasNextCursor){if(!Text(J,TEXT("nextCursor"),R.NextCursor,2048))return false;const FTCHARToUTF8 B(*R.NextCursor,R.NextCursor.Len()+1);if(!HorizonSharedContract::Cursor(std::string(B.Get(),B.Length())))return false;}
    TSet<FString> Ids;int32 Bytes=0;for(auto V:*Rows){FHorizonSharedObject O;if(!V||V->Type!=EJson::Object||!ReadObject(V->AsObject(),O)||Ids.Contains(O.Id.ToLower()))return false;Ids.Add(O.Id.ToLower());Bytes+=O.Data.Num();if(Bytes>1048576)return false;R.Objects.Add(O);}R.bPresent=true;Out=MoveTemp(R);return true;
}
bool FHorizonSharedWriteResult::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonSharedWriteResult& Out)
{
    using namespace HorizonSharedJson;FHorizonSharedWriteResult R;const TSharedPtr<FJsonObject> *O,*U;if(!J||!J->TryGetObjectField(TEXT("object"),O)||!ReadObject(*O,R.Object)||!J->TryGetObjectField(TEXT("usage"),U)||!FHorizonSharedUsage::TryFromJson(*U,R.Usage)||R.Usage.UserId!=R.Object.OwnerId||R.Usage.Collection!=R.Object.Collection||!Envelope(J,R.PollAfterSeconds,R.ServerTime))return false;R.bPresent=true;Out=MoveTemp(R);return true;
}
bool FHorizonSharedDeleteResult::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonSharedDeleteResult& Out)
{
    using namespace HorizonSharedJson;FHorizonSharedDeleteResult R;const TSharedPtr<FJsonObject>* U;if(!Id(J,TEXT("objectId"),R.ObjectId)||!Bool(J,TEXT("deleted"),R.bDeleted)||!R.bDeleted||!J->TryGetObjectField(TEXT("usage"),U)||!FHorizonSharedUsage::TryFromJson(*U,R.Usage)||!Envelope(J,R.PollAfterSeconds,R.ServerTime))return false;R.bPresent=true;Out=MoveTemp(R);return true;
}
