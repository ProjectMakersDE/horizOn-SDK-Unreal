#include "Managers/HorizonSharedObjectsManager.h"
#include "Models/HorizonSharedObjectsWire.h"
#include "Http/HorizonLosslessJson.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Base64.h"
namespace HorizonSharedRequest
{
const FString Root=TEXT("/api/v1/app/shared-objects");
bool Key(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return HorizonItemsContract::Key(std::string(B.Get(),B.Length()));}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Revision(int64 R){return R>=0&&R<=HorizonSharedContract::MaxSafe;}
FString UserQuery(const FString& U){return TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U);}
FString Path(const FString& C){return Root+TEXT("/")+C;}
bool Scalar(const FHorizonSharedScalar& V,TSharedPtr<FJsonValue>& Out)
{
    if(V.Type==EHorizonSharedScalarType::Integer){if(V.IntegerValue< -HorizonSharedContract::MaxSafe||V.IntegerValue>HorizonSharedContract::MaxSafe)return false;Out=MakeShared<FJsonValueNumberString>(LexToString(V.IntegerValue));return true;}
    if(V.Type!=EHorizonSharedScalarType::String)return false;
    for(int32 I=0;I<V.StringValue.Len();++I){const uint16 C=V.StringValue[I];if(C>=0xd800&&C<=0xdbff){if(++I>=V.StringValue.Len()||V.StringValue[I]<0xdc00||V.StringValue[I]>0xdfff)return false;}else if(C>=0xdc00&&C<=0xdfff)return false;}
    if(FTCHARToUTF8(*V.StringValue,V.StringValue.Len()+1).Length()>96)return false;Out=MakeShared<FJsonValueString>(V.StringValue);return true;
}
bool Input(const FString& U,const FHorizonSharedObjectInput& I,TSharedPtr<FJsonObject>& B)
{
    if(I.Data.Num()>int32(HorizonSharedContract::MaxObjectBytes)||I.Search.Num()>3||(I.bHasPosition&&(I.Position.X< -HorizonSharedContract::MaxSafe||I.Position.X>HorizonSharedContract::MaxSafe||I.Position.Y< -HorizonSharedContract::MaxSafe||I.Position.Y>HorizonSharedContract::MaxSafe)))return false;
    B=MakeShared<FJsonObject>();B->SetStringField(TEXT("userId"),U);B->SetStringField(TEXT("dataBase64"),FBase64::Encode(I.Data));auto Search=MakeShared<FJsonObject>();TSet<FString> Keys;
    for(const auto& Row:I.Search){TSharedPtr<FJsonValue> V;if(!Key(Row.Key)||Keys.Contains(Row.Key)||!Scalar(Row.Value,V))return false;Keys.Add(Row.Key);Search->SetField(Row.Key,V);}B->SetObjectField(TEXT("search"),Search);
    if(I.bHasPosition){auto P=MakeShared<FJsonObject>();P->SetField(TEXT("x"),MakeShared<FJsonValueNumberString>(LexToString(I.Position.X)));P->SetField(TEXT("y"),MakeShared<FJsonValueNumberString>(LexToString(I.Position.Y)));B->SetObjectField(TEXT("position"),P);}else B->SetField(TEXT("position"),MakeShared<FJsonValueNull>());return true;
}
bool Range(const FHorizonSharedScalar& Low,const FHorizonSharedScalar& High)
{
    if(Low.Type!=High.Type)return false;if(Low.Type==EHorizonSharedScalarType::Integer)return Low.IntegerValue<=High.IntegerValue;
    const FTCHARToUTF8 A(*Low.StringValue,Low.StringValue.Len()+1),B(*High.StringValue,High.StringValue.Len()+1);return std::string(A.Get(),A.Length())<=std::string(B.Get(),B.Length());
}
bool Query(const FString& U,const FHorizonSharedQuery& Q,TSharedPtr<FJsonObject>& B)
{
    if(Q.Limit<1||Q.Limit>50||(!Q.OwnerId.IsEmpty()&&!Uuid(Q.OwnerId))||(!Q.GroupId.IsEmpty()&&!Uuid(Q.GroupId)))return false;
    const FTCHARToUTF8 Cursor(*Q.Cursor,Q.Cursor.Len()+1);if(!Q.Cursor.IsEmpty()&&!HorizonSharedContract::Cursor(std::string(Cursor.Get(),Cursor.Length())))return false;
    B=MakeShared<FJsonObject>();B->SetStringField(TEXT("userId"),U);B->SetNumberField(TEXT("limit"),Q.Limit);if(!Q.OwnerId.IsEmpty())B->SetStringField(TEXT("ownerId"),Q.OwnerId);if(!Q.GroupId.IsEmpty())B->SetStringField(TEXT("groupId"),Q.GroupId);if(!Q.Cursor.IsEmpty())B->SetStringField(TEXT("cursor"),Q.Cursor);
    FString Mode;switch(Q.Mode){case EHorizonSharedQueryMode::List:Mode=TEXT("list");break;case EHorizonSharedQueryMode::Owner:if(Q.OwnerId.IsEmpty())return false;Mode=TEXT("owner");break;case EHorizonSharedQueryMode::Field:Mode=TEXT("field");break;case EHorizonSharedQueryMode::Rectangle:Mode=TEXT("rectangle");break;case EHorizonSharedQueryMode::Random:if(!Q.Cursor.IsEmpty()||Q.bUseEqual||Q.Min.Type!=EHorizonSharedScalarType::Integer||Q.Max.Type!=EHorizonSharedScalarType::Integer)return false;Mode=TEXT("random");break;default:return false;}B->SetStringField(TEXT("mode"),Mode);
    if(Q.Mode==EHorizonSharedQueryMode::Field||Q.Mode==EHorizonSharedQueryMode::Random)
    {
        if(!Key(Q.Field))return false;B->SetStringField(TEXT("field"),Q.Field);TSharedPtr<FJsonValue> V,W;
        if(Q.bUseEqual){if(!Scalar(Q.Equal,V))return false;B->SetField(TEXT("equal"),V);}else {if(!Scalar(Q.Min,V)||!Scalar(Q.Max,W)||!Range(Q.Min,Q.Max))return false;B->SetField(TEXT("min"),V);B->SetField(TEXT("max"),W);}
    }
    if(Q.Mode==EHorizonSharedQueryMode::Rectangle)
    {
        if(Q.MinX>Q.MaxX||Q.MinY>Q.MaxY||Q.MinX< -HorizonSharedContract::MaxSafe||Q.MaxX>HorizonSharedContract::MaxSafe||Q.MinY< -HorizonSharedContract::MaxSafe||Q.MaxY>HorizonSharedContract::MaxSafe)return false;
        B->SetField(TEXT("minX"),MakeShared<FJsonValueNumberString>(LexToString(Q.MinX)));B->SetField(TEXT("maxX"),MakeShared<FJsonValueNumberString>(LexToString(Q.MaxX)));B->SetField(TEXT("minY"),MakeShared<FJsonValueNumberString>(LexToString(Q.MinY)));B->SetField(TEXT("maxY"),MakeShared<FJsonValueNumberString>(LexToString(Q.MaxY)));
    }return true;
}
}
void UHorizonSharedObjectsManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A){HttpClient=H;AuthManager=A;ClearCache();}
void UHorizonSharedObjectsManager::ClearSnapshots(){CurrentCollections={};CurrentUsage={};CurrentObject={};CurrentQuery={};CurrentWrite={};CurrentDelete={};}
void UHorizonSharedObjectsManager::Sync(){const FString I=HttpClient&&AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 C=HttpClient?HttpClient->GetRequestContextEpoch():0;if(I!=Identity||C!=Context){Identity=I;Context=C;++Epoch;++Ticket;Busy=false;Scope.Reset();RevisionFloor=ServerMillis=-1;ClearSnapshots();}}
void UHorizonSharedObjectsManager::ClearCache(){Sync();++Epoch;++Ticket;Busy=false;ClearSnapshots();}
uint64 UHorizonSharedObjectsManager::GetCacheGeneration(){Sync();return Epoch;}
FHorizonNetworkResponse UHorizonSharedObjectsManager::Error(const FString& C){FHorizonNetworkResponse R;R.ServerErrorCode=C;R.ErrorMessage=C;return R;}
bool UHorizonSharedObjectsManager::Accept(const FString& Time,int64 Revision){FDateTime T;if(!FDateTime::ParseIso8601(*Time,T))return false;const int64 Millis=T.ToUnixTimestamp()*1000+T.GetMillisecond();if(Millis<ServerMillis||(Revision>=0&&Revision<RevisionFloor))return false;ServerMillis=Millis;if(Revision>=0)RevisionFloor=Revision;return true;}
void UHorizonSharedObjectsManager::Call(const FString& U,const FString& Collection,const FString& Route,const FString& Verb,const TSharedPtr<FJsonObject>& Body,int64 Revision,FOnHttpResponse Done)
{
    Sync();FString Failure;if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)Failure=TEXT("SESSION_REQUIRED");else if(U!=AuthManager->GetCurrentUser().UserId)Failure=TEXT("SESSION_FORBIDDEN");else if(!HorizonSharedRequest::Uuid(U)||(!Collection.IsEmpty()&&!HorizonSharedRequest::Key(Collection)))Failure=TEXT("INVALID_REQUEST");else if(Busy)Failure=TEXT("REQUEST_IN_PROGRESS");if(!Failure.IsEmpty()){Done.ExecuteIfBound(Error(Failure));return;}
    FString Json;if(Body&&!HorizonLosslessJson::Object(Body.ToSharedRef(),Json)){Done.ExecuteIfBound(Error(TEXT("INVALID_REQUEST")));return;}
    const FString Input=Verb+TEXT("|")+Route+TEXT("|")+LexToString(Revision)+TEXT("|")+Json;const FTCHARToUTF8 Bytes(*Input,Input.Len()+1);const FString NewScope=UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length()).c_str());if(NewScope!=Scope){Scope=NewScope;RevisionFloor=-1;}++Epoch;ClearSnapshots();Busy=true;const uint64 E=Epoch,T=++Ticket,C=Context;TWeakObjectPtr<UHorizonSharedObjectsManager> Weak(this);
    const auto Complete=FOnHttpResponse::CreateLambda([Weak,E,T,C,NewScope,Done](const FHorizonNetworkResponse& Response)
    {
        auto* S=Weak.Get();if(!S)return;S->Sync();if(S->Epoch!=E||S->Ticket!=T||S->Context!=C||S->Scope!=NewScope){Done.ExecuteIfBound(Error(TEXT("STALE_RESPONSE")));return;}S->Busy=false;
        if(!Response.bSuccess){Done.ExecuteIfBound(Response);return;}FHorizonNetworkResponse R=Response;if(R.RawJsonBody.IsEmpty()||!HorizonSharedObjectsWire::Parse(R.RawJsonBody,R.JsonData)){Done.ExecuteIfBound(Error(TEXT("INVALID_RESPONSE")));return;}Done.ExecuteIfBound(R);
    });
    if(Verb==TEXT("GET"))HttpClient->Get(Route,true,Complete);else if(Revision<0)HttpClient->PostJsonRead(Body.ToSharedRef(),Route,Complete);else HttpClient->RevisionJsonOnce(Body,Route,Verb,Revision,Complete);
}
FHorizonSharedCollectionPage UHorizonSharedObjectsManager::GetCurrentCollections(){Sync();return CurrentCollections;}
FHorizonSharedUsage UHorizonSharedObjectsManager::GetCurrentUsage(){Sync();return CurrentUsage;}
FHorizonSharedObjectRead UHorizonSharedObjectsManager::GetCurrentObject(){Sync();return CurrentObject;}
FHorizonSharedQueryResult UHorizonSharedObjectsManager::GetCurrentQuery(){Sync();return CurrentQuery;}
FHorizonSharedWriteResult UHorizonSharedObjectsManager::GetCurrentWrite(){Sync();return CurrentWrite;}
FHorizonSharedDeleteResult UHorizonSharedObjectsManager::GetCurrentDelete(){Sync();return CurrentDelete;}
namespace HorizonSharedRequest
{
template<typename D> bool Failed(const FHorizonNetworkResponse& R,const D& Done){if(R.bSuccess)return false;Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return true;}
template<typename D> void Invalid(const D& Done){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("An explicit valid original request is required"));}
template<typename D> void BadResponse(const D& Done){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Malformed or mismatched authoritative snapshot"));}
template<typename D> void Stale(const D& Done){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older authoritative snapshot"));}
}
void UHorizonSharedObjectsManager::GetCollections(const FString& U,FOnSharedCollectionsComplete Done,const FString& After,int32 Limit,int64 Revision)
{
    if((!After.IsEmpty()&&(!HorizonSharedRequest::Key(After)||Revision<0))||Limit<1||Limit>50||Revision< -1||Revision>HorizonSharedContract::MaxSafe){HorizonSharedRequest::Invalid(Done);return;}
    FString Route=HorizonSharedRequest::Root+TEXT("/collections")+HorizonSharedRequest::UserQuery(U)+TEXT("&limit=")+LexToString(Limit);if(!After.IsEmpty())Route+=TEXT("&afterKey=")+FGenericPlatformHttp::UrlEncode(After);if(Revision>=0)Route+=TEXT("&catalogRevision=")+LexToString(Revision);
    Call(U,TEXT(""),Route,TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,After,Limit,Revision,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedCollectionPage P;
        if(!FHorizonSharedCollectionPage::TryFromJson(R.JsonData,P)||P.Definitions.Num()>Limit||(Revision>=0&&P.Revision!=Revision)||(!After.IsEmpty()&&!P.Definitions.IsEmpty()&&P.Definitions[0].Key.Compare(After,ESearchCase::CaseSensitive)<=0)){HorizonSharedRequest::BadResponse(Done);return;}
        if(!Accept(P.ServerTime,P.Revision)){HorizonSharedRequest::Stale(Done);return;}CurrentCollections=P;Done.ExecuteIfBound(true,P,TEXT(""),TEXT(""));
    }));
}
void UHorizonSharedObjectsManager::GetUsage(const FString& U,const FString& C,FOnSharedUsageComplete Done)
{
    Call(U,C,HorizonSharedRequest::Path(C)+TEXT("/usage")+HorizonSharedRequest::UserQuery(U),TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,U,C,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedUsage V;if(!FHorizonSharedUsage::TryFromJson(R.JsonData,V)||V.UserId!=U||V.Collection!=C){HorizonSharedRequest::BadResponse(Done);return;}if(!Accept(V.ServerTime,V.Revision)){HorizonSharedRequest::Stale(Done);return;}CurrentUsage=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    }));
}
void UHorizonSharedObjectsManager::GetObject(const FString& U,const FString& C,const FString& Id,FOnSharedObjectComplete Done)
{
    if(!HorizonSharedRequest::Uuid(Id)){HorizonSharedRequest::Invalid(Done);return;}
    Call(U,C,HorizonSharedRequest::Path(C)+TEXT("/objects/")+Id+HorizonSharedRequest::UserQuery(U),TEXT("GET"),nullptr,-1,FOnHttpResponse::CreateWeakLambda(this,[this,C,Id,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedObjectRead V;if(!FHorizonSharedObjectRead::TryFromJson(R.JsonData,V)||!V.Object.Id.Equals(Id,ESearchCase::IgnoreCase)||V.Object.Collection!=C){HorizonSharedRequest::BadResponse(Done);return;}if(!Accept(V.ServerTime,V.Object.Revision)){HorizonSharedRequest::Stale(Done);return;}CurrentObject=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    }));
}
void UHorizonSharedObjectsManager::QueryObjects(const FString& U,const FString& C,const FHorizonSharedQuery& Q,FOnSharedQueryComplete Done)
{
    TSharedPtr<FJsonObject> B;if(!HorizonSharedRequest::Query(U,Q,B)){HorizonSharedRequest::Invalid(Done);return;}
    Call(U,C,HorizonSharedRequest::Path(C)+TEXT("/query"),TEXT("POST"),B,-1,FOnHttpResponse::CreateWeakLambda(this,[this,C,Q,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedQueryResult V;if(!FHorizonSharedQueryResult::TryFromJson(R.JsonData,V)||V.Objects.Num()>Q.Limit||(Q.Mode==EHorizonSharedQueryMode::Random&&V.bHasNextCursor)){HorizonSharedRequest::BadResponse(Done);return;}
        for(const auto& O:V.Objects)if(O.Collection!=C||(!Q.OwnerId.IsEmpty()&&!O.OwnerId.Equals(Q.OwnerId,ESearchCase::IgnoreCase))||(!Q.GroupId.IsEmpty()&&(!O.bHasGroupId||!O.GroupId.Equals(Q.GroupId,ESearchCase::IgnoreCase)))){HorizonSharedRequest::BadResponse(Done);return;}
        if(!Accept(V.ServerTime)){HorizonSharedRequest::Stale(Done);return;}CurrentQuery=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    }));
}
void UHorizonSharedObjectsManager::CreateObject(const FString& U,const FString& C,int64 Revision,const FHorizonSharedObjectInput& I,FOnSharedWriteComplete Done,const FString& Group)
{
    TSharedPtr<FJsonObject> B;if(!HorizonSharedRequest::Revision(Revision)||(!Group.IsEmpty()&&!HorizonSharedRequest::Uuid(Group))||!HorizonSharedRequest::Input(U,I,B)){HorizonSharedRequest::Invalid(Done);return;}if(!Group.IsEmpty())B->SetStringField(TEXT("groupId"),Group);
    Call(U,C,HorizonSharedRequest::Path(C)+TEXT("/objects"),TEXT("POST"),B,Revision,FOnHttpResponse::CreateWeakLambda(this,[this,U,C,Group,Revision,I,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedWriteResult V;if(!FHorizonSharedWriteResult::TryFromJson(R.JsonData,V)||V.Object.Collection!=C||V.Object.OwnerId!=U||V.Object.Revision!=1||V.Usage.Revision!=Revision+1||V.Object.Data!=I.Data||V.Object.bHasGroupId!=!Group.IsEmpty()||(!Group.IsEmpty()&&!V.Object.GroupId.Equals(Group,ESearchCase::IgnoreCase))){HorizonSharedRequest::BadResponse(Done);return;}if(!Accept(V.ServerTime)){HorizonSharedRequest::Stale(Done);return;}CurrentWrite=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    }));
}
void UHorizonSharedObjectsManager::UpdateObject(const FString& U,const FString& C,const FString& Id,int64 Revision,const FHorizonSharedObjectInput& I,FOnSharedWriteComplete Done)
{
    TSharedPtr<FJsonObject> B;if(!HorizonSharedRequest::Revision(Revision)||!HorizonSharedRequest::Uuid(Id)||!HorizonSharedRequest::Input(U,I,B)){HorizonSharedRequest::Invalid(Done);return;}
    Call(U,C,HorizonSharedRequest::Path(C)+TEXT("/objects/")+Id,TEXT("PUT"),B,Revision,FOnHttpResponse::CreateWeakLambda(this,[this,C,Id,Revision,I,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedWriteResult V;if(!FHorizonSharedWriteResult::TryFromJson(R.JsonData,V)||V.Object.Collection!=C||!V.Object.Id.Equals(Id,ESearchCase::IgnoreCase)||V.Object.Revision!=Revision+1||V.Object.Data!=I.Data){HorizonSharedRequest::BadResponse(Done);return;}if(!Accept(V.ServerTime)){HorizonSharedRequest::Stale(Done);return;}CurrentWrite=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    }));
}
void UHorizonSharedObjectsManager::DeleteObject(const FString& U,const FString& C,const FString& Id,int64 Revision,FOnSharedDeleteComplete Done)
{
    if(!HorizonSharedRequest::Revision(Revision)||!HorizonSharedRequest::Uuid(Id)){HorizonSharedRequest::Invalid(Done);return;}
    Call(U,C,HorizonSharedRequest::Path(C)+TEXT("/objects/")+Id+HorizonSharedRequest::UserQuery(U),TEXT("DELETE"),nullptr,Revision,FOnHttpResponse::CreateWeakLambda(this,[this,C,Id,Done](const FHorizonNetworkResponse& R)
    {
        if(HorizonSharedRequest::Failed(R,Done))return;FHorizonSharedDeleteResult V;if(!FHorizonSharedDeleteResult::TryFromJson(R.JsonData,V)||V.Usage.Collection!=C||!V.ObjectId.Equals(Id,ESearchCase::IgnoreCase)){HorizonSharedRequest::BadResponse(Done);return;}if(!Accept(V.ServerTime)){HorizonSharedRequest::Stale(Done);return;}CurrentDelete=V;Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    }));
}
