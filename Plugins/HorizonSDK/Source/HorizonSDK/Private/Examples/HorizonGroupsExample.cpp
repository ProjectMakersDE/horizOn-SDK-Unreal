// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Examples/HorizonGroupsExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonGroupsManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
namespace HorizonGroupExampleStorage
{
FString Project(UHorizonSubsystem* SDK) {return UHorizonExampleIdentity::Project(SDK->GetHttpClient()->GetApiKey());}
FString Slot(const FString& P,const FString& U) {return TEXT("horizon-group-original-")+UHorizonExampleIdentity::Project(P+TEXT("|")+U);}
bool Uuid(const FString& S) {FGuid G;return FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Valid(const UHorizonGroupPendingAction* P,const FString& Project,const FString& User)
{
    if(!P||P->Project!=Project||P->UserId!=User||!Uuid(P->IntentId)||P->ExpectedRevision<0||P->ExpectedRevision>9007199254740991LL||P->SendSerial<0||P->SendSerial>9007199254740991LL)return false;
    const FString A=P->Action;const bool Create=A==TEXT("create"),Meta=Create||A==TEXT("update");
    const bool Target=A==TEXT("invite")||A==TEXT("cancel-invite")||A==TEXT("accept-request")||A==TEXT("decline-request")||A==TEXT("remove")||A==TEXT("role")||A==TEXT("transfer");
    const bool Request=A==TEXT("accept-invite")||A==TEXT("decline-invite")||A==TEXT("cancel-invite")||A==TEXT("accept-request")||A==TEXT("decline-request")||A==TEXT("cancel-request");
    if((Create?!P->GroupId.IsEmpty():!Uuid(P->GroupId))||(Target?(!Uuid(P->TargetUserId)||P->TargetUserId==User):!P->TargetUserId.IsEmpty())||(Request?!Uuid(P->RequestId):!P->RequestId.IsEmpty())||(A==TEXT("role")?(P->Role!=TEXT("officer")&&P->Role!=TEXT("member")):!P->Role.IsEmpty()))return false;
    if(Meta){FHorizonGroupJsonData D;return !P->Metadata.Name.TrimStartAndEnd().IsEmpty()&&(P->Metadata.JoinMode==TEXT("open")||P->Metadata.JoinMode==TEXT("request")||P->Metadata.JoinMode==TEXT("invite"))&&FHorizonGroupJsonData::TryFromJson(P->Metadata.Data.GetJson(),D);}
    return Target||Request||A==TEXT("join")||A==TEXT("request")||A==TEXT("leave")||A==TEXT("delete");
}
FString Signature(const UHorizonGroupPendingAction* P)
{
    if(!P)return {};FString Value;
    for(const FString& S:{P->Project,P->UserId,P->IntentId,P->Action,P->GroupId,P->TargetUserId,P->RequestId,P->Role,LexToString(P->ExpectedRevision),LexToString(P->SendSerial),P->Metadata.Name,P->Metadata.Description,P->Metadata.JoinMode,P->Metadata.Data.GetJson()})Value+=LexToString(S.Len())+TEXT(":")+S;
    FTCHARToUTF8 Bytes(*Value,Value.Len());return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length()).c_str());
}
bool Same(const UHorizonGroupPendingAction* A,const UHorizonGroupPendingAction* B)
{return A&&B&&Signature(A)==Signature(B);}
}
UHorizonSubsystem* AHorizonGroupsExample::Session() const {auto* GI=GetGameInstance();auto* SDK=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;return SDK&&SDK->Auth&&SDK->Auth->IsSignedIn()?SDK:nullptr;}
FHorizonMyGroupsView AHorizonGroupsExample::GetGroups() const {auto* SDK=Session();return SDK?SDK->Groups->GetCurrentGroups():FHorizonMyGroupsView{};}
FHorizonGroupView AHorizonGroupsExample::GetGroup() const
{
    auto* SDK=Session();if(!SDK||SnapshotContext!=SDK->GetHttpClient()->GetRequestContextEpoch()||SnapshotGeneration!=SDK->Groups->GetCacheGeneration())return {};
    const auto Mine=SDK->Groups->GetCurrentGroups();if(!Mine.bPresent)return {};
    if(CurrentGroup.bHasData){const auto* Member=Mine.Groups.FindByPredicate([this](const FHorizonGroupView& G){return G.Id==CurrentGroup.Id;});if(!Member||Member->Revision!=CurrentGroup.Revision||Member->MyRole!=CurrentGroup.MyRole)return {};}
    return CurrentGroup;
}
void AHorizonGroupsExample::Refresh(const FString& Group)
{
    auto* SDK=Session();if(!SDK||bSending||bRefreshing||(!Group.IsEmpty()&&!HorizonGroupExampleStorage::Uuid(Group)))return;
    bRefreshed=false;bRefreshing=true;CurrentGroup={};PendingSnapshot={};RefreshedIntent.Reset();RefreshedSerial=-1;
    const FString User=SDK->Auth->GetCurrentUser().UserId,Project=HorizonGroupExampleStorage::Project(SDK),Slot=HorizonGroupExampleStorage::Slot(Project,User);const uint64 C=SDK->GetHttpClient()->GetRequestContextEpoch();
    auto* P=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));const bool Valid=HorizonGroupExampleStorage::Valid(P,Project,User);const FString Intent=Valid?HorizonGroupExampleStorage::Signature(P):FString();const int64 Serial=Valid?P->SendSerial:-1;
    if(Group.IsEmpty()){ReadOwn(Project,User,Slot,Intent,Serial,C);return;}
    SDK->Groups->GetGroup(User,Group,FOnGroupComplete::CreateWeakLambda(this,[this,Project,User,Slot,Intent,Serial,C](bool Ok,const FHorizonGroupView& G,const FString& Code,const FString&)
    {
        auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C){bRefreshing=false;UE_LOG(LogTemp,Warning,TEXT("[GroupsExample] Explicit refresh failed: %s"),*Code);return;}CurrentGroup=G;
        if(G.MyRole==TEXT("leader")||G.MyRole==TEXT("officer"))Current->Groups->GetPending(User,G.Id,FOnGroupPendingComplete::CreateWeakLambda(this,[this,Project,User,Slot,Intent,Serial,C](bool Good,const FHorizonGroupPendingSnapshot& S,const FString&,const FString&){if(!Good){bRefreshing=false;return;}PendingSnapshot=S;ReadOwn(Project,User,Slot,Intent,Serial,C);}));
        else ReadOwn(Project,User,Slot,Intent,Serial,C);
    }));
}
void AHorizonGroupsExample::ReadOwn(const FString& Project,const FString& User,const FString& Slot,const FString& Intent,int64 Serial,uint64 C)
{
    auto* SDK=Session();if(!SDK||SDK->GetHttpClient()->GetRequestContextEpoch()!=C){bRefreshing=false;return;}
    SDK->Groups->GetGroups(User,FOnGroupsComplete::CreateWeakLambda(this,[this,Project,User,Slot,Intent,Serial,C](bool Ok,const FHorizonMyGroupsView& S,const FString&,const FString&)
    {
        bRefreshing=false;auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C)return;
        // Separate reads can observe different group revisions; never offer a mixed-revision action.
        const auto* Own=S.Groups.FindByPredicate([this](const FHorizonGroupView& G){return G.Id==CurrentGroup.Id;});
        if((Own && Own->Revision!=CurrentGroup.Revision)||(!Own&&CurrentGroup.bHasData)){CurrentGroup={};PendingSnapshot={};}
        if(!PendingSnapshot.GroupId.IsEmpty()&&PendingSnapshot.Revision!=CurrentGroup.Revision)PendingSnapshot={};
        SnapshotContext=C;SnapshotPlayerRevision=S.Revision;SnapshotGeneration=Current->Groups->GetCacheGeneration();auto* Stored=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if(!Intent.IsEmpty()&&HorizonGroupExampleStorage::Valid(Stored,Project,User)&&HorizonGroupExampleStorage::Signature(Stored)==Intent&&Stored->SendSerial==Serial){bRefreshed=true;RefreshedIntent=Intent;RefreshedSerial=Serial;}
    }));
}
void AHorizonGroupsExample::StartAction(const FString& A,const FString& G,const FString& Target,int64 Expected,const FString& Request,const FString& RequestedRole,const FHorizonGroupMetadata& Metadata)
{
    auto* SDK=Session();if(!SDK||bSending||bRefreshing||(SnapshotContext!=SDK->GetHttpClient()->GetRequestContextEpoch()||SnapshotGeneration!=SDK->Groups->GetCacheGeneration()))return;const auto S=SDK->Groups->GetCurrentGroups();if(!S.bPresent)return;
    const FString User=S.UserId,Project=HorizonGroupExampleStorage::Project(SDK),Slot=HorizonGroupExampleStorage::Slot(Project,User);if(UGameplayStatics::DoesSaveGameExist(Slot,0))return;
    auto* P=Cast<UHorizonGroupPendingAction>(UGameplayStatics::CreateSaveGameObject(UHorizonGroupPendingAction::StaticClass()));
    P->Project=Project;P->UserId=User;P->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);P->Action=A;P->GroupId=G;P->TargetUserId=Target;P->ExpectedRevision=Expected;P->RequestId=Request;P->Role=RequestedRole;P->Metadata=Metadata;
    if(!HorizonGroupExampleStorage::Valid(P,Project,User)){UE_LOG(LogTemp,Warning,TEXT("[GroupsExample] Invalid original tuple; nothing sent"));return;}
    if(A==TEXT("create")){if(S.Revision!=Expected)return;}else if(!GetGroup().bPresent||!GetGroup().Id.Equals(G,ESearchCase::IgnoreCase)||GetGroup().Revision!=Expected){UE_LOG(LogTemp,Warning,TEXT("[GroupsExample] Reviewed group revision changed; refresh explicitly"));return;}
    const auto PlayerRequest=[&](const FHorizonGroupPendingView& V){return V.GroupId.Equals(G,ESearchCase::IgnoreCase)&&V.RequestId==Request;};
    const auto GroupRequest=[&](const FHorizonGroupPending& V){return V.UserId==Target&&V.RequestId==Request;};
    if((A==TEXT("accept-invite")||A==TEXT("decline-invite"))&&!S.IncomingInvites.ContainsByPredicate(PlayerRequest))return;
    if(A==TEXT("cancel-request")&&!S.OutgoingRequests.ContainsByPredicate(PlayerRequest))return;
    if((A==TEXT("cancel-invite")||A==TEXT("accept-request")||A==TEXT("decline-request"))&&(!PendingSnapshot.GroupId.Equals(G,ESearchCase::IgnoreCase)||PendingSnapshot.Revision!=Expected||(A==TEXT("cancel-invite")?!PendingSnapshot.Invitations.ContainsByPredicate(GroupRequest):!PendingSnapshot.JoinRequests.ContainsByPredicate(GroupRequest))))return;
    if(!UGameplayStatics::SaveGameToSlot(P,Slot,0)){UE_LOG(LogTemp,Warning,TEXT("[GroupsExample] Original could not be persisted; nothing sent"));return;}Send(P,Slot);
}
void AHorizonGroupsExample::RetrySavedAction()
{
    auto* SDK=Session();if(!SDK||bSending||bRefreshing)return;const FString U=SDK->Auth->GetCurrentUser().UserId,P=HorizonGroupExampleStorage::Project(SDK),S=HorizonGroupExampleStorage::Slot(P,U);auto* Original=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(S,0));if(HorizonGroupExampleStorage::Valid(Original,P,U))Send(Original,S);
}
bool AHorizonGroupsExample::AcknowledgePendingAfterRefresh()
{
    auto* SDK=Session();if(!SDK||bSending||bRefreshing||!bRefreshed||(SnapshotContext!=SDK->GetHttpClient()->GetRequestContextEpoch()||SnapshotGeneration!=SDK->Groups->GetCacheGeneration())||!SDK->Groups->GetCurrentGroups().bPresent||SDK->Groups->GetCurrentGroups().Revision!=SnapshotPlayerRevision)return false;
    const FString U=SDK->Auth->GetCurrentUser().UserId,P=HorizonGroupExampleStorage::Project(SDK),S=HorizonGroupExampleStorage::Slot(P,U);auto* Original=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(S,0));if(!HorizonGroupExampleStorage::Valid(Original,P,U)||HorizonGroupExampleStorage::Signature(Original)!=RefreshedIntent||Original->SendSerial!=RefreshedSerial)return false;bRefreshed=false;return UGameplayStatics::DeleteGameInSlot(S,0);
}
void AHorizonGroupsExample::Send(UHorizonGroupPendingAction* P,const FString& Slot)
{
    auto* SDK=Session();if(!SDK||bSending||!HorizonGroupExampleStorage::Valid(P,HorizonGroupExampleStorage::Project(SDK),SDK->Auth->GetCurrentUser().UserId)||P->SendSerial>=9007199254740991LL)return;
    ++P->SendSerial;if(!UGameplayStatics::SaveGameToSlot(P,Slot,0))return;bSending=true;bRefreshed=false;CurrentGroup={};PendingSnapshot={};
    // Root the immutable captured original for the asynchronous callback; never clear a later intent.
    TStrongObjectPtr<UHorizonGroupPendingAction> Original(DuplicateObject<UHorizonGroupPendingAction>(P,GetTransientPackage()));
    auto Done=FOnGroupMutationComplete::CreateWeakLambda(this,[this,Slot,Original](bool Ok,const FHorizonGroupMutationView&,const FString& Code,const FString&)
    {
        bSending=false;if(!Ok){UE_LOG(LogTemp,Warning,TEXT("[GroupsExample] Original retained; refresh and reconcile explicitly: %s"),*Code);return;}auto* Stored=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(HorizonGroupExampleStorage::Same(Stored,Original.Get()))UGameplayStatics::DeleteGameInSlot(Slot,0);
    });
    const auto& U=P->UserId;const auto& G=P->GroupId;const auto& T=P->TargetUserId;const auto& R=P->RequestId;const int64 E=P->ExpectedRevision;const FString A=P->Action;
    if(A==TEXT("create"))SDK->Groups->CreateGroup(U,E,P->Metadata,Done);else if(A==TEXT("update"))SDK->Groups->UpdateGroup(U,G,E,P->Metadata,Done);
    else if(A==TEXT("join"))SDK->Groups->JoinGroup(U,G,E,Done);else if(A==TEXT("request"))SDK->Groups->RequestJoin(U,G,E,Done);else if(A==TEXT("invite"))SDK->Groups->InvitePlayer(U,G,T,E,Done);
    else if(A==TEXT("accept-invite"))SDK->Groups->AcceptInvite(U,G,E,R,Done);else if(A==TEXT("decline-invite"))SDK->Groups->DeclineInvite(U,G,E,R,Done);else if(A==TEXT("cancel-invite"))SDK->Groups->CancelInvite(U,G,T,E,R,Done);
    else if(A==TEXT("accept-request"))SDK->Groups->AcceptRequest(U,G,T,E,R,Done);else if(A==TEXT("decline-request"))SDK->Groups->DeclineRequest(U,G,T,E,R,Done);else if(A==TEXT("cancel-request"))SDK->Groups->CancelRequest(U,G,E,R,Done);
    else if(A==TEXT("leave"))SDK->Groups->LeaveGroup(U,G,E,Done);else if(A==TEXT("remove"))SDK->Groups->RemoveMember(U,G,T,E,Done);else if(A==TEXT("role"))SDK->Groups->SetMemberRole(U,G,T,E,P->Role,Done);
    else if(A==TEXT("transfer"))SDK->Groups->TransferLeadership(U,G,T,E,Done);else if(A==TEXT("delete"))SDK->Groups->DeleteGroup(U,G,E,Done);
}
