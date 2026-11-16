// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Examples/HorizonItemGiftsExample.h"
#include "Examples/HorizonItemGiftJournal.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonItemGiftsManager.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Managers/HorizonInboxManager.h"
struct FHorizonItemGiftFlow : TSharedFromThis<FHorizonItemGiftFlow>
{
    TWeakObjectPtr<UHorizonItemGiftsExample> Owner;
    TWeakObjectPtr<UHorizonSubsystem> SDK;
    TSharedPtr<HorizonItemGiftJournal::FLease> Lease;
    FString Project,User;
    uint64 Context=0;
    // Identity-switch protection: a changed player, session token or request context fences every later step.
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
                Self->SDK->ItemGifts->GetState(Self->User,FOnItemGiftComplete::CreateLambda([Self](bool Done,const FHorizonItemGiftResponse&,const FString& Error,const FString&)
                {
                    if(!Done||!Self->Current()){Self->Finish(false,Done?TEXT("STALE_RESPONSE"):Error);return;}const bool Ack=Self->Lease->AcknowledgeAfterRefresh();Self->Finish(Ack,Ack?TEXT(""):TEXT("PENDING_STORAGE_ERROR"));
                }));
            }));
        }));
    }
};
UHorizonSubsystem* UHorizonItemGiftsExample::Session() const{return Bound&&HorizonMarketplaceWire::Id(Project)&&Bound->Auth&&Bound->Auth->IsSignedIn()&&Bound->GetHttpClient()->GetSessionToken()==Bound->Auth->GetCurrentUser().AccessToken&&Bound->ItemGifts&&Bound->Items&&Bound->PlayerValues&&Bound->Inbox?Bound:nullptr;}
bool UHorizonItemGiftsExample::Bind(UHorizonSubsystem* S,const FString& P){if(Busy||!HorizonMarketplaceWire::Id(P))return false;Bound=S;Project=P;Reviewed=false;return Session()!=nullptr;}
void UHorizonItemGiftsExample::ReadConfig(){if(auto* S=Session())S->ItemGifts->ReadConfig(S->Auth->GetCurrentUser().UserId);}
void UHorizonItemGiftsExample::ReadState(){if(auto* S=Session())S->ItemGifts->ReadState(S->Auth->GetCurrentUser().UserId);}
void UHorizonItemGiftsExample::ReadInventory(){if(auto* S=Session())S->Items->RequestInventory(S->Auth->GetCurrentUser().UserId);}
void UHorizonItemGiftsExample::ReadGift(const FString& I){if(auto* S=Session())S->ItemGifts->ReadGift(S->Auth->GetCurrentUser().UserId,I);}
bool UHorizonItemGiftsExample::ReviewSend(const FString& R,const FString& I)
{
    auto* S=Session();if(!S||Busy)return false;const FString U=S->Auth->GetCurrentUser().UserId;const auto C=S->ItemGifts->GetCurrent(TEXT("config"));const auto Inv=S->Items->GetCurrentInventory();
    const auto* Item=Inv.Instances.FindByPredicate([&](const auto& X){return X.InstanceId==I;});
    if(!C.Present||!C.Config.Enabled||C.Config.Fee>0&&C.Config.CurrencyKey.IsEmpty()||!Inv.bPresent||!Item||Item->bEquipped||!HorizonMarketplaceWire::Id(R)||R==U)return false;
    Reviewed=true;ReviewRecipient=R;ReviewInstance=I;ReviewInventory=Inv.Revision;ReviewConfig=C.Config.Revision;ReviewUser=U;ReviewContext=S->GetHttpClient()->GetRequestContextEpoch();
    ReviewedFee=C.Config.Fee;ReviewedCurrencyKey=C.Config.Fee>0?C.Config.CurrencyKey:FString();return true;
}
void UHorizonItemGiftsExample::ConfirmReviewedSend()
{
    auto* S=Session();if(!S||Busy||!Reviewed||ReviewUser!=S->Auth->GetCurrentUser().UserId||ReviewContext!=S->GetHttpClient()->GetRequestContextEpoch())return;const int64 Now=S->ItemGifts->GetServerNow();if(Now<0)return;
    const FString O=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower(),T=(FDateTime::FromUnixTimestamp(Now/1000)+FTimespan::FromMilliseconds(Now%1000)).ToIso8601();
    const auto I=UHorizonItemGiftsManager::PrepareSend(ReviewUser,O,T,ReviewRecipient,ReviewInstance,ReviewInventory,ReviewConfig);
    auto F=MakeShared<FHorizonItemGiftFlow>();F->Lease=MakeShared<HorizonItemGiftJournal::FLease>(Project,ReviewUser);if(!F->Lease->Begin(I)){LastErrorCode=TEXT("PENDING_STORAGE_ERROR");return;}Apply(F,false);
}
void UHorizonItemGiftsExample::RetryPending(){Resume(false);}
void UHorizonItemGiftsExample::RecoverOriginalOperation(){Resume(true);}
void UHorizonItemGiftsExample::Resume(bool Lookup)
{
    auto* S=Session();if(!S||Busy)return;auto F=MakeShared<FHorizonItemGiftFlow>();F->Lease=MakeShared<HorizonItemGiftJournal::FLease>(Project,S->Auth->GetCurrentUser().UserId);if(!F->Lease->Load()){LastErrorCode=TEXT("PENDING_STORAGE_ERROR");return;}Apply(F,Lookup);
}
void UHorizonItemGiftsExample::Apply(const TSharedPtr<FHorizonItemGiftFlow>& F,bool Lookup)
{
    auto* S=Session();if(!S)return;Busy=true;F->Owner=this;F->SDK=S;F->Project=Project;F->User=S->Auth->GetCurrentUser().UserId;F->Context=S->GetHttpClient()->GetRequestContextEpoch();Reviewed=false;
    if(!F->Lease->Original.ConfirmedGift.IsEmpty()){F->Refresh();return;}
    const auto Done=FOnItemGiftComplete::CreateLambda([F](bool Ok,const FHorizonItemGiftResponse& V,const FString& C,const FString&)
    {
        if(!Ok||!F->Current()){F->Finish(false,Ok?TEXT("STALE_RESPONSE"):C);return;}
        if(!F->Lease->Confirm(V.Receipt)){F->Finish(false,TEXT("PENDING_STORAGE_ERROR"));return;}F->Refresh();
    });
    if(Lookup)S->ItemGifts->GetOperation(F->User,F->Lease->Original.Intent.Operation(),Done);else S->ItemGifts->SubmitPrepared(F->Lease->Original.Intent,Done);
}
bool UHorizonItemGiftsExample::DeclineGift(const FString& Id)
{
    auto* S=Session();if(!S)return false;const FString U=S->Auth->GetCurrentUser().UserId;const auto Point=S->ItemGifts->GetCurrent(TEXT("point")),State=S->ItemGifts->GetCurrent(TEXT("state"));
    const FHorizonItemGift* G=Point.Present&&Point.Gift.Id==Id?&Point.Gift:State.Present?State.State.Received.FindByPredicate([&](const auto& X){return X.Id==Id;}):nullptr;
    if(!G||G->Status!=TEXT("offered")||G->RecipientId!=U)return false;S->ItemGifts->DeclineGift(U,Id,G->Revision);return true;
}
void UHorizonItemGiftsExample::ClaimGiftEntry(const FString& I){if(auto* S=Session())S->Inbox->ClaimEntry(S->Auth->GetCurrentUser().UserId,I);}
