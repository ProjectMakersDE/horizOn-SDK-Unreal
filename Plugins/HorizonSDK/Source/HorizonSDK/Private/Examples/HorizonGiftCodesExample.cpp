// Copyright (c) 2025-2026 horizOn. All rights reserved.
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
{
    return Pending && Pending->ProjectScope==Project && Pending->UserId==User && !Pending->Code.TrimStartAndEnd().IsEmpty() && Pending->Code.Len()<=50 &&
        Pending->IdempotencyId.Len()<=128 && HorizonTransportContract::HasNonBlankIdempotencyId(TCHAR_TO_UTF8(*Pending->IdempotencyId));
}
}
AHorizonGiftCodesExample::AHorizonGiftCodesExample() { PrimaryActorTick.bCanEverTick=false; }
UHorizonSubsystem* AHorizonGiftCodesExample::Session() const
{
    auto* Instance=GetGameInstance(); auto* SDK=Instance?Instance->GetSubsystem<UHorizonSubsystem>():nullptr;
    return SDK && SDK->Auth && SDK->Auth->IsSignedIn()?SDK:nullptr;
}
void AHorizonGiftCodesExample::RedeemGiftCode() { Send(false); }
void AHorizonGiftCodesExample::RetryPendingRedemption() { Send(true); }
FString AHorizonGiftCodesExample::GetPendingIdempotencyId() const
{
    auto* SDK=Session(); if (!SDK) return {};
    const FString Project=HorizonGiftCodesExampleStorage::Project(SDK),User=SDK->Auth->GetCurrentUser().UserId;
    auto* Pending=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(HorizonGiftCodesExampleStorage::Slot(Project,User),0));
    return HorizonGiftCodesExampleStorage::Valid(Pending,Project,User)?Pending->IdempotencyId:FString();
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
    if (!Retry && Existing && Pending->Code!=GiftCode)
    { UE_LOG(LogTemp,Warning,TEXT("[GiftCodesExample] Finish the original pending code before starting another")); return; }
    if (!Pending)
    {
        if (GiftCode.TrimStartAndEnd().IsEmpty() || GiftCode.Len()>50) return;
        Pending=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::CreateSaveGameObject(UHorizonGiftCodePendingRedeem::StaticClass()));
        Pending->ProjectScope=Project; Pending->UserId=User; Pending->Code=GiftCode; Pending->IdempotencyId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
        if (!UGameplayStatics::SaveGameToSlot(Pending,Slot,0))
        { UE_LOG(LogTemp,Warning,TEXT("[GiftCodesExample] Could not persist original intent; nothing sent")); return; }
    }
    const FString OriginalCode=Pending->Code,OriginalId=Pending->IdempotencyId;
    bSending=true;
    SDK->GiftCodes->RedeemWithIdempotency(OriginalCode,OriginalId,FOnGiftCodeRedeemResultComplete::CreateWeakLambda(this,
        [this,Slot,Project,User,OriginalCode,OriginalId](bool Success,const FHorizonGiftCodeRedeemResult& Result,const FString& Code,const FString&)
        {
            bSending=false;
            if (!Success) { UE_LOG(LogTemp,Warning,TEXT("[GiftCodesExample] Keep original intent for explicit recovery/retry: %s"),*Code); return; }
            auto* Stored=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(Slot,0));
            if (HorizonGiftCodesExampleStorage::Valid(Stored,Project,User) && Stored->Code==OriginalCode && Stored->IdempotencyId==OriginalId)
                if (!UGameplayStatics::DeleteGameInSlot(Slot,0)) UE_LOG(LogTemp,Warning,TEXT("[GiftCodesExample] Receipt accepted; retain original ID for safe replay"));
            // Render Result.PlayerValues or the manager's complete state. Never add GrantedValues locally.
            UE_LOG(LogTemp,Log,TEXT("[GiftCodesExample] Receipt accepted (replayed=%s, %d value grants)"),Result.bReplayed?TEXT("true"):TEXT("false"),Result.GrantedValues.Num());
        }));
}
