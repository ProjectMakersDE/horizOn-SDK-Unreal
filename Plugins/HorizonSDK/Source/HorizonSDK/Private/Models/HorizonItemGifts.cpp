// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Models/HorizonItemGifts.h"
#include "Models/HorizonMarketplaceWire.h"
// Item gifts share the Marketplace strict wire helpers: canonical lowercase UUIDs, exact UTC milliseconds,
// safe integer tokens and duplicate-key detection before engine DOM normalization.
namespace HorizonItemGiftsWire
{
using namespace HorizonMarketplaceWire;
bool Limits(const TSharedPtr<FJsonObject>& J,FHorizonItemGiftLimits& L)
{
    return Small(J,TEXT("openGiftsPerPlayer"),L.OpenGiftsPerPlayer,0)&&Small(J,TEXT("retainedGiftsPerProject"),L.RetainedGiftsPerProject,0)&&Small(J,TEXT("projectWritesPerMinute"),L.ProjectWritesPerMinute,0)
        &&Small(J,TEXT("historyPerPlayer"),L.HistoryPerPlayer,0)&&Small(J,TEXT("receivedPageSize"),L.ReceivedPageSize,0)&&Number(J,TEXT("playerWriteIntervalMillis"),L.PlayerWriteIntervalMillis)
        &&Number(J,TEXT("expirySeconds"),L.ExpirySeconds,1)&&Number(J,TEXT("terminalRetentionSeconds"),L.TerminalRetentionSeconds)&&Number(J,TEXT("operationRetentionSeconds"),L.OperationRetentionSeconds,1);
}
bool Ref(const TSharedPtr<FJsonObject>& J,FHorizonItemReference& R){return J&&J->Values.Num()==2&&ReadId(J,TEXT("instanceId"),R.InstanceId)&&Text(J,TEXT("itemKey"),R.ItemKey)&&Key(R.ItemKey);}
bool OptionalCurrency(const TSharedPtr<FJsonObject>& J,FString& Out){return Null(J,TEXT("currencyKey"))||Text(J,TEXT("currencyKey"),Out)&&Currency(Out);}
bool OptionalTime(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& Out){return Null(J,K)||Time(J,K,Out);}
bool OptionalEntry(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& Out){return Null(J,K)||Text(J,K,Out)&&Out.StartsWith(TEXT("message:"),ESearchCase::CaseSensitive)&&Id(Out.Mid(8));}
bool Config(const TSharedPtr<FJsonObject>& J,FHorizonItemGiftConfig& C)
{
    // A fee needs a currency. A free gift never charges, whatever currency the header keeps.
    return Number(J,TEXT("revision"),C.Revision)&&Bool(J,TEXT("enabled"),C.Enabled)&&OptionalCurrency(J,C.CurrencyKey)&&Number(J,TEXT("fee"),C.Fee)&&(C.Fee==0||!C.CurrencyKey.IsEmpty());
}
bool Gift(const TSharedPtr<FJsonObject>& J,FHorizonItemGift& G)
{
    if(!ReadId(J,TEXT("id"),G.Id)||!Number(J,TEXT("revision"),G.Revision,1)||!ReadId(J,TEXT("senderId"),G.SenderId)||!ReadId(J,TEXT("recipientId"),G.RecipientId)||!ReadId(J,TEXT("returnOwnerId"),G.ReturnOwnerId)
        ||!Ref(Object(J,TEXT("itemReference")),G.ItemReference)||!OptionalCurrency(J,G.CurrencyKey)||!Number(J,TEXT("fee"),G.Fee)||!Number(J,TEXT("configRevision"),G.ConfigRevision)
        ||!Text(J,TEXT("status"),G.Status)||!Time(J,TEXT("createdAt"),G.CreatedAt)||!Time(J,TEXT("expiresAt"),G.ExpiresAt)||!OptionalTime(J,TEXT("terminalAt"),G.TerminalAt)||!OptionalTime(J,TEXT("declinedAt"),G.DeclinedAt)
        ||!OptionalEntry(J,TEXT("offerEntryId"),G.OfferEntryId)||!OptionalEntry(J,TEXT("returnEntryId"),G.ReturnEntryId)||!Bool(J,TEXT("ownerUnavailable"),G.OwnerUnavailable))return false;
    if(!Choice(G.Status,{TEXT("offered"),TEXT("claimed"),TEXT("return_pending"),TEXT("return_delivered"),TEXT("returned")}))return false;
    return G.SenderId!=G.RecipientId&&G.IsTerminal()!=G.TerminalAt.IsEmpty()&&(G.Fee==0)==G.CurrencyKey.IsEmpty()&&Millis(G.ExpiresAt)>=Millis(G.CreatedAt);
}
bool Receipt(const TSharedPtr<FJsonObject>& J,FHorizonItemGiftReceipt& R)
{
    return ReadId(J,TEXT("operationId"),R.OperationId)&&Time(J,TEXT("issuedAt"),R.IssuedAt)&&ReadId(J,TEXT("giftId"),R.GiftId)&&Number(J,TEXT("giftRevision"),R.GiftRevision,1)&&Time(J,TEXT("recordedAt"),R.RecordedAt)
        &&ReadId(J,TEXT("recipientId"),R.RecipientId)&&Ref(Object(J,TEXT("itemReference")),R.ItemReference)&&OptionalCurrency(J,R.CurrencyKey)&&Number(J,TEXT("fee"),R.Fee)&&Number(J,TEXT("configRevision"),R.ConfigRevision)&&(R.Fee==0)==R.CurrencyKey.IsEmpty();
}
bool Gifts(const TSharedPtr<FJsonObject>& J,const TCHAR* K,TArray<FHorizonItemGift>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows;if(!J||!J->TryGetArrayField(K,Rows))return false;TSet<FString> Seen;
    for(auto V:*Rows){FHorizonItemGift G;if(!V||V->Type!=EJson::Object||!Gift(V->AsObject(),G)||Seen.Contains(G.Id))return false;Seen.Add(G.Id);Out.Add(MoveTemp(G));}return true;
}
bool State(const TSharedPtr<FJsonObject>& J,FHorizonItemGiftState& S)
{
    auto W=Object(J,TEXT("operationWindow"));
    if(!ReadId(J,TEXT("userId"),S.UserId)||!Number(J,TEXT("revision"),S.Revision)||!Gifts(J,TEXT("sent"),S.Sent)||!Gifts(J,TEXT("history"),S.History)||!Gifts(J,TEXT("received"),S.Received)
        ||!Time(W,TEXT("serverTime"),S.WindowServerTime)||!Time(W,TEXT("acceptAfter"),S.AcceptAfter)||!Limits(Object(J,TEXT("limits")),S.Limits))return false;
    for(const auto& G:S.Sent)if(G.IsTerminal()||G.ReturnOwnerId!=S.UserId)return false;
    for(const auto& G:S.History)if(!G.IsTerminal()||G.SenderId!=S.UserId&&G.RecipientId!=S.UserId&&G.ReturnOwnerId!=S.UserId)return false;
    for(const auto& G:S.Received)if(G.Status!=TEXT("offered")||G.RecipientId!=S.UserId)return false;
    return true;
}
FString Field(const FString& Json,const TCHAR* Key){TSharedPtr<FJsonObject> J;FString S;return Parse(Json,J)&&Text(J,Key,S)?S:FString();}
bool SameReference(const FHorizonItemGift& G,const FHorizonItemGiftReceipt& R)
{
    return G.Id==R.GiftId&&G.Revision>=R.GiftRevision&&G.RecipientId==R.RecipientId&&G.ItemReference.InstanceId==R.ItemReference.InstanceId&&G.ItemReference.ItemKey==R.ItemReference.ItemKey&&G.CurrencyKey==R.CurrencyKey&&G.Fee==R.Fee&&G.ConfigRevision==R.ConfigRevision;
}
}
bool FHorizonItemGiftResponse::TryFromJson(const FString& Body,const FString& Kind,FHorizonItemGiftResponse& Out)
{
    using namespace HorizonItemGiftsWire;TSharedPtr<FJsonObject> J;if(!Parse(Body,J))return false;FHorizonItemGiftResponse V;V.Kind=Kind;if(!Time(J,TEXT("serverTime"),V.ServerTime))return false;
    if(Kind==TEXT("config")){if(!HorizonItemGiftsWire::Config(Object(J,TEXT("config")),V.Config)||!HorizonItemGiftsWire::Limits(Object(J,TEXT("limits")),V.Limits))return false;}
    else if(Kind==TEXT("state")){if(!HorizonItemGiftsWire::State(Object(J,TEXT("state")),V.State))return false;}
    else if(Kind==TEXT("operation")){if(!HorizonItemGiftsWire::Receipt(Object(J,TEXT("receipt")),V.Receipt))return false;}
    else if(Kind==TEXT("point")){if(!HorizonItemGiftsWire::Gift(Object(J,TEXT("gift")),V.Gift))return false;V.HasGift=true;}
    else if(Kind==TEXT("declined"))
    {
        if(!HorizonItemGiftsWire::Gift(Object(J,TEXT("gift")),V.Gift)||!Bool(J,TEXT("replayed"),V.Replayed)||!Text(J,TEXT("returnDelivery"),V.ReturnDelivery)||!Choice(V.ReturnDelivery,{TEXT("pending"),TEXT("delivered"),TEXT("returned"),TEXT("claimed")}))return false;
        V.HasGift=true;if(V.Gift.DeclinedAt.IsEmpty()||V.Gift.Status==TEXT("offered")||V.Gift.Status==TEXT("claimed"))return false;
    }
    else if(Kind==TEXT("written"))
    {
        // A replay may carry a later gift state, or none once the terminal gift left its retention window.
        if(!HorizonItemGiftsWire::Receipt(Object(J,TEXT("receipt")),V.Receipt)||!Bool(J,TEXT("replayed"),V.Replayed))return false;V.HasGift=!Null(J,TEXT("gift"));
        if(V.HasGift&&(!HorizonItemGiftsWire::Gift(Object(J,TEXT("gift")),V.Gift)||!SameReference(V.Gift,V.Receipt)))return false;
        if(!V.Replayed&&(!V.HasGift||V.Gift.Revision!=V.Receipt.GiftRevision||V.Gift.Status!=TEXT("offered")))return false;
    }
    else return false;V.Present=true;Out=MoveTemp(V);return true;
}
bool FHorizonItemGiftIntent::IsValid() const
{
    using namespace HorizonItemGiftsWire;TSharedPtr<FJsonObject> J;FString U,O,T,R,I;int64 A,B;
    return Parse(Json,J)&&J->Values.Num()==7&&ReadId(J,TEXT("userId"),U)&&ReadId(J,TEXT("operationId"),O)&&Time(J,TEXT("issuedAt"),T)&&ReadId(J,TEXT("recipientId"),R)&&ReadId(J,TEXT("instanceId"),I)
        &&Number(J,TEXT("expectedInventoryRevision"),A)&&Number(J,TEXT("expectedConfigRevision"),B)&&U!=R;
}
FString FHorizonItemGiftIntent::User() const{return HorizonItemGiftsWire::Field(Json,TEXT("userId"));}
FString FHorizonItemGiftIntent::Operation() const{return HorizonItemGiftsWire::Field(Json,TEXT("operationId"));}
bool FHorizonItemGiftIntent::Matches(const FHorizonItemGiftReceipt& R) const
{
    using namespace HorizonItemGiftsWire;TSharedPtr<FJsonObject> J;int64 Config;
    return IsValid()&&Parse(Json,J)&&Number(J,TEXT("expectedConfigRevision"),Config)&&R.OperationId==Operation()&&R.IssuedAt==Field(Json,TEXT("issuedAt"))&&R.RecipientId==Field(Json,TEXT("recipientId"))
        &&R.ItemReference.InstanceId==Field(Json,TEXT("instanceId"))&&R.ConfigRevision==Config&&Id(R.GiftId);
}
