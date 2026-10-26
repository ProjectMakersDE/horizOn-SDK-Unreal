// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Examples/HorizonItemsExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Transport/HorizonItemsTransportContract.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
namespace HorizonItemExampleStorage
{
FString Project(UHorizonSubsystem* S){return UHorizonExampleIdentity::Project(S->GetHttpClient()->GetApiKey());}
FString Slot(const FString& P,const FString& U){return TEXT("horizon-item-original-")+UHorizonExampleIdentity::Project(P+TEXT("|")+U);}
FString Hash(const FString& S){const FTCHARToUTF8 B(*S,S.Len());return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(B.Get()),B.Length()).c_str());}
void Add(FString& S,const FString& V){S+=LexToString(V.Len())+TEXT(":")+V;}
FString Offer(const FHorizonPlayerValueOffer& O)
{
    FString V;for(const FString& S:{O.Key,O.Name,LexToString(O.bActive),LexToString(O.bHasPurchasesPerDay),LexToString(O.PurchasesPerDay)})Add(V,S);
    Add(V,TEXT("costs"));for(const auto& A:O.Costs){Add(V,A.Key);Add(V,LexToString(A.Amount));}Add(V,TEXT("rewards"));for(const auto& A:O.Rewards){Add(V,A.Key);Add(V,LexToString(A.Amount));}Add(V,TEXT("items"));for(const auto& A:O.ItemRewards){Add(V,A.ItemKey);Add(V,LexToString(A.Quantity));}return Hash(V);
}
FString Signature(const UHorizonItemOriginal* P)
{
    if(!P)return {};FString V;for(const FString& S:{P->Project,P->UserId,P->IntentId,P->Action,P->InstanceId,LexToString(P->ExpectedRevision),LexToString(P->Equipped),P->OfferKey,P->OperationId,P->OfferFingerprint,LexToString(P->SendSerial),LexToString(P->Confirmed)})Add(V,S);for(const auto& D:P->Changes){Add(V,D.Key);Add(V,LexToString(D.Amount));}return Hash(V);
}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Key(const FString& S){const FTCHARToUTF8 B(*S,S.Len());return HorizonItemsContract::Key(std::string(B.Get(),B.Length()));}
bool Valid(const UHorizonItemOriginal* P,const FString& Project,const FString& User)
{
    if(!P||P->Project!=Project||P->UserId!=User||!Uuid(P->IntentId)||P->SendSerial<0||P->SendSerial>=HorizonItemsContract::MaxSafe)return false;
    if(P->Action==TEXT("purchase")){FString Time,Id;int64 M;return P->InstanceId.IsEmpty()&&P->Changes.IsEmpty()&&Key(P->OfferKey)&&P->OfferFingerprint.Len()==64&&P->OperationId.Split(TEXT(":"),&Time,&Id)&&Time.Len()<=16&&Time.IsNumeric()&&LexTryParseString(M,*Time)&&M>0&&Uuid(Id);}
    if(P->Confirmed||!P->OfferKey.IsEmpty()||!P->OperationId.IsEmpty()||!P->OfferFingerprint.IsEmpty()||!Uuid(P->InstanceId)||P->ExpectedRevision<0||P->ExpectedRevision>HorizonItemsContract::MaxSafe)return false;
    if(P->Action==TEXT("equip")||P->Action==TEXT("discard"))return P->Changes.IsEmpty();
    if(P->Action!=TEXT("adjust")||P->Changes.IsEmpty()||P->Changes.Num()>8)return false;
    TSet<FString> Keys;for(const auto& D:P->Changes){if(!Key(D.Key)||Keys.Contains(D.Key)||D.Amount==0||D.Amount< -HorizonItemsContract::MaxSafe||D.Amount>HorizonItemsContract::MaxSafe)return false;Keys.Add(D.Key);}return true;
}
}
UHorizonSubsystem* AHorizonItemsExample::Session() const{auto* GI=GetGameInstance();auto* S=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;return S&&S->Auth&&S->Auth->IsSignedIn()&&S->Items&&S->PlayerValues?S:nullptr;}
FHorizonInventorySnapshot AHorizonItemsExample::GetInventory() const{auto* S=Session();return S?S->Items->GetCurrentInventory():FHorizonInventorySnapshot{};}
void AHorizonItemsExample::Refresh()
{
    auto* S=Session();if(!S||Busy)return;Busy=true;Refreshed=false;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonItemExampleStorage::Project(S),Slot=HorizonItemExampleStorage::Slot(P,U);const uint64 C=S->GetHttpClient()->GetRequestContextEpoch();auto* Original=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));const FString Signature=HorizonItemExampleStorage::Valid(Original,P,U)?HorizonItemExampleStorage::Signature(Original):FString();
    S->Items->GetCatalog(U,FOnItemCatalogComplete::CreateWeakLambda(this,[this,U,Slot,Signature,C](bool Ok,const FHorizonItemCatalogPage&,const FString&,const FString&)
    {
        auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C){Busy=false;return;}
        Current->PlayerValues->GetOffers(U,FOnPlayerValuesOffersComplete::CreateWeakLambda(this,[this,Slot,Signature,C](bool Good,const FHorizonPlayerValuesOffers&,const FString&,const FString&){if(!Good){Busy=false;return;}ReadInventory(Slot,Signature,C,false);}));
    }));
}
void AHorizonItemsExample::ReadInventory(const FString& Slot,const FString& Signature,uint64 C,bool FinishConfirmed)
{
    auto* S=Session();if(!S||S->GetHttpClient()->GetRequestContextEpoch()!=C){Busy=false;return;}const FString U=S->Auth->GetCurrentUser().UserId;
    S->Items->GetInventory(U,FOnInventoryComplete::CreateWeakLambda(this,[this,Slot,Signature,C,FinishConfirmed](bool Ok,const FHorizonInventorySnapshot& V,const FString&,const FString&)
    {
        Busy=false;auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C)return;
        SnapshotContext=C;SnapshotGeneration=Current->Items->GetCacheGeneration();SnapshotRevision=V.Revision;RefreshedSignature=Signature;Refreshed=true;
        auto* Saved=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if(FinishConfirmed&&Saved&&Saved->Confirmed&&HorizonItemExampleStorage::Signature(Saved)==Signature)UGameplayStatics::DeleteGameInSlot(Slot,0);
    }));
}
void AHorizonItemsExample::PurchaseItemOffer(const FString& Key)
{
    auto* S=Session();if(!S||Busy||!Refreshed||SnapshotContext!=S->GetHttpClient()->GetRequestContextEpoch()||SnapshotGeneration!=S->Items->GetCacheGeneration())return;
    const auto Offers=S->PlayerValues->GetCurrentOffers();const auto* O=Offers.Offers.FindByPredicate([&](const FHorizonPlayerValueOffer& V){return V.Key==Key;});if(!Offers.bPresent||!O||!O->bActive||O->ItemRewards.IsEmpty()||!S->PlayerValues->HasServerClock())return;
    const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonItemExampleStorage::Project(S),Slot=HorizonItemExampleStorage::Slot(P,U);if(UGameplayStatics::DoesSaveGameExist(Slot,0))return;
    auto* Original=NewObject<UHorizonItemOriginal>();Original->Project=P;Original->UserId=U;Original->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);Original->Action=TEXT("purchase");Original->OfferKey=Key;Original->OfferFingerprint=HorizonItemExampleStorage::Offer(*O);Original->OperationId=S->PlayerValues->CreateOperationId();if(!HorizonItemExampleStorage::Valid(Original,P,U)||!UGameplayStatics::SaveGameToSlot(Original,Slot,0))return;Send(Original,Slot);
}
void AHorizonItemsExample::StartItemAction(const FString& A,const FString& I,int64 R,const TArray<FHorizonItemPropertyDelta>& D,bool B)
{
    auto* S=Session();if(!S||Busy||!Refreshed||SnapshotContext!=S->GetHttpClient()->GetRequestContextEpoch()||SnapshotGeneration!=S->Items->GetCacheGeneration())return;const auto V=S->Items->GetCurrentInventory();if(!V.bPresent||V.Revision!=R||!V.Instances.ContainsByPredicate([&](const FHorizonItemInstance& X){return X.InstanceId==I;}))return;
    const FString P=HorizonItemExampleStorage::Project(S),Slot=HorizonItemExampleStorage::Slot(P,V.UserId);if(UGameplayStatics::DoesSaveGameExist(Slot,0))return;auto* Original=NewObject<UHorizonItemOriginal>();Original->Project=P;Original->UserId=V.UserId;Original->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);Original->Action=A;Original->InstanceId=I;Original->ExpectedRevision=R;Original->Changes=D;Original->Equipped=B;
    if(!HorizonItemExampleStorage::Valid(Original,P,V.UserId)||!UGameplayStatics::SaveGameToSlot(Original,Slot,0))return;Send(Original,Slot);
}
void AHorizonItemsExample::RetrySavedAction(bool Confirm)
{
    auto* S=Session();if(!S||Busy)return;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonItemExampleStorage::Project(S),Slot=HorizonItemExampleStorage::Slot(P,U);auto* Original=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!HorizonItemExampleStorage::Valid(Original,P,U))return;
    if(Original->Action!=TEXT("purchase")){Send(Original,Slot);return;}
    if(Original->Confirmed){Busy=true;ReadInventory(Slot,HorizonItemExampleStorage::Signature(Original),S->GetHttpClient()->GetRequestContextEpoch(),true);return;}
    Busy=true;const FString Signature=HorizonItemExampleStorage::Signature(Original);const uint64 C=S->GetHttpClient()->GetRequestContextEpoch();
    S->PlayerValues->GetOffers(U,FOnPlayerValuesOffersComplete::CreateWeakLambda(this,[this,P,U,Slot,Signature,C,Confirm](bool Ok,const FHorizonPlayerValuesOffers& V,const FString&,const FString&)
    {
        Busy=false;auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C)return;auto* Saved=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!HorizonItemExampleStorage::Valid(Saved,P,U)||HorizonItemExampleStorage::Signature(Saved)!=Signature)return;
        const auto* O=V.Offers.FindByPredicate([&](const FHorizonPlayerValueOffer& X){return X.Key==Saved->OfferKey;});const FString Fingerprint=O&&O->bActive&&!O->ItemRewards.IsEmpty()?HorizonItemExampleStorage::Offer(*O):TEXT("unavailable");
        if(Fingerprint!=Saved->OfferFingerprint&&(!Confirm||ReviewedOfferChange!=Signature+Fingerprint)){ReviewedOfferChange=Signature+Fingerprint;UE_LOG(LogTemp,Warning,TEXT("[ItemsExample] Current offer changed or is unavailable; review and explicitly confirm original receipt recovery. Original operation ID retained."));return;}
        ReviewedOfferChange.Reset();Send(Saved,Slot);
    }));
}
bool AHorizonItemsExample::AcknowledgeItemActionAfterRefresh()
{
    auto* S=Session();if(!S||Busy||!Refreshed||SnapshotContext!=S->GetHttpClient()->GetRequestContextEpoch()||SnapshotGeneration!=S->Items->GetCacheGeneration()||S->Items->GetCurrentInventory().Revision!=SnapshotRevision)return false;
    const FString P=HorizonItemExampleStorage::Project(S),U=S->Auth->GetCurrentUser().UserId,Slot=HorizonItemExampleStorage::Slot(P,U);auto* Saved=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!HorizonItemExampleStorage::Valid(Saved,P,U)||Saved->Action==TEXT("purchase")||HorizonItemExampleStorage::Signature(Saved)!=RefreshedSignature)return false;Refreshed=false;return UGameplayStatics::DeleteGameInSlot(Slot,0);
}
void AHorizonItemsExample::Send(UHorizonItemOriginal* P,const FString& Slot)
{
    auto* S=Session();if(!S||Busy||!HorizonItemExampleStorage::Valid(P,HorizonItemExampleStorage::Project(S),S->Auth->GetCurrentUser().UserId))return;
    ++P->SendSerial;if(!UGameplayStatics::SaveGameToSlot(P,Slot,0))return;Busy=true;Refreshed=false;const uint64 C=S->GetHttpClient()->GetRequestContextEpoch();TStrongObjectPtr<UHorizonItemOriginal> Original(DuplicateObject<UHorizonItemOriginal>(P,GetTransientPackage()));
    const auto Done=FOnInventoryComplete::CreateWeakLambda(this,[this,Slot,Original,C](bool Ok,const FHorizonInventorySnapshot&,const FString& Code,const FString&)
    {
        Busy=false;auto* Current=Session();if(!Ok){UE_LOG(LogTemp,Warning,TEXT("[ItemsExample] Original retained; no replacement action: %s"),*Code);return;}if(!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C)return;auto* Saved=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(Saved&&HorizonItemExampleStorage::Signature(Saved)==HorizonItemExampleStorage::Signature(Original.Get()))UGameplayStatics::DeleteGameInSlot(Slot,0);
    });
    if(P->Action==TEXT("purchase"))S->PlayerValues->Purchase(P->UserId,P->OfferKey,P->OperationId,FOnPlayerValuesComplete::CreateWeakLambda(this,[this,Slot,Original,C](bool Ok,const FHorizonPlayerValuesState& V,const FString& Code,const FString&)
    {
        Busy=false;auto* Current=Session();if(!Ok){UE_LOG(LogTemp,Warning,TEXT("[ItemsExample] Purchase original retained, including expired/unknown outcomes: %s"),*Code);return;}if(!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C||!V.Operation.bPresent||V.Operation.OperationId!=Original->OperationId)return;auto* Saved=Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!Saved||HorizonItemExampleStorage::Signature(Saved)!=HorizonItemExampleStorage::Signature(Original.Get()))return;
        Saved->Confirmed=true;if(!UGameplayStatics::SaveGameToSlot(Saved,Slot,0))return;Busy=true;ReadInventory(Slot,HorizonItemExampleStorage::Signature(Saved),C,true);
    }));
    else if(P->Action==TEXT("adjust"))S->Items->AdjustProperties(P->UserId,P->InstanceId,P->ExpectedRevision,P->Changes,Done);
    else if(P->Action==TEXT("equip"))S->Items->SetEquipped(P->UserId,P->InstanceId,P->ExpectedRevision,P->Equipped,Done);
    else S->Items->DiscardItem(P->UserId,P->InstanceId,P->ExpectedRevision,Done);
}
