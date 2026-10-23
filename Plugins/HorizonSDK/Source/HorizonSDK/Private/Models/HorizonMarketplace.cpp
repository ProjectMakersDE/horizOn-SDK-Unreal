// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Models/HorizonMarketplace.h"
#include "Models/HorizonMarketplaceWire.h"
namespace HorizonMarketplaceWire
{
bool Limits(const TSharedPtr<FJsonObject>& J,FHorizonMarketplaceLimits& L)
{
    return Small(J,TEXT("openListingsPerPlayer"),L.OpenListingsPerPlayer)&&Small(J,TEXT("retainedListingsPerProject"),L.RetainedListingsPerProject)&&Small(J,TEXT("receiptsPerPlayer"),L.ReceiptsPerPlayer)&&Small(J,TEXT("historyPerPlayer"),L.HistoryPerPlayer)&&Small(J,TEXT("projectWritesPerMinute"),L.ProjectWritesPerMinute)
        &&Number(J,TEXT("playerWriteIntervalMillis"),L.PlayerWriteIntervalMillis,1)&&Number(J,TEXT("minDurationSeconds"),L.MinDurationSeconds,1)&&Number(J,TEXT("maxDurationSeconds"),L.MaxDurationSeconds,L.MinDurationSeconds)&&Number(J,TEXT("terminalRetentionSeconds"),L.TerminalRetentionSeconds,1)&&Number(J,TEXT("operationRetentionSeconds"),L.OperationRetentionSeconds,1)&&Small(J,TEXT("queryCandidates"),L.QueryCandidates)&&Small(J,TEXT("pageSize"),L.PageSize);
}
bool Ref(const TSharedPtr<FJsonObject>& J,FHorizonItemReference& R){return ReadId(J,TEXT("instanceId"),R.InstanceId)&&Text(J,TEXT("itemKey"),R.ItemKey)&&Key(R.ItemKey);}
bool Config(const TSharedPtr<FJsonObject>& J,FHorizonMarketplaceConfig& C)
{
    if(!Number(J,TEXT("revision"),C.Revision)||!Bool(J,TEXT("enabled"),C.Enabled))return false;
    if(!Null(J,TEXT("currencyKey"))&&(!Text(J,TEXT("currencyKey"),C.CurrencyKey)||!Currency(C.CurrencyKey)))return false;
    C.HasFeeBps=!Null(J,TEXT("feeBps"));return (!C.HasFeeBps||Small(J,TEXT("feeBps"),C.FeeBps,0,10000))&&(!C.Enabled||C.HasFeeBps&&!C.CurrencyKey.IsEmpty());
}
bool Listing(const TSharedPtr<FJsonObject>& J,FHorizonMarketplaceListing& L)
{
    if(!ReadId(J,TEXT("id"),L.Id)||!Number(J,TEXT("revision"),L.Revision,1)||!ReadId(J,TEXT("sellerId"),L.SellerId)||!ReadId(J,TEXT("returnOwnerId"),L.ReturnOwnerId)||!Ref(Object(J,TEXT("itemReference")),L.ItemReference)||!Text(J,TEXT("currencyKey"),L.CurrencyKey)||!Currency(L.CurrencyKey)||!Number(J,TEXT("price"),L.Price,1)||!Small(J,TEXT("feeBps"),L.FeeBps,0,10000)||!Number(J,TEXT("feeAmount"),L.FeeAmount)||!Number(J,TEXT("sellerNet"),L.SellerNet)||!Text(J,TEXT("status"),L.Status)||!Time(J,TEXT("createdAt"),L.CreatedAt)||!Time(J,TEXT("expiresAt"),L.ExpiresAt)||!Bool(J,TEXT("ownerUnavailable"),L.OwnerUnavailable))return false;
    if(!Null(J,TEXT("buyerId"))&&!ReadId(J,TEXT("buyerId"),L.BuyerId))return false;
    if(!Null(J,TEXT("terminalAt"))&&!Time(J,TEXT("terminalAt"),L.TerminalAt))return false;
    if(!Null(J,TEXT("returnEntryId"))&&(!Text(J,TEXT("returnEntryId"),L.ReturnEntryId)||!L.ReturnEntryId.StartsWith(TEXT("message:"),ESearchCase::CaseSensitive)||!Id(L.ReturnEntryId.Mid(8))))return false;
    L.HasItem=!Null(J,TEXT("item"));if(L.HasItem&&(!FHorizonItemInstance::TryFromJson(Object(J,TEXT("item")),L.Item)||!Id(L.Item.InstanceId)||L.Item.bEquipped||L.Item.InstanceId!=L.ItemReference.InstanceId||L.Item.ItemKey!=L.ItemReference.ItemKey))return false;
    if(!Choice(L.Status,{TEXT("active"),TEXT("return_pending"),TEXT("return_delivered"),TEXT("sold"),TEXT("returned")}))return false;
    const bool Terminal=L.Status==TEXT("sold")||L.Status==TEXT("returned");
    return (Terminal||L.Status==TEXT("active")||L.Status==TEXT("return_pending")||L.Status==TEXT("return_delivered"))&&Terminal!=L.HasItem&&Terminal!=L.TerminalAt.IsEmpty()&&(L.Status==TEXT("sold"))!=L.BuyerId.IsEmpty()&&L.FeeAmount==HorizonMarketplaceContract::Fee(L.Price,L.FeeBps)&&L.SellerNet==L.Price-L.FeeAmount&&Millis(L.ExpiresAt)>=Millis(L.CreatedAt);
}
bool Receipt(const TSharedPtr<FJsonObject>& J,FHorizonMarketplaceReceipt& R)
{
    return ReadId(J,TEXT("operationId"),R.OperationId)&&Time(J,TEXT("issuedAt"),R.IssuedAt)&&Text(J,TEXT("kind"),R.Kind)&&Choice(R.Kind,{TEXT("create"),TEXT("buy"),TEXT("cancel")})&&ReadId(J,TEXT("listingId"),R.ListingId)&&Number(J,TEXT("listingRevision"),R.ListingRevision,1)&&Time(J,TEXT("recordedAt"),R.RecordedAt)&&Ref(Object(J,TEXT("itemReference")),R.ItemReference)&&Text(J,TEXT("currencyKey"),R.CurrencyKey)&&Currency(R.CurrencyKey)&&Number(J,TEXT("price"),R.Price,1)&&Number(J,TEXT("feeAmount"),R.FeeAmount,0,R.Price)&&Number(J,TEXT("sellerNet"),R.SellerNet)&&R.SellerNet==R.Price-R.FeeAmount;
}
bool Listings(const TSharedPtr<FJsonObject>& J,const TCHAR* K,TArray<FHorizonMarketplaceListing>& Out,int32 Max=MAX_int32)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows;if(!J||!J->TryGetArrayField(K,Rows)||Rows->Num()>Max)return false;TSet<FString> Seen;
    for(auto V:*Rows){FHorizonMarketplaceListing L;if(!V||V->Type!=EJson::Object||!Listing(V->AsObject(),L)||Seen.Contains(L.Id))return false;Seen.Add(L.Id);Out.Add(MoveTemp(L));}return true;
}
bool State(const TSharedPtr<FJsonObject>& J,FHorizonMarketplaceState& S)
{
    auto W=Object(J,TEXT("operationWindow")),M=Object(J,TEXT("moderation"));if(!ReadId(J,TEXT("userId"),S.UserId)||!Number(J,TEXT("revision"),S.Revision)||!Listings(J,TEXT("openListings"),S.OpenListings)||!Listings(J,TEXT("history"),S.History)||!Time(W,TEXT("serverTime"),S.WindowServerTime)||!Time(W,TEXT("acceptAfter"),S.AcceptAfter)||!M||!Text(M,TEXT("reason"),S.ModerationReason)||!Limits(Object(J,TEXT("limits")),S.Limits)||!Null(M,TEXT("banUntil"))&&!Time(M,TEXT("banUntil"),S.BanUntil))return false;
    TSet<FString> Seen;for(const auto& L:S.OpenListings){if(L.Status==TEXT("sold")||L.Status==TEXT("returned")||L.ReturnOwnerId!=S.UserId)return false;Seen.Add(L.Id);}
    for(const auto& L:S.History)if((L.Status!=TEXT("sold")&&L.Status!=TEXT("returned"))||L.SellerId!=S.UserId&&L.BuyerId!=S.UserId&&L.ReturnOwnerId!=S.UserId||Seen.Contains(L.Id))return false;return true;
}
bool NullableCursor(const TSharedPtr<FJsonObject>& J,FString& Out,bool IsKey){return Null(J,TEXT("nextCursor"))||Text(J,TEXT("nextCursor"),Out)&&(IsKey?Key(Out):Cursor(Out));}
}
bool FHorizonMarketplaceResponse::TryFromJson(const FString& Body,const FString& Kind,FHorizonMarketplaceResponse& Out)
{
    using namespace HorizonMarketplaceWire;TSharedPtr<FJsonObject> J;if(!Parse(Body,J))return false;FHorizonMarketplaceResponse V;V.Kind=Kind;if(!Time(J,TEXT("serverTime"),V.ServerTime))return false;
    if(Kind==TEXT("limits")){auto C=Object(J,TEXT("capabilities"));bool Enabled;if(!HorizonMarketplaceWire::Limits(Object(J,TEXT("limits")),V.Limits)||!HorizonMarketplaceWire::Config(Object(J,TEXT("config")),V.Config)||!Bool(C,TEXT("marketplace"),Enabled)||!Enabled||!Bool(C,TEXT("realtime"),V.Realtime))return false;}
    else if(Kind==TEXT("rules"))
    {
        const TArray<TSharedPtr<FJsonValue>>* Rows;if(!Number(J,TEXT("revision"),V.CatalogRevision)||!J->TryGetArrayField(TEXT("rules"),Rows)||Rows->Num()>50||!NullableCursor(J,V.NextCursor,true))return false;FString Last;
        for(auto R:*Rows){FHorizonMarketplaceRule Rule;if(!R||R->Type!=EJson::Object)return false;auto O=R->AsObject();if(!Text(O,TEXT("itemKey"),Rule.ItemKey)||!Key(Rule.ItemKey)||!Bool(O,TEXT("active"),Rule.Active)||!Number(O,TEXT("minPrice"),Rule.MinPrice,1)||!Number(O,TEXT("maxPrice"),Rule.MaxPrice,Rule.MinPrice)||!Last.IsEmpty()&&Last.Compare(Rule.ItemKey,ESearchCase::CaseSensitive)>=0)return false;Last=Rule.ItemKey;V.Rules.Add(Rule);}if(!V.NextCursor.IsEmpty()&&(V.Rules.IsEmpty()||V.NextCursor!=Last))return false;
    }
    else if(Kind==TEXT("state")){if(!HorizonMarketplaceWire::State(Object(J,TEXT("state")),V.State))return false;}
    else if(Kind==TEXT("query")){if(!HorizonMarketplaceWire::Listings(J,TEXT("listings"),V.Listings,50)||!NullableCursor(J,V.NextCursor,false))return false;}
    else if(Kind==TEXT("point")||Kind==TEXT("return")){if(!HorizonMarketplaceWire::Listing(Object(J,TEXT("listing")),V.Listing))return false;V.HasListing=true;if(Kind==TEXT("return")&&(!Text(J,TEXT("delivery"),V.Delivery)||!Choice(V.Delivery,{TEXT("pending"),TEXT("delivered"),TEXT("returned")})))return false;}
    else if(Kind==TEXT("operation")){if(!HorizonMarketplaceWire::Receipt(Object(J,TEXT("receipt")),V.Receipt))return false;}
    else if(Kind==TEXT("written"))
    {
        if(!HorizonMarketplaceWire::Receipt(Object(J,TEXT("receipt")),V.Receipt)||!Bool(J,TEXT("replayed"),V.Replayed))return false;V.HasListing=!Null(J,TEXT("listing"));if(V.HasListing&&!HorizonMarketplaceWire::Listing(Object(J,TEXT("listing")),V.Listing))return false;
        const auto& L=V.Listing;const auto& R=V.Receipt;
        if(V.HasListing&&(L.Id!=R.ListingId||L.Revision<R.ListingRevision||L.ItemReference.InstanceId!=R.ItemReference.InstanceId||L.ItemReference.ItemKey!=R.ItemReference.ItemKey||L.CurrencyKey!=R.CurrencyKey||L.Price!=R.Price||L.FeeAmount!=R.FeeAmount||L.SellerNet!=R.SellerNet))return false;
        if(!V.Replayed&&(!V.HasListing||L.Revision!=R.ListingRevision||L.Status!=(R.Kind==TEXT("create")?TEXT("active"):R.Kind==TEXT("buy")?TEXT("sold"):TEXT("return_pending"))))return false;
    }
    else return false;V.Present=true;Out=MoveTemp(V);return true;
}
bool FHorizonMarketplaceIntent::IsValid() const
{
    using namespace HorizonMarketplaceWire;TSharedPtr<FJsonObject> J;FString U,O,T,I,C;int64 A,B,P,D;
    if(!Choice(Kind,{TEXT("create"),TEXT("buy"),TEXT("cancel")})||!Parse(Json,J)||!ReadId(J,TEXT("userId"),U)||!ReadId(J,TEXT("operationId"),O)||!Time(J,TEXT("issuedAt"),T))return false;
    if(Kind==TEXT("create"))return ListingId.IsEmpty()&&J->Values.Num()==8&&ReadId(J,TEXT("instanceId"),I)&&Number(J,TEXT("expectedInventoryRevision"),A)&&Number(J,TEXT("expectedConfigRevision"),B)&&Number(J,TEXT("price"),P,1)&&Number(J,TEXT("durationSeconds"),D,300,604800);
    if(!Id(ListingId)||!Number(J,TEXT("expectedListingRevision"),A))return false;
    return Kind==TEXT("cancel")?J->Values.Num()==4:Kind==TEXT("buy")&&J->Values.Num()==6&&Number(J,TEXT("expectedPrice"),P,1)&&Text(J,TEXT("expectedCurrencyKey"),C)&&Currency(C);
}
FString FHorizonMarketplaceIntent::User() const{return HorizonMarketplaceWire::Field(*this,TEXT("userId"));}
FString FHorizonMarketplaceIntent::Operation() const{return HorizonMarketplaceWire::Field(*this,TEXT("operationId"));}
bool FHorizonMarketplaceIntent::Matches(const FHorizonMarketplaceReceipt& R) const
{
    using namespace HorizonMarketplaceWire;TSharedPtr<FJsonObject> J;if(!IsValid()||!Parse(Json,J)||R.OperationId!=Operation()||R.Kind!=Kind||R.IssuedAt!=Field(*this,TEXT("issuedAt"))||!Id(R.ListingId))return false;int64 P,V;
    if(Kind==TEXT("create"))return R.ListingRevision==1&&R.ItemReference.InstanceId==Field(*this,TEXT("instanceId"))&&Number(J,TEXT("price"),P)&&P==R.Price;
    if(!Number(J,TEXT("expectedListingRevision"),V)||R.ListingId!=ListingId||R.ListingRevision!=V+1)return false;
    return Kind!=TEXT("buy")||Number(J,TEXT("expectedPrice"),P)&&R.Price==P&&R.CurrencyKey==Field(*this,TEXT("expectedCurrencyKey"));
}
