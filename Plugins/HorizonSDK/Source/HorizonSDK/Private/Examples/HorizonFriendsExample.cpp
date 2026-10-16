#include "Examples/HorizonFriendsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonFriendsManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
namespace HorizonFriendExampleStorage
{
FString Project(UHorizonSubsystem* SDK) { return FMD5::HashAnsiString(*SDK->GetHttpClient()->GetApiKey()); }
FString Slot(const FString& Project,const FString& User) { return TEXT("horizon-friend-action-")+FMD5::HashAnsiString(*(Project+TEXT("|")+User)); }
bool Valid(const UHorizonFriendPendingAction* P,const FString& Project,const FString& User)
{
    FGuid Target,Request,Intent; const bool Needs=P && (P->Action==TEXT("accept") || P->Action==TEXT("decline") || P->Action==TEXT("cancel"));
    return P && P->ProjectScope==Project && P->UserId==User && FGuid::ParseExact(P->IntentId,EGuidFormats::DigitsWithHyphens,Intent) && P->SendSerial>=0 && P->SendSerial<=9007199254740991LL && P->ExpectedRevision>=0 && P->ExpectedRevision<=9007199254740991LL &&
        FGuid::ParseExact(P->TargetUserId,EGuidFormats::DigitsWithHyphens,Target) && !P->TargetUserId.Equals(User,ESearchCase::IgnoreCase) &&
        (Needs?FGuid::ParseExact(P->RequestId,EGuidFormats::DigitsWithHyphens,Request):P->RequestId.IsEmpty()) &&
        (Needs || P->Action==TEXT("request") || P->Action==TEXT("remove") || P->Action==TEXT("block") || P->Action==TEXT("unblock"));
}

}
UHorizonSubsystem* AHorizonFriendsExample::Session() const
{
    auto* Instance=GetGameInstance(); auto* SDK=Instance?Instance->GetSubsystem<UHorizonSubsystem>():nullptr;
    return SDK && SDK->Auth && SDK->Auth->IsSignedIn()?SDK:nullptr;
}
FHorizonFriendsSnapshot AHorizonFriendsExample::GetCurrentFriends() const
{ auto* SDK=Session(); return SDK?SDK->Friends->GetCurrentFriends():FHorizonFriendsSnapshot{}; }
FHorizonFriendLookup AHorizonFriendsExample::GetLastLookup() const
{ auto* SDK=Session(); return SDK && SDK->GetHttpClient()->GetRequestContextEpoch()==LookupContext?LastLookup:FHorizonFriendLookup{}; }
void AHorizonFriendsExample::LoadFriends()
{
    auto* SDK=Session(); if (!SDK || bSending) return; bRefreshed=false; RefreshedIntentId.Reset(); RefreshedSendSerial=-1;
    const FString User=SDK->Auth->GetCurrentUser().UserId,Project=HorizonFriendExampleStorage::Project(SDK),Slot=HorizonFriendExampleStorage::Slot(Project,User);
    auto* Pending=Cast<UHorizonFriendPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    const FString IntentId=HorizonFriendExampleStorage::Valid(Pending,Project,User)?Pending->IntentId:FString();
    const int64 SendSerial=IntentId.IsEmpty()?-1:Pending->SendSerial;
    SDK->Friends->GetFriends(User,FOnFriendsComplete::CreateWeakLambda(this,[this,Project,User,Slot,IntentId,SendSerial](bool Ok,const FHorizonFriendsSnapshot&,const FString& Code,const FString&)
    {
        auto* Current=Session();
        auto* Stored=Cast<UHorizonFriendPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if (Ok && Current && !bSending && !IntentId.IsEmpty() && HorizonFriendExampleStorage::Valid(Stored,Project,User) && Stored->IntentId==IntentId && Stored->SendSerial==SendSerial)
        { bRefreshed=true; RefreshedContext=Current->GetHttpClient()->GetRequestContextEpoch(); RefreshedIntentId=IntentId; RefreshedSendSerial=SendSerial; }
        else if (!Ok) UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Refresh failed: %s"),*Code);
    }));
}
void AHorizonFriendsExample::FindPlayer(const FString& Query)
{
    auto* SDK=Session(); if (!SDK) return;
    LastLookup={}; LookupContext=0;
    SDK->Friends->FindPlayer(SDK->Auth->GetCurrentUser().UserId,Query,FOnFriendLookupComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendLookup& R,const FString& Code,const FString&)
    {
        auto* Current=Session();
        if (Ok && Current) { LastLookup=R; LookupContext=Current->GetHttpClient()->GetRequestContextEpoch(); }
        else UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Exact lookup failed: %s"),*Code);
    }));
}
void AHorizonFriendsExample::StartAction(const FString& Action,const FString& Target,int64 Expected,const FString& RequestId)
{
    auto* SDK=Session(); if (!SDK || bSending) return;
    const FString User=SDK->Auth->GetCurrentUser().UserId,Project=HorizonFriendExampleStorage::Project(SDK),Slot=HorizonFriendExampleStorage::Slot(Project,User);
    if (UGameplayStatics::DoesSaveGameExist(Slot,0))
    { UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Preserve/reconcile the original pending action first")); return; }
    const auto S=SDK->Friends->GetCurrentFriends();
    if (!S.bPresent || S.Revision!=Expected) return;
    auto* P=Cast<UHorizonFriendPendingAction>(UGameplayStatics::CreateSaveGameObject(UHorizonFriendPendingAction::StaticClass()));
    P->ProjectScope=Project; P->UserId=User; P->Action=Action; P->TargetUserId=Target; P->ExpectedRevision=Expected; P->RequestId=RequestId; P->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    if (!HorizonFriendExampleStorage::Valid(P,Project,User)) return;
    const auto Peer=[&](const auto& Row) { return Row.UserId.Equals(Target,ESearchCase::IgnoreCase); };
    const auto Request=[&](const FHorizonFriendRequest& Row) { return Peer(Row) && Row.RequestId==RequestId; };
    if ((Action==TEXT("accept") || Action==TEXT("decline")) && !S.Incoming.ContainsByPredicate(Request)) return;
    if (Action==TEXT("cancel") && !S.Outgoing.ContainsByPredicate(Request)) return;
    if (Action==TEXT("remove") && !S.Friends.ContainsByPredicate(Peer)) return;
    if (Action==TEXT("unblock") && !S.Blocked.ContainsByPredicate(Peer)) return;
    if ((Action==TEXT("request") || Action==TEXT("block")) && !S.Friends.ContainsByPredicate(Peer) && !S.Incoming.ContainsByPredicate(Peer) && !S.Outgoing.ContainsByPredicate(Peer) && !S.Blocked.ContainsByPredicate(Peer) && !GetLastLookup().UserId.Equals(Target,ESearchCase::IgnoreCase)) return;
    if (!UGameplayStatics::SaveGameToSlot(P,Slot,0)) { UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Original action could not be persisted; nothing sent")); return; }
    Send(P,Slot);
}
void AHorizonFriendsExample::RetrySavedAction()
{
    auto* SDK=Session(); if (!SDK || bSending) return;
    const FString User=SDK->Auth->GetCurrentUser().UserId,Project=HorizonFriendExampleStorage::Project(SDK),Slot=HorizonFriendExampleStorage::Slot(Project,User);
    auto* P=Cast<UHorizonFriendPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (HorizonFriendExampleStorage::Valid(P,Project,User)) Send(P,Slot);
}
bool AHorizonFriendsExample::AcknowledgePendingAfterRefresh()
{
    auto* SDK=Session(); if (!SDK || bSending || !bRefreshed || RefreshedContext!=SDK->GetHttpClient()->GetRequestContextEpoch() || !SDK->Friends->GetCurrentFriends().bPresent) return false;
    const FString User=SDK->Auth->GetCurrentUser().UserId,Project=HorizonFriendExampleStorage::Project(SDK),Slot=HorizonFriendExampleStorage::Slot(Project,User);
    auto* P=Cast<UHorizonFriendPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (!HorizonFriendExampleStorage::Valid(P,Project,User) || P->IntentId!=RefreshedIntentId || P->SendSerial!=RefreshedSendSerial) return false;
    // User acknowledged the refreshed relationship. This is NOT cancellation or an automatic new action.
    bRefreshed=false; return UGameplayStatics::DeleteGameInSlot(Slot,0);
}
void AHorizonFriendsExample::Send(UHorizonFriendPendingAction* P,const FString& Slot)
{
    auto* SDK=Session(); if (!SDK || bSending) return;
    const FString CurrentUser=SDK->Auth->GetCurrentUser().UserId,CurrentProject=HorizonFriendExampleStorage::Project(SDK);
    if (!HorizonFriendExampleStorage::Valid(P,CurrentProject,CurrentUser) || P->SendSerial>=9007199254740991LL) return;
    // Persist an attempt fence before HTTP, shared by every actor and retained across restarts.
    ++P->SendSerial;
    if (!UGameplayStatics::SaveGameToSlot(P,Slot,0)) { UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Send fence could not be persisted; nothing sent")); return; }
    bSending=true; bRefreshed=false;
    // Capture the original tuple by value; another actor/save edit cannot change the receipt identity.
    const FString Project=P->ProjectScope,User=P->UserId,Action=P->Action,Target=P->TargetUserId,RequestId=P->RequestId,IntentId=P->IntentId; const int64 Expected=P->ExpectedRevision,SendSerial=P->SendSerial;
    const auto Callback=FOnFriendsComplete::CreateWeakLambda(this,[this,Slot,Project,User,Action,Target,RequestId,Expected,IntentId,SendSerial](bool Ok,const FHorizonFriendsSnapshot&,const FString& Code,const FString&)
    {
        bSending=false;
        if (!Ok) { UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Keep original action; refresh and explicitly reconcile: %s"),*Code); return; }
        auto* Stored=Cast<UHorizonFriendPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if (HorizonFriendExampleStorage::Valid(Stored,Project,User) && Stored->IntentId==IntentId && Stored->SendSerial==SendSerial && Stored->Action==Action && Stored->TargetUserId==Target && Stored->RequestId==RequestId && Stored->ExpectedRevision==Expected)
            if (!UGameplayStatics::DeleteGameInSlot(Slot,0)) UE_LOG(LogTemp,Warning,TEXT("[FriendsExample] Original action retained for reconciliation"));
    });
    if (Action==TEXT("request")) SDK->Friends->SendRequest(User,Target,Expected,Callback);
    else if (Action==TEXT("accept")) SDK->Friends->AcceptRequest(User,Target,Expected,RequestId,Callback);
    else if (Action==TEXT("decline")) SDK->Friends->DeclineRequest(User,Target,Expected,RequestId,Callback);
    else if (Action==TEXT("cancel")) SDK->Friends->CancelRequest(User,Target,Expected,RequestId,Callback);
    else if (Action==TEXT("remove")) SDK->Friends->RemoveFriend(User,Target,Expected,Callback);
    else if (Action==TEXT("block")) SDK->Friends->BlockPlayer(User,Target,Expected,Callback);
    else if (Action==TEXT("unblock")) SDK->Friends->UnblockPlayer(User,Target,Expected,Callback);
}
