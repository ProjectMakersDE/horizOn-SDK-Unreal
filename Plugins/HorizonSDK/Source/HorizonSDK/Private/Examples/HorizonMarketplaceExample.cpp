#include "Examples/HorizonMarketplaceExample.h"
#include "Examples/HorizonMarketplaceJournal.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonMarketplaceManager.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Managers/HorizonInboxManager.h"
struct FHorizonMarketplaceFlow : TSharedFromThis<FHorizonMarketplaceFlow>
{
    TWeakObjectPtr<UHorizonMarketplaceExample> Owner;
    TWeakObjectPtr<UHorizonSubsystem> SDK;
    TSharedPtr<HorizonMarketplaceJournal::FLease> Lease;
    FString Project,User;
    uint64 Context=0;
    bool Current() const{const auto* S=SDK.Get();return S&&S->Auth&&S->Auth->IsSignedIn()&&S->GetHttpClient()->GetRequestContextEpoch()==Context&&S->Auth->GetCurrentUser().UserId==User&&S->GetHttpClient()->GetSessionToken()==S->Auth->GetCurrentUser().AccessToken;}
    void Finish(bool Ok,const FString& Code)
    {
        Lease.Reset();if(auto* O=Owner.Get()){O->Busy=false;O->LastErrorCode=Code;O->OnAction.Broadcast(Ok,Code);}
    }
    void Refresh()
    {
        if(!Current()){Finish(false,TEXT("STALE_RESPONSE"));return;}auto Self=AsShared();
        SDK->Items->GetInventory(User,FOnInventoryComplete::CreateLambda([Self](bool Ok,const FHorizonInventorySnapshot&,const FString& C,const FString&)
        {
            if(!Ok||!Self->Current()){Self->Finish(false,Ok?TEXT("STALE_RESPONSE"):C);return;}
            Self->SDK->PlayerValues->GetState(Self->User,FOnPlayerValuesComplete::CreateLambda([Self](bool Good,const FHorizonPlayerValuesState&,const FString& Code,const FString&)
            {
                if(!Good||!Self->Current()){Self->Finish(false,Good?TEXT("STALE_RESPONSE"):Code);return;}
                Self->SDK->Marketplace->GetMarketplace(Self->User,FOnMarketplaceComplete::CreateLambda([Self](bool Done,const FHorizonMarketplaceResponse&,const FString& Error,const FString&)
                {
                    if(!Done||!Self->Current()){Self->Finish(false,Done?TEXT("STALE_RESPONSE"):Error);return;}const bool Ack=Self->Lease->AcknowledgeAfterRefresh();Self->Finish(Ack,Ack?TEXT(""):TEXT("PENDING_STORAGE_ERROR"));
                }));
            }));
        }));
    }
};
UHorizonSubsystem* UHorizonMarketplaceExample::Session() const{return Bound&&HorizonMarketplaceWire::Id(Project)&&Bound->Auth&&Bound->Auth->IsSignedIn()&&Bound->GetHttpClient()->GetSessionToken()==Bound->Auth->GetCurrentUser().AccessToken&&Bound->Marketplace&&Bound->Items&&Bound->PlayerValues&&Bound->Inbox?Bound:nullptr;}
bool UHorizonMarketplaceExample::Bind(UHorizonSubsystem* S,const FString& P){if(Busy||!HorizonMarketplaceWire::Id(P))return false;Bound=S;Project=P;ReviewKind.Reset();return Session()!=nullptr;}
void UHorizonMarketplaceExample::ReadLimits(){if(auto* S=Session())S->Marketplace->ReadLimits(S->Auth->GetCurrentUser().UserId);}
void UHorizonMarketplaceExample::ReadRulesPage(const FString& A,int64 R){if(auto* S=Session())S->Marketplace->ReadRules(S->Auth->GetCurrentUser().UserId,A,50,R);}
void UHorizonMarketplaceExample::ReadMarketplace(){if(auto* S=Session())S->Marketplace->ReadMarketplace(S->Auth->GetCurrentUser().UserId);}
void UHorizonMarketplaceExample::ReadListings(const FString& K,int64 Min,int64 Max,const FString& C){if(auto* S=Session())S->Marketplace->ReadListings(S->Auth->GetCurrentUser().UserId,K,Min,Max,C,50);}
void UHorizonMarketplaceExample::ReadListing(const FString& I){if(auto* S=Session())S->Marketplace->ReadListing(S->Auth->GetCurrentUser().UserId,I);}
void UHorizonMarketplaceExample::ReadInventory(){if(auto* S=Session())S->Items->RequestInventory(S->Auth->GetCurrentUser().UserId);}
bool UHorizonMarketplaceExample::ReviewCreate(const FString& I,int64 P,int64 D)
{
    auto* S=Session();if(!S||Busy)return false;const auto L=S->Marketplace->GetCurrent(TEXT("limits")),R=S->Marketplace->GetCurrent(TEXT("rules"));const auto Inv=S->Items->GetCurrentInventory();const auto* Item=Inv.Instances.FindByPredicate([&](const auto& X){return X.InstanceId==I;});const auto* Rule=Item?R.Rules.FindByPredicate([&](const auto& X){return X.ItemKey==Item->ItemKey;}):nullptr;
    if(!L.Present||!R.Present||!Inv.bPresent||!L.Config.Enabled||R.CatalogRevision!=L.Config.Revision||!Item||Item->bEquipped||!Rule||!Rule->Active||P<Rule->MinPrice||P>Rule->MaxPrice||D<L.Limits.MinDurationSeconds||D>L.Limits.MaxDurationSeconds)return false;
    ReviewKind=TEXT("create");ReviewListingId.Reset();ReviewInstance=I;ReviewInventory=Inv.Revision;ReviewConfig=L.Config.Revision;ReviewPrice=P;ReviewDuration=D;ReviewUser=S->Auth->GetCurrentUser().UserId;ReviewContext=S->GetHttpClient()->GetRequestContextEpoch();ReviewedPrice=P;ReviewedFee=HorizonMarketplaceContract::Fee(P,L.Config.FeeBps);ReviewedSellerNet=P-ReviewedFee;return true;
}
bool UHorizonMarketplaceExample::ReviewListing(const FString& Id,bool Cancel)
{
    auto* S=Session();if(!S||Busy)return false;const auto V=S->Marketplace->GetCurrent(TEXT("point"));const auto& L=V.Listing;const FString U=S->Auth->GetCurrentUser().UserId;if(!V.Present||L.Id!=Id||L.Status!=TEXT("active")||Cancel!=(L.SellerId==U))return false;
    ReviewKind=Cancel?TEXT("cancel"):TEXT("buy");ReviewListingId=Id;ReviewRevision=L.Revision;ReviewPrice=L.Price;ReviewCurrency=L.CurrencyKey;ReviewUser=U;ReviewContext=S->GetHttpClient()->GetRequestContextEpoch();ReviewedPrice=L.Price;ReviewedFee=L.FeeAmount;ReviewedSellerNet=L.SellerNet;return true;
}
bool UHorizonMarketplaceExample::ReviewBuy(const FString& I){return ReviewListing(I,false);}
bool UHorizonMarketplaceExample::ReviewCancel(const FString& I){return ReviewListing(I,true);}
void UHorizonMarketplaceExample::ConfirmReviewedTrade()
{
    auto* S=Session();if(!S||Busy||ReviewKind.IsEmpty()||ReviewUser!=S->Auth->GetCurrentUser().UserId||ReviewContext!=S->GetHttpClient()->GetRequestContextEpoch())return;const int64 Now=S->Marketplace->GetServerNow();if(Now<0)return;
    const FString O=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower(),T=(FDateTime::FromUnixTimestamp(Now/1000)+FTimespan::FromMilliseconds(Now%1000)).ToIso8601();
    const auto I=ReviewKind==TEXT("create")?UHorizonMarketplaceManager::PrepareCreate(ReviewUser,O,T,ReviewInstance,ReviewInventory,ReviewConfig,ReviewPrice,ReviewDuration):ReviewKind==TEXT("buy")?UHorizonMarketplaceManager::PrepareBuy(ReviewUser,ReviewListingId,O,T,ReviewRevision,ReviewPrice,ReviewCurrency):UHorizonMarketplaceManager::PrepareCancel(ReviewUser,ReviewListingId,O,T,ReviewRevision);
    auto F=MakeShared<FHorizonMarketplaceFlow>();F->Lease=MakeShared<HorizonMarketplaceJournal::FLease>(Project,ReviewUser);if(!F->Lease->Begin(I)){LastErrorCode=TEXT("PENDING_STORAGE_ERROR");return;}Apply(F,false);
}
void UHorizonMarketplaceExample::RetryPending(){Resume(false);}
void UHorizonMarketplaceExample::RecoverOriginalOperation(){Resume(true);}
void UHorizonMarketplaceExample::Resume(bool Lookup)
{
    auto* S=Session();if(!S||Busy)return;auto F=MakeShared<FHorizonMarketplaceFlow>();F->Lease=MakeShared<HorizonMarketplaceJournal::FLease>(Project,S->Auth->GetCurrentUser().UserId);if(!F->Lease->Load()){LastErrorCode=TEXT("PENDING_STORAGE_ERROR");return;}Apply(F,Lookup);
}
void UHorizonMarketplaceExample::Apply(const TSharedPtr<FHorizonMarketplaceFlow>& F,bool Lookup)
{
    auto* S=Session();if(!S)return;Busy=true;F->Owner=this;F->SDK=S;F->Project=Project;F->User=S->Auth->GetCurrentUser().UserId;F->Context=S->GetHttpClient()->GetRequestContextEpoch();ReviewKind.Reset();
    if(!F->Lease->Original.ConfirmedListing.IsEmpty()){F->Refresh();return;}
    const auto Done=FOnMarketplaceComplete::CreateLambda([F](bool Ok,const FHorizonMarketplaceResponse& V,const FString& C,const FString&)
    {
        if(!Ok||!F->Current()){F->Finish(false,Ok?TEXT("STALE_RESPONSE"):C);return;}
        if(!F->Lease->Confirm(V.Receipt)){F->Finish(false,TEXT("PENDING_STORAGE_ERROR"));return;}F->Refresh();
    });
    if(Lookup)S->Marketplace->GetOperation(F->User,F->Lease->Original.Intent.Operation(),Done);else S->Marketplace->SubmitPrepared(F->Lease->Original.Intent,Done);
}
void UHorizonMarketplaceExample::RetryReturnDelivery(const FString& I){if(auto* S=Session())S->Marketplace->RetryReturnDelivery(S->Auth->GetCurrentUser().UserId,I);}
void UHorizonMarketplaceExample::ClaimReturn(const FString& I){if(auto* S=Session())S->Inbox->ClaimEntry(S->Auth->GetCurrentUser().UserId,I);}
