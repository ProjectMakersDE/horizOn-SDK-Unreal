// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonItemsManager.h"
#include "Models/HorizonItemsWire.h"
#include "Transport/HorizonItemsTransportContract.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Dom/JsonValue.h"
namespace HorizonItemsRequest
{
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Key(const FString& S){const FTCHARToUTF8 B(*S,S.Len());return HorizonItemsContract::Key(std::string(B.Get(),B.Length()));}
int64 Millis(const FString& S){FDateTime T;return FDateTime::ParseIso8601(*S,T)?T.ToUnixTimestamp()*1000+T.GetMillisecond():-1;}
const FString Root=TEXT("/api/v1/app/items");
}
void UHorizonItemsManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A){HttpClient=H;AuthManager=A;ClearCache();}
void UHorizonItemsManager::Sync()
{
    const FString Next=HttpClient&&AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 E=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if(Next!=Identity||E!=Context){Identity=Next;Context=E;++Epoch;++Ticket;bBusy=false;InventoryRevision=CatalogRevisionFloor=ServerMillis=-1;Inventory={};Catalog={};}
}
void UHorizonItemsManager::ClearCache(){Sync();++Epoch;++Ticket;bBusy=false;Inventory={};Catalog={};}
uint64 UHorizonItemsManager::GetCacheGeneration(){Sync();return Epoch;}
FHorizonInventorySnapshot UHorizonItemsManager::GetCurrentInventory(){Sync();return Inventory;}
FHorizonItemCatalogPage UHorizonItemsManager::GetCurrentCatalog(){Sync();return Catalog;}
FHorizonNetworkResponse UHorizonItemsManager::Error(const FString& Code){FHorizonNetworkResponse R;R.ServerErrorCode=Code;R.ErrorMessage=Code;return R;}
void UHorizonItemsManager::Call(const FString& U,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Done)
{
    Sync();FString Failure;
    if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)Failure=TEXT("SESSION_REQUIRED");
    else if(U!=AuthManager->GetCurrentUser().UserId)Failure=TEXT("SESSION_FORBIDDEN");else if(!HorizonItemsRequest::Uuid(U))Failure=TEXT("INVALID_REQUEST");else if(bBusy)Failure=TEXT("REQUEST_IN_PROGRESS");
    if(!Failure.IsEmpty()){Done.ExecuteIfBound(Error(Failure));return;}
    bBusy=true;if(Body){++Epoch;Inventory={};}const uint64 T=++Ticket,E=Epoch,C=Context;const FString I=Identity;TWeakObjectPtr<UHorizonItemsManager> Weak(this);
    auto Complete=FOnHttpResponse::CreateLambda([Weak,T,E,C,I,Done](const FHorizonNetworkResponse& Response)
    {
        auto* Self=Weak.Get();if(!Self)return;Self->Sync();if(Self->Ticket!=T||Self->Epoch!=E||Self->Context!=C||Self->Identity!=I){Done.ExecuteIfBound(Error(TEXT("STALE_RESPONSE")));return;}
        Self->bBusy=false;if(Response.StatusCode==401||Response.StatusCode==403)Self->ClearCache();FHorizonNetworkResponse R=Response;
        if(R.bSuccess&&(R.RawJsonBody.IsEmpty()||!HorizonItemsWire::Parse(R.RawJsonBody,R.JsonData))){Done.ExecuteIfBound(Error(TEXT("INVALID_RESPONSE")));return;}Done.ExecuteIfBound(R);
    });
    if(Body)HttpClient->JsonOnce(Body.ToSharedRef(),Route,false,true,Complete);else HttpClient->Get(Route,true,Complete);
}
void UHorizonItemsManager::ReceiveInventory(const FString& U,const FHorizonNetworkResponse& R,FOnInventoryComplete Done)
{
    if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonInventorySnapshot S;
    if(!FHorizonInventorySnapshot::TryFromJson(R.JsonData,S)||S.UserId!=U){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid inventory snapshot"));return;}
    const int64 Time=HorizonItemsRequest::Millis(S.ServerTime);if(S.Revision<InventoryRevision||Time<ServerMillis){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older inventory snapshot"));return;}
    InventoryRevision=S.Revision;ServerMillis=Time;Inventory=S;Done.ExecuteIfBound(true,S,TEXT(""),TEXT(""));
}
void UHorizonItemsManager::GetInventory(const FString& U,FOnInventoryComplete Done)
{Call(U,HorizonItemsRequest::Root+TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U),nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,U,Done](const FHorizonNetworkResponse& R){ReceiveInventory(U,R,Done);}));}
void UHorizonItemsManager::GetCatalog(const FString& U,FOnItemCatalogComplete Done,const FString& After,int32 Limit,int64 Revision)
{
    if((!After.IsEmpty()&&(!HorizonItemsRequest::Key(After)||Revision<0))||Limit<1||Limit>50||Revision< -1||Revision>HorizonItemsContract::MaxSafe){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Explicit bounded cursor and original catalog revision required"));return;}
    FString Route=HorizonItemsRequest::Root+TEXT("/catalog?userId=")+FGenericPlatformHttp::UrlEncode(U)+TEXT("&limit=")+LexToString(Limit);if(!After.IsEmpty())Route+=TEXT("&afterKey=")+FGenericPlatformHttp::UrlEncode(After);if(Revision>=0)Route+=TEXT("&catalogRevision=")+LexToString(Revision);
    Call(U,Route,nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,After,Limit,Revision,Done](const FHorizonNetworkResponse& R)
    {
        if(!R.bSuccess){Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}FHorizonItemCatalogPage P;
        if(!FHorizonItemCatalogPage::TryFromJson(R.JsonData,P)||P.Definitions.Num()>Limit||(Revision>=0&&P.Revision!=Revision)||(!After.IsEmpty()&&!P.Definitions.IsEmpty()&&P.Definitions[0].Key.Compare(After,ESearchCase::CaseSensitive)<=0)){Done.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid or mixed-revision catalog page"));return;}
        const int64 Time=HorizonItemsRequest::Millis(P.ServerTime);if(P.Revision<CatalogRevisionFloor||Time<ServerMillis){Done.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older catalog page"));return;}
        CatalogRevisionFloor=P.Revision;ServerMillis=Time;Catalog=P;Done.ExecuteIfBound(true,P,TEXT(""),TEXT(""));
    }));
}
void UHorizonItemsManager::Mutate(const FString& U,const FString& Id,int64 Revision,const FString& Action,const TArray<FHorizonItemPropertyDelta>& Changes,bool Equipped,FOnInventoryComplete Done)
{
    if(!HorizonItemsRequest::Uuid(Id)||Revision<0||Revision>HorizonItemsContract::MaxSafe||(Action==TEXT("adjust")&&(Changes.IsEmpty()||Changes.Num()>8))){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Original instance and safe revision required"));return;}
    auto B=MakeShared<FJsonObject>();B->SetStringField(TEXT("userId"),U);B->SetStringField(TEXT("instanceId"),Id);B->SetField(TEXT("expectedRevision"),MakeShared<FJsonValueNumberString>(LexToString(Revision)));
    if(Action==TEXT("adjust"))
    {
        TSet<FString> Keys;TArray<TSharedPtr<FJsonValue>> Deltas;
        for(const auto& D:Changes){if(!HorizonItemsRequest::Key(D.Key)||Keys.Contains(D.Key)||D.Amount==0||D.Amount< -HorizonItemsContract::MaxSafe||D.Amount>HorizonItemsContract::MaxSafe){Done.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Unique nonzero safe deltas required"));return;}Keys.Add(D.Key);auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("key"),D.Key);J->SetField(TEXT("amount"),MakeShared<FJsonValueNumberString>(LexToString(D.Amount)));Deltas.Add(MakeShared<FJsonValueObject>(J));}
        B->SetArrayField(TEXT("changes"),Deltas);
    }
    else if(Action==TEXT("equip"))B->SetBoolField(TEXT("equipped"),Equipped);
    Call(U,HorizonItemsRequest::Root+TEXT("/")+Action,B,FOnHttpResponse::CreateWeakLambda(this,[this,U,Done](const FHorizonNetworkResponse& R){ReceiveInventory(U,R,Done);}));
}
void UHorizonItemsManager::AdjustProperties(const FString& U,const FString& I,int64 R,const TArray<FHorizonItemPropertyDelta>& C,FOnInventoryComplete Done){Mutate(U,I,R,TEXT("adjust"),C,false,Done);}
void UHorizonItemsManager::SetEquipped(const FString& U,const FString& I,int64 R,bool B,FOnInventoryComplete Done){Mutate(U,I,R,TEXT("equip"),{},B,Done);}
void UHorizonItemsManager::DiscardItem(const FString& U,const FString& I,int64 R,FOnInventoryComplete Done){Mutate(U,I,R,TEXT("discard"),{},false,Done);}
FOnInventoryComplete UHorizonItemsManager::InventoryEvent(){return FOnInventoryComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonInventorySnapshot& S,const FString& C,const FString& M){OnInventoryResult.Broadcast(Ok,S,C,M);});}
void UHorizonItemsManager::RequestInventory(const FString& U){GetInventory(U,InventoryEvent());}
void UHorizonItemsManager::RequestCatalog(const FString& U,const FString& A,int32 L,int64 R){GetCatalog(U,FOnItemCatalogComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonItemCatalogPage& P,const FString& C,const FString& M){OnCatalogResult.Broadcast(Ok,P,C,M);}),A,L,R);}
void UHorizonItemsManager::AdjustItemProperties(const FString& U,const FString& I,int64 R,const TArray<FHorizonItemPropertyDelta>& C){AdjustProperties(U,I,R,C,InventoryEvent());}
void UHorizonItemsManager::EquipItem(const FString& U,const FString& I,int64 R,bool B){SetEquipped(U,I,R,B,InventoryEvent());}
void UHorizonItemsManager::DiscardInventoryItem(const FString& U,const FString& I,int64 R){DiscardItem(U,I,R,InventoryEvent());}
