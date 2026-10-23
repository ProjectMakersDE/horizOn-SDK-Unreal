// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonMarketplaceManager.h"
#include "Models/HorizonMarketplaceWire.h"
#include "Http/HorizonLosslessJson.h"
#include "GenericPlatform/GenericPlatformHttp.h"
namespace HorizonMarketplaceRequest
{
const FString Root=TEXT("/api/v1/app/marketplace");
FString Query(const FString& U){return TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U);}
TSharedRef<FJsonObject> Head(const FString& U,const FString& O,const FString& T){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("userId"),U);J->SetStringField(TEXT("operationId"),O);J->SetStringField(TEXT("issuedAt"),T);return J;}
FHorizonMarketplaceIntent Intent(const FString& Kind,const FString& Listing,const TSharedRef<FJsonObject>& J){FHorizonMarketplaceIntent I;I.Kind=Kind;I.ListingId=Listing;if(!HorizonLosslessJson::Object(J,I.Json)||!I.IsValid())return {};return I;}
}
void UHorizonMarketplaceManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A){HttpClient=H;AuthManager=A;Sync();}
void UHorizonMarketplaceManager::Sync()
{
    const FString Next=HttpClient&&AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 C=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if(Next!=Identity||C!=Context){Identity=Next;Context=C;++Epoch;++Ticket;Busy=false;ServerMillis=StateRevision=ConfigRevision=-1;Cache.Reset();}
}
void UHorizonMarketplaceManager::ClearCache(){Sync();++Epoch;++Ticket;Busy=false;Cache.Reset();}
FHorizonMarketplaceResponse UHorizonMarketplaceManager::GetCurrent(const FString& Kind){Sync();const auto* P=Cache.Find(Kind);return P?*P:FHorizonMarketplaceResponse{};}
int64 UHorizonMarketplaceManager::GetServerNow(){Sync();return ServerMillis<0?-1:ServerMillis+int64((FPlatformTime::Seconds()-Clock)*1000);}
void UHorizonMarketplaceManager::Fail(FOnMarketplaceComplete Done,const FString& Code){Done.ExecuteIfBound(false,{},Code,Code);}
void UHorizonMarketplaceManager::Call(const FString& U,const FString& Route,const FString& Kind,const FString& Body,FOnMarketplaceComplete Done,TFunction<bool(const FHorizonMarketplaceResponse&)> Valid)
{
    using namespace HorizonMarketplaceWire;Sync();
    if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken){Fail(Done,TEXT("SESSION_REQUIRED"));return;}
    if(U!=AuthManager->GetCurrentUser().UserId){Fail(Done,TEXT("SESSION_FORBIDDEN"));return;}if(!Id(U)){Fail(Done,TEXT("INVALID_REQUEST"));return;}if(Busy){Fail(Done,TEXT("REQUEST_IN_PROGRESS"));return;}
    Busy=true;const uint64 E=Epoch,T=++Ticket,C=Context;const FString I=Identity;TWeakObjectPtr<UHorizonMarketplaceManager> Weak(this);
    auto Complete=FOnHttpResponse::CreateLambda([Weak,E,T,C,I,U,Kind,Body,Done,Valid](const FHorizonNetworkResponse& R)
    {
        auto* Self=Weak.Get();if(!Self)return;Self->Sync();if(Self->Epoch!=E||Self->Ticket!=T||Self->Context!=C||Self->Identity!=I){Fail(Done,TEXT("STALE_RESPONSE"));return;}Self->Busy=false;
        if(!R.bSuccess){if(R.StatusCode==401||R.StatusCode==403)Self->ClearCache();Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}
        FHorizonMarketplaceResponse V;if(!FHorizonMarketplaceResponse::TryFromJson(R.RawJsonBody,Kind,V)||!Valid(V)){Fail(Done,TEXT("INVALID_RESPONSE"));return;}
        const int64 Time=Millis(V.ServerTime);const auto* Point=Self->Cache.Find(TEXT("point"));
        if(Time<Self->ServerMillis||Kind==TEXT("state")&&V.State.Revision<Self->StateRevision||Kind==TEXT("limits")&&V.Config.Revision<Self->ConfigRevision||Kind==TEXT("rules")&&V.CatalogRevision<Self->ConfigRevision||Kind==TEXT("point")&&Point&&Point->Listing.Id==V.Listing.Id&&Point->Listing.Revision>V.Listing.Revision){Fail(Done,TEXT("STALE_RESPONSE"));return;}
        Self->ServerMillis=Time;Self->Clock=FPlatformTime::Seconds();if(Kind==TEXT("state"))Self->StateRevision=V.State.Revision;if(Kind==TEXT("limits"))Self->ConfigRevision=V.Config.Revision;if(Kind==TEXT("rules"))Self->ConfigRevision=V.CatalogRevision;
        if(Body.IsEmpty()){if(Kind!=TEXT("operation"))Self->Cache.Add(Kind,V);}else{Self->Cache.Remove(TEXT("state"));Self->Cache.Remove(TEXT("query"));Self->Cache.Remove(TEXT("point"));Self->TradingStateInvalidated.Broadcast();}
        Self->Sync();if(Self->Epoch!=E||Self->Context!=C||Self->Identity!=I){Fail(Done,TEXT("STALE_RESPONSE"));return;}Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    });
    if(Body.IsEmpty())HttpClient->Get(Route,true,Complete);else HttpClient->PostOriginalJsonOnce(Body,Route,Complete);
}
void UHorizonMarketplaceManager::GetLimits(const FString& U,FOnMarketplaceComplete D){Call(U,HorizonMarketplaceRequest::Root+TEXT("/limits")+HorizonMarketplaceRequest::Query(U),TEXT("limits"),TEXT(""),D,[](const auto&){return true;});}
void UHorizonMarketplaceManager::GetRules(const FString& U,FOnMarketplaceComplete D,const FString& After,int32 Limit,int64 Revision)
{
    using namespace HorizonMarketplaceWire;if(Limit<1||Limit>50||Revision< -1||Revision>HorizonItemsContract::MaxSafe||!After.IsEmpty()&&(!Key(After)||Revision<0)){Fail(D,TEXT("INVALID_REQUEST"));return;}
    FString Route=HorizonMarketplaceRequest::Root+TEXT("/rules")+HorizonMarketplaceRequest::Query(U)+TEXT("&limit=")+LexToString(Limit);if(!After.IsEmpty())Route+=TEXT("&after=")+FGenericPlatformHttp::UrlEncode(After);if(Revision>=0)Route+=TEXT("&catalogRevision=")+LexToString(Revision);
    Call(U,Route,TEXT("rules"),TEXT(""),D,[After,Limit,Revision](const auto& V){return V.Rules.Num()<=Limit&&(Revision<0||V.CatalogRevision==Revision)&&(After.IsEmpty()||V.Rules.IsEmpty()||V.Rules[0].ItemKey.Compare(After,ESearchCase::CaseSensitive)>0);});
}
void UHorizonMarketplaceManager::GetMarketplace(const FString& U,FOnMarketplaceComplete D){Call(U,HorizonMarketplaceRequest::Root+HorizonMarketplaceRequest::Query(U),TEXT("state"),TEXT(""),D,[U](const auto& V){return V.State.UserId==U;});}
void UHorizonMarketplaceManager::QueryListings(const FString& U,FOnMarketplaceComplete D,const FString& Key,int64 Min,int64 Max,const FString& Cursor,int32 Limit)
{
    using namespace HorizonMarketplaceWire;if(Limit<1||Limit>50||Min<0||Max>HorizonItemsContract::MaxSafe||Min>Max||!Key.IsEmpty()&&!HorizonMarketplaceWire::Key(Key)||!Cursor.IsEmpty()&&!HorizonMarketplaceWire::Cursor(Cursor)){Fail(D,TEXT("INVALID_REQUEST"));return;}
    FString Route=HorizonMarketplaceRequest::Root+TEXT("/listings")+HorizonMarketplaceRequest::Query(U)+TEXT("&minPrice=")+LexToString(Min)+TEXT("&maxPrice=")+LexToString(Max)+TEXT("&limit=")+LexToString(Limit);if(!Key.IsEmpty())Route+=TEXT("&itemKey=")+FGenericPlatformHttp::UrlEncode(Key);if(!Cursor.IsEmpty())Route+=TEXT("&cursor=")+FGenericPlatformHttp::UrlEncode(Cursor);
    Call(U,Route,TEXT("query"),TEXT(""),D,[Key,Min,Max,Limit](const auto& V){if(V.Listings.Num()>Limit)return false;const FHorizonMarketplaceListing* Previous=nullptr;for(const auto& L:V.Listings){if(L.Status!=TEXT("active")||!Key.IsEmpty()&&L.ItemReference.ItemKey!=Key||L.Price<Min||L.Price>Max||Previous&&(Previous->Price>L.Price||Previous->Price==L.Price&&Previous->Id.Compare(L.Id,ESearchCase::CaseSensitive)>=0))return false;Previous=&L;}return true;});
}
void UHorizonMarketplaceManager::GetListing(const FString& U,const FString& Id,FOnMarketplaceComplete D){if(!HorizonMarketplaceWire::Id(Id)){Fail(D,TEXT("INVALID_REQUEST"));return;}Call(U,HorizonMarketplaceRequest::Root+TEXT("/listings/")+Id+HorizonMarketplaceRequest::Query(U),TEXT("point"),TEXT(""),D,[Id](const auto& V){return V.Listing.Id==Id;});}
void UHorizonMarketplaceManager::GetOperation(const FString& U,const FString& Id,FOnMarketplaceComplete D){if(!HorizonMarketplaceWire::Id(Id)){Fail(D,TEXT("INVALID_REQUEST"));return;}Call(U,HorizonMarketplaceRequest::Root+TEXT("/operations/")+Id+HorizonMarketplaceRequest::Query(U),TEXT("operation"),TEXT(""),D,[Id](const auto& V){return V.Receipt.OperationId==Id;});}
void UHorizonMarketplaceManager::SubmitPrepared(const FHorizonMarketplaceIntent& I,FOnMarketplaceComplete D)
{
    if(!I.IsValid()){Fail(D,TEXT("INVALID_REQUEST"));return;}const FString Path=HorizonMarketplaceRequest::Root+TEXT("/listings")+(I.Kind==TEXT("create")?FString():TEXT("/")+I.ListingId+TEXT("/")+I.Kind);
    Call(I.User(),Path,TEXT("written"),I.Json,D,[I](const auto& V){return I.Matches(V.Receipt);});
}
void UHorizonMarketplaceManager::RetryReturn(const FString& U,const FString& Id,FOnMarketplaceComplete D)
{
    if(!HorizonMarketplaceWire::Id(U)||!HorizonMarketplaceWire::Id(Id)){Fail(D,TEXT("INVALID_REQUEST"));return;}auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("userId"),U);FString Body;HorizonLosslessJson::Object(J,Body);
    Call(U,HorizonMarketplaceRequest::Root+TEXT("/listings/")+Id+TEXT("/retry-return"),TEXT("return"),Body,D,[U,Id](const auto& V){return V.Listing.Id==Id&&V.Listing.ReturnOwnerId==U;});
}
FHorizonMarketplaceIntent UHorizonMarketplaceManager::PrepareCreate(const FString& U,const FString& O,const FString& T,const FString& I,int64 Inv,int64 Config,int64 Price,int64 Duration)
{auto J=HorizonMarketplaceRequest::Head(U,O,T);J->SetStringField(TEXT("instanceId"),I);for(const auto& P:{TPair<const TCHAR*,int64>(TEXT("expectedInventoryRevision"),Inv),{TEXT("expectedConfigRevision"),Config},{TEXT("price"),Price},{TEXT("durationSeconds"),Duration}})HorizonMarketplaceWire::AddNumber(J,P.Key,P.Value);return HorizonMarketplaceRequest::Intent(TEXT("create"),TEXT(""),J);}
FHorizonMarketplaceIntent UHorizonMarketplaceManager::PrepareBuy(const FString& U,const FString& L,const FString& O,const FString& T,int64 Rev,int64 Price,const FString& Currency)
{auto J=HorizonMarketplaceRequest::Head(U,O,T);HorizonMarketplaceWire::AddNumber(J,TEXT("expectedListingRevision"),Rev);HorizonMarketplaceWire::AddNumber(J,TEXT("expectedPrice"),Price);J->SetStringField(TEXT("expectedCurrencyKey"),Currency);return HorizonMarketplaceRequest::Intent(TEXT("buy"),L,J);}
FHorizonMarketplaceIntent UHorizonMarketplaceManager::PrepareCancel(const FString& U,const FString& L,const FString& O,const FString& T,int64 Rev)
{auto J=HorizonMarketplaceRequest::Head(U,O,T);HorizonMarketplaceWire::AddNumber(J,TEXT("expectedListingRevision"),Rev);return HorizonMarketplaceRequest::Intent(TEXT("cancel"),L,J);}
FOnMarketplaceComplete UHorizonMarketplaceManager::Event(){return FOnMarketplaceComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonMarketplaceResponse& V,const FString& C,const FString& M){OnResult.Broadcast(Ok,V,C,M);});}
void UHorizonMarketplaceManager::ReadLimits(const FString& U){GetLimits(U,Event());}
void UHorizonMarketplaceManager::ReadRules(const FString& U,const FString& A,int32 L,int64 R){GetRules(U,Event(),A,L,R);}
void UHorizonMarketplaceManager::ReadMarketplace(const FString& U){GetMarketplace(U,Event());}
void UHorizonMarketplaceManager::ReadListings(const FString& U,const FString& K,int64 Min,int64 Max,const FString& C,int32 L){QueryListings(U,Event(),K,Min,Max,C,L);}
void UHorizonMarketplaceManager::ReadListing(const FString& U,const FString& I){GetListing(U,I,Event());}
void UHorizonMarketplaceManager::SubmitOriginal(const FHorizonMarketplaceIntent& I){SubmitPrepared(I,Event());}
void UHorizonMarketplaceManager::LookupOriginal(const FString& U,const FString& O){GetOperation(U,O,Event());}
void UHorizonMarketplaceManager::RetryReturnDelivery(const FString& U,const FString& L){RetryReturn(U,L,Event());}
