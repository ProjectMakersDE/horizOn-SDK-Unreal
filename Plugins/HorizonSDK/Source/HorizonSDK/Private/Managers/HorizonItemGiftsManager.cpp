// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonItemGiftsManager.h"
#include "Models/HorizonMarketplaceWire.h"
#include "Http/HorizonLosslessJson.h"
#include "GenericPlatform/GenericPlatformHttp.h"
namespace HorizonItemGiftsRequest
{
const FString Root=TEXT("/api/v1/app/item-gifts");
FString Query(const FString& U){return TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U);}
}
void UHorizonItemGiftsManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A){HttpClient=H;AuthManager=A;Sync();}
void UHorizonItemGiftsManager::Sync()
{
    const FString Next=HttpClient&&AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 C=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if(Next!=Identity||C!=Context){Identity=Next;Context=C;++Epoch;++Ticket;Busy=false;ServerMillis=StateRevision=ConfigRevision=-1;Cache.Reset();}
}
void UHorizonItemGiftsManager::ClearCache(){Sync();++Epoch;++Ticket;Busy=false;Cache.Reset();}
FHorizonItemGiftResponse UHorizonItemGiftsManager::GetCurrent(const FString& Kind){Sync();const auto* P=Cache.Find(Kind);return P?*P:FHorizonItemGiftResponse{};}
int64 UHorizonItemGiftsManager::GetServerNow(){Sync();return ServerMillis<0?-1:ServerMillis+int64((FPlatformTime::Seconds()-Clock)*1000);}
void UHorizonItemGiftsManager::Fail(FOnItemGiftComplete Done,const FString& Code){Done.ExecuteIfBound(false,{},Code,Code);}
void UHorizonItemGiftsManager::Call(const FString& U,const FString& Route,const FString& Kind,const FString& Body,FOnItemGiftComplete Done,TFunction<bool(const FHorizonItemGiftResponse&)> Valid)
{
    using namespace HorizonMarketplaceWire;Sync();
    if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken){Fail(Done,TEXT("SESSION_REQUIRED"));return;}
    if(U!=AuthManager->GetCurrentUser().UserId){Fail(Done,TEXT("SESSION_FORBIDDEN"));return;}if(!Id(U)){Fail(Done,TEXT("INVALID_REQUEST"));return;}if(Busy){Fail(Done,TEXT("REQUEST_IN_PROGRESS"));return;}
    Busy=true;const uint64 E=Epoch,T=++Ticket,C=Context;const FString I=Identity;TWeakObjectPtr<UHorizonItemGiftsManager> Weak(this);
    auto Complete=FOnHttpResponse::CreateLambda([Weak,E,T,C,I,Kind,Body,Done,Valid](const FHorizonNetworkResponse& R)
    {
        auto* Self=Weak.Get();if(!Self)return;Self->Sync();if(Self->Epoch!=E||Self->Ticket!=T||Self->Context!=C||Self->Identity!=I){Fail(Done,TEXT("STALE_RESPONSE"));return;}Self->Busy=false;
        if(!R.bSuccess){if(R.StatusCode==401||R.StatusCode==403)Self->ClearCache();Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return;}
        FHorizonItemGiftResponse V;if(!FHorizonItemGiftResponse::TryFromJson(R.RawJsonBody,Kind,V)||!Valid(V)){Fail(Done,TEXT("INVALID_RESPONSE"));return;}
        const int64 Time=Millis(V.ServerTime);const auto* Point=Self->Cache.Find(TEXT("point"));
        if(Time<Self->ServerMillis||Kind==TEXT("state")&&V.State.Revision<Self->StateRevision||Kind==TEXT("config")&&V.Config.Revision<Self->ConfigRevision||Kind==TEXT("point")&&Point&&Point->Gift.Id==V.Gift.Id&&Point->Gift.Revision>V.Gift.Revision){Fail(Done,TEXT("STALE_RESPONSE"));return;}
        Self->ServerMillis=Time;Self->Clock=FPlatformTime::Seconds();if(Kind==TEXT("state"))Self->StateRevision=V.State.Revision;if(Kind==TEXT("config"))Self->ConfigRevision=V.Config.Revision;
        if(Body.IsEmpty()){if(Kind!=TEXT("operation"))Self->Cache.Add(Kind,V);}else{Self->Cache.Remove(TEXT("state"));Self->Cache.Remove(TEXT("point"));Self->GiftStateInvalidated.Broadcast();}
        Self->Sync();if(Self->Epoch!=E||Self->Context!=C||Self->Identity!=I){Fail(Done,TEXT("STALE_RESPONSE"));return;}Done.ExecuteIfBound(true,V,TEXT(""),TEXT(""));
    });
    if(Body.IsEmpty())HttpClient->Get(Route,true,Complete);else HttpClient->PostOriginalJsonOnce(Body,Route,Complete);
}
void UHorizonItemGiftsManager::GetConfig(const FString& U,FOnItemGiftComplete D){Call(U,HorizonItemGiftsRequest::Root+TEXT("/config")+HorizonItemGiftsRequest::Query(U),TEXT("config"),TEXT(""),D,[](const auto&){return true;});}
void UHorizonItemGiftsManager::GetState(const FString& U,FOnItemGiftComplete D){Call(U,HorizonItemGiftsRequest::Root+HorizonItemGiftsRequest::Query(U),TEXT("state"),TEXT(""),D,[U](const auto& V){return V.State.UserId==U;});}
void UHorizonItemGiftsManager::GetGift(const FString& U,const FString& Id,FOnItemGiftComplete D)
{
    if(!HorizonMarketplaceWire::Id(Id)){Fail(D,TEXT("INVALID_REQUEST"));return;}
    Call(U,HorizonItemGiftsRequest::Root+TEXT("/")+Id+HorizonItemGiftsRequest::Query(U),TEXT("point"),TEXT(""),D,[U,Id](const auto& V){return V.Gift.Id==Id&&(V.Gift.SenderId==U||V.Gift.RecipientId==U||V.Gift.ReturnOwnerId==U);});
}
void UHorizonItemGiftsManager::GetOperation(const FString& U,const FString& Id,FOnItemGiftComplete D)
{
    if(!HorizonMarketplaceWire::Id(Id)){Fail(D,TEXT("INVALID_REQUEST"));return;}
    Call(U,HorizonItemGiftsRequest::Root+TEXT("/operations/")+Id+HorizonItemGiftsRequest::Query(U),TEXT("operation"),TEXT(""),D,[Id](const auto& V){return V.Receipt.OperationId==Id;});
}
void UHorizonItemGiftsManager::SubmitPrepared(const FHorizonItemGiftIntent& I,FOnItemGiftComplete D)
{
    if(!I.IsValid()){Fail(D,TEXT("INVALID_REQUEST"));return;}
    Call(I.User(),HorizonItemGiftsRequest::Root,TEXT("written"),I.Json,D,[I](const auto& V){return I.Matches(V.Receipt);});
}
void UHorizonItemGiftsManager::Decline(const FString& U,const FString& Id,int64 Revision,FOnItemGiftComplete D)
{
    if(!HorizonMarketplaceWire::Id(U)||!HorizonMarketplaceWire::Id(Id)||Revision<0||Revision>HorizonItemsContract::MaxSafe){Fail(D,TEXT("INVALID_REQUEST"));return;}
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("userId"),U);HorizonMarketplaceWire::AddNumber(J,TEXT("expectedGiftRevision"),Revision);FString Body;HorizonLosslessJson::Object(J,Body);
    Call(U,HorizonItemGiftsRequest::Root+TEXT("/")+Id+TEXT("/decline"),TEXT("declined"),Body,D,[U,Id](const auto& V){return V.Gift.Id==Id&&V.Gift.RecipientId==U;});
}
FHorizonItemGiftIntent UHorizonItemGiftsManager::PrepareSend(const FString& U,const FString& O,const FString& T,const FString& R,const FString& I,int64 Inv,int64 Config)
{
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("userId"),U);J->SetStringField(TEXT("operationId"),O);J->SetStringField(TEXT("issuedAt"),T);J->SetStringField(TEXT("recipientId"),R);J->SetStringField(TEXT("instanceId"),I);
    HorizonMarketplaceWire::AddNumber(J,TEXT("expectedInventoryRevision"),Inv);HorizonMarketplaceWire::AddNumber(J,TEXT("expectedConfigRevision"),Config);
    FHorizonItemGiftIntent Intent;if(!HorizonLosslessJson::Object(J,Intent.Json)||!Intent.IsValid())return {};return Intent;
}
FOnItemGiftComplete UHorizonItemGiftsManager::Event(){return FOnItemGiftComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonItemGiftResponse& V,const FString& C,const FString& M){OnResult.Broadcast(Ok,V,C,M);});}
void UHorizonItemGiftsManager::ReadConfig(const FString& U){GetConfig(U,Event());}
void UHorizonItemGiftsManager::ReadState(const FString& U){GetState(U,Event());}
void UHorizonItemGiftsManager::ReadGift(const FString& U,const FString& I){GetGift(U,I,Event());}
void UHorizonItemGiftsManager::SubmitOriginal(const FHorizonItemGiftIntent& I){SubmitPrepared(I,Event());}
void UHorizonItemGiftsManager::LookupOriginal(const FString& U,const FString& O){GetOperation(U,O,Event());}
void UHorizonItemGiftsManager::DeclineGift(const FString& U,const FString& I,int64 R){Decline(U,I,R,Event());}
