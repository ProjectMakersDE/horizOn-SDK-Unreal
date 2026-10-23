// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.
#include "Examples/HorizonGiftCodesExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
#include "Transport/HorizonRetryContract.h"
namespace HorizonGiftCodesExampleStorage
{
FString Project(UHorizonSubsystem* SDK) { return FMD5::HashAnsiString(*SDK->GetHttpClient()->GetApiKey()); }
FString Slot(const FString& Project,const FString& User) { return TEXT("horizon-gift-redeem-")+FMD5::HashAnsiString(*(Project+TEXT("|")+User)); }
bool Valid(const UHorizonGiftCodePendingRedeem* Pending,const FString& Project,const FString& User)
{ return Pending && Pending->Intent.IsValidFor(Project,User); }

}
AHorizonGiftCodesExample::AHorizonGiftCodesExample() { PrimaryActorTick.bCanEverTick=false; }
UHorizonSubsystem* AHorizonGiftCodesExample::Session() const
{
    auto* Instance=GetGameInstance(); auto* SDK=Instance?Instance->GetSubsystem<UHorizonSubsystem>():nullptr;
    return SDK && SDK->Auth && SDK->Auth->IsSignedIn()?SDK:nullptr;
}
void AHorizonGiftCodesExample::RedeemGiftCode() { Send(false); }
void AHorizonGiftCodesExample::RetryPendingRedemption() { Send(true); }
FHorizonGiftCodeIntent AHorizonGiftCodesExample::GetPendingIntent() const
{
    auto* SDK=Session(); if (!SDK) return {};
    const FString Project=HorizonGiftCodesExampleStorage::Project(SDK),User=SDK->Auth->GetCurrentUser().UserId;
    auto* Pending=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(HorizonGiftCodesExampleStorage::Slot(Project,User),0));
    return HorizonGiftCodesExampleStorage::Valid(Pending,Project,User)?Pending->Intent:FHorizonGiftCodeIntent();
}
void AHorizonGiftCodesExample::Send(bool Retry)
{
    auto* SDK=Session(); if (!SDK || bSending || !SDK->GiftCodes) return;
    const FString Project=HorizonGiftCodesExampleStorage::Project(SDK),User=SDK->Auth->GetCurrentUser().UserId;
    const FString Slot=HorizonGiftCodesExampleStorage::Slot(Project,User);
    const bool Existing=UGameplayStatics::DoesSaveGameExist(Slot,0);
    auto* Pending=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (Existing && !HorizonGiftCodesExampleStorage::Valid(Pending,Project,User))
    { UE_LOG(LogTemp,Warning,TEXT("[GiftCodesExample] Stored original intent needs recovery; nothing sent or replaced")); return; }
    if (Retry && !Existing) return;
    if (!Retry && Existing) { UE_LOG(LogTemp,Warning,TEXT("Resolve or explicitly retry the saved tuple first")); return; }
    bSending=true;
    if (Pending) { Dispatch(SDK,Pending->Intent,Slot); return; }
    SDK->GiftCodes->PrepareRedemption(GiftCode,FOnGiftCodePrepareComplete::CreateWeakLambda(this,[this,Project,User,Slot](bool Ok,const FHorizonGiftCodeIntent& Intent,const FString& Error,const FString&)
    {
        auto* Current=Session();
        if (!Ok || !Current || HorizonGiftCodesExampleStorage::Project(Current)!=Project || Current->Auth->GetCurrentUser().UserId!=User)
        { bSending=false; UE_LOG(LogTemp,Warning,TEXT("Preparation stopped: %s"),*Error); return; }
        if (UGameplayStatics::DoesSaveGameExist(Slot,0)) { bSending=false; return; }
        auto* Saved=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::CreateSaveGameObject(UHorizonGiftCodePendingRedeem::StaticClass()));
        Saved->Intent=Intent;
        if (!UGameplayStatics::SaveGameToSlot(Saved,Slot,0)) { bSending=false; return; }
        auto* Verified=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if (!Verified || !Verified->Intent.SameIdentity(Intent)) { bSending=false; return; }
        Dispatch(Current,Intent,Slot);
    }));
}
void AHorizonGiftCodesExample::Dispatch(UHorizonSubsystem* SDK,const FHorizonGiftCodeIntent& Intent,const FString& Slot)
{
    SDK->GiftCodes->RedeemPrepared(Intent,FOnGiftCodeRedeemResultComplete::CreateWeakLambda(this,[this,Intent,Slot](bool Ok,const FHorizonGiftCodeRedeemResult& Result,const FString& Error,const FString&)
    {
        bSending=false;
        if (!Ok) { UE_LOG(LogTemp,Warning,TEXT("Keep original tuple for recovery/retry: %s"),*Error); return; }
        auto* Current=Session();
        if (!Current || !Intent.IsValidFor(HorizonGiftCodesExampleStorage::Project(Current),Current->Auth->GetCurrentUser().UserId)) return;
        auto* Saved=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if (Saved && Saved->Intent.SameIdentity(Intent)) UGameplayStatics::DeleteGameInSlot(Slot,0);
        UE_LOG(LogTemp,Log,TEXT("Current state accepted; alreadyApplied=%s, no historical rewards on replay"),Result.bAlreadyApplied?TEXT("true"):TEXT("false"));
    }));
}
