// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "AsyncActions/HorizonAsync_Items.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonItemsManager.h"
UHorizonAsync_ItemsInventory* UHorizonAsync_ItemsInventory::Create(const UObject* World,const FString& A,const FString& I,int64 R,const TArray<FHorizonItemPropertyDelta>& D,bool B)
{auto* N=NewObject<UHorizonAsync_ItemsInventory>();N->WorldContext=World;N->Action=A;N->Instance=I;N->Revision=R;N->Changes=D;N->Equipped=B;auto* S=UHorizonBlueprintLibrary::GetHorizonSubsystem(World);if(S&&S->Auth){N->User=S->Auth->GetCurrentUser().UserId;N->Context=S->GetHttpClient()->GetRequestContextEpoch();}N->RegisterWithGameInstance(World);return N;}
UHorizonAsync_ItemsInventory* UHorizonAsync_ItemsInventory::GetInventory(const UObject* W){return Create(W,TEXT("get"),TEXT(""),0,{},false);}
UHorizonAsync_ItemsInventory* UHorizonAsync_ItemsInventory::AdjustProperties(const UObject* W,const FString& I,int64 R,const TArray<FHorizonItemPropertyDelta>& D){return Create(W,TEXT("adjust"),I,R,D,false);}
UHorizonAsync_ItemsInventory* UHorizonAsync_ItemsInventory::SetEquipped(const UObject* W,const FString& I,int64 R,bool B){return Create(W,TEXT("equip"),I,R,{},B);}
UHorizonAsync_ItemsInventory* UHorizonAsync_ItemsInventory::DiscardItem(const UObject* W,const FString& I,int64 R){return Create(W,TEXT("discard"),I,R,{},false);}
void UHorizonAsync_ItemsInventory::Activate()
{
    if(bActivated)return;bActivated=true;auto* S=UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
    if(!S||!S->Items){Complete(false,{},TEXT("SDK_NOT_CONNECTED"),TEXT("Items manager unavailable"));return;}
    if(S->GetHttpClient()->GetRequestContextEpoch()!=Context){Complete(false,{},TEXT("STALE_RESPONSE"),TEXT("Original Blueprint request context changed"));return;}
    const auto D=FOnInventoryComplete::CreateUObject(this,&UHorizonAsync_ItemsInventory::Complete);
    if(Action==TEXT("get"))S->Items->GetInventory(User,D);else if(Action==TEXT("adjust"))S->Items->AdjustProperties(User,Instance,Revision,Changes,D);else if(Action==TEXT("equip"))S->Items->SetEquipped(User,Instance,Revision,Equipped,D);else S->Items->DiscardItem(User,Instance,Revision,D);
}
void UHorizonAsync_ItemsInventory::Complete(bool B,const FHorizonInventorySnapshot& S,const FString& C,const FString& M){OnComplete.Broadcast(B,S,C,M);SetReadyToDestroy();}
UHorizonAsync_ItemCatalog* UHorizonAsync_ItemCatalog::GetCatalog(const UObject* W,const FString& A,int32 L,int64 R)
{auto* N=NewObject<UHorizonAsync_ItemCatalog>();N->WorldContext=W;N->After=A;N->Limit=L;N->Revision=R;auto* S=UHorizonBlueprintLibrary::GetHorizonSubsystem(W);if(S&&S->Auth){N->User=S->Auth->GetCurrentUser().UserId;N->Context=S->GetHttpClient()->GetRequestContextEpoch();}N->RegisterWithGameInstance(W);return N;}
void UHorizonAsync_ItemCatalog::Activate()
{if(bActivated)return;bActivated=true;auto* S=UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());if(!S||!S->Items){Complete(false,{},TEXT("SDK_NOT_CONNECTED"),TEXT("Items manager unavailable"));return;}if(S->GetHttpClient()->GetRequestContextEpoch()!=Context){Complete(false,{},TEXT("STALE_RESPONSE"),TEXT("Original Blueprint request context changed"));return;}S->Items->GetCatalog(User,FOnItemCatalogComplete::CreateUObject(this,&UHorizonAsync_ItemCatalog::Complete),After,Limit,Revision);}
void UHorizonAsync_ItemCatalog::Complete(bool B,const FHorizonItemCatalogPage& P,const FString& C,const FString& M){OnComplete.Broadcast(B,P,C,M);SetReadyToDestroy();}
