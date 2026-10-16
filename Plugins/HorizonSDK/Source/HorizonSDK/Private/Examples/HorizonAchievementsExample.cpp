#include "Examples/HorizonAchievementsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAchievementsManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
namespace HorizonAchievementExampleStorage
{
FString Project(UHorizonSubsystem* SDK) { return FMD5::HashAnsiString(*SDK->GetHttpClient()->GetApiKey()); }
FString Slot(const FString& Project,const FString& User) { return TEXT("horizon-achievement-delivery-")+FMD5::HashAnsiString(*(Project+TEXT("|")+User)); }
bool Keys(const TArray<FString>& Values)
{
    TSet<FString> Unique;
    for (const auto& Key:Values) { if (!UHorizonAchievementsManager::IsKey(Key) || Unique.Contains(Key)) return false; Unique.Add(Key); }
    return !Values.IsEmpty() && Values.Num()<=8;
}
bool Valid(const UHorizonAchievementPendingDelivery* Pending,const FString& Project,const FString& User)
{ return Pending && Pending->ProjectScope==Project && Pending->UserId==User && Keys(Pending->Keys); }
}
UHorizonSubsystem* AHorizonAchievementsExample::Session() const
{
    auto* Instance=GetGameInstance(); auto* SDK=Instance?Instance->GetSubsystem<UHorizonSubsystem>():nullptr;
    return SDK && SDK->Auth && SDK->Auth->IsSignedIn()?SDK:nullptr;
}
void AHorizonAchievementsExample::LoadAchievements()
{
    if (auto* SDK=Session()) SDK->Achievements->GetAchievements(SDK->Auth->GetCurrentUser().UserId,FOnAchievementsComplete::CreateWeakLambda(this,
        [](bool Ok,const FHorizonAchievementsSnapshot&,const FString& Code,const FString&) { if (!Ok) UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Refresh failed: %s"),*Code); }));
}
void AHorizonAchievementsExample::RefreshAfterStatistics(const FHorizonStatisticsSnapshot& Statistics)
{
    auto* SDK=Session();
    if (SDK && Statistics.bValid && Statistics.UserId==SDK->Auth->GetCurrentUser().UserId && !Statistics.bReplayed && !Statistics.NewlyUnlocked.IsEmpty()) LoadAchievements();
}
void AHorizonAchievementsExample::UnlockAchievement(const FString& Key)
{
    if (auto* SDK=Session()) SDK->Achievements->Unlock(SDK->Auth->GetCurrentUser().UserId,Key,FOnAchievementUnlockComplete::CreateWeakLambda(this,
        [](bool Ok,const FHorizonAchievementUnlockResponse& R,const FString& Code,const FString&)
        {
            if (!Ok) { UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Retry the same original key explicitly: %s"),*Code); return; }
            // Display R.Achievements.Progress: PendingReward holds original values, not edited definition rewards.
            UE_LOG(LogTemp,Log,TEXT("[AchievementsExample] New unlocks: %d; replayed=%s"),R.NewlyUnlocked.Num(),R.bReplayed?TEXT("true"):TEXT("false"));
        }));
}
FHorizonAchievementsSnapshot AHorizonAchievementsExample::GetCurrentAchievements() const
{ auto* SDK=Session(); return SDK?SDK->Achievements->GetCurrentAchievements():FHorizonAchievementsSnapshot{}; }
FString AHorizonAchievementsExample::GetDeliveredInboxEntryId(const FString& Key) const
{
    const auto Snapshot=GetCurrentAchievements(); const auto* P=Snapshot.Progress.FindByPredicate([&](const FHorizonAchievementProgress& V) { return V.Key==Key; });
    return P && P->RewardState==TEXT("delivered")?P->InboxEntryId:FString();
}
void AHorizonAchievementsExample::DeliverPendingRewards(const TArray<FString>& Keys) { Deliver(Keys,false); }
void AHorizonAchievementsExample::RetrySavedDelivery() { Deliver({},true); }
void AHorizonAchievementsExample::Deliver(const TArray<FString>& Keys,bool Retry)
{
    auto* SDK=Session(); if (!SDK || bDelivering) return;
    const FString Project=HorizonAchievementExampleStorage::Project(SDK),User=SDK->Auth->GetCurrentUser().UserId;
    const FString Slot=HorizonAchievementExampleStorage::Slot(Project,User);
    const bool Existing=UGameplayStatics::DoesSaveGameExist(Slot,0);
    auto* Pending=Cast<UHorizonAchievementPendingDelivery>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (Existing && !HorizonAchievementExampleStorage::Valid(Pending,Project,User))
    { UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Preserve unreadable original pending keys; nothing sent")); return; }
    if (Retry && !Existing) return;
    if (!Retry && Existing && Pending->Keys!=Keys)
    { UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Finish the original pending keys first")); return; }
    if (!Pending)
    {
        if (!HorizonAchievementExampleStorage::Keys(Keys)) return;
        const auto Snapshot=SDK->Achievements->GetCurrentAchievements();
        for (const auto& Key:Keys)
            if (!Snapshot.Progress.ContainsByPredicate([&](const FHorizonAchievementProgress& P) { return P.Key==Key && P.RewardState==TEXT("pending") && P.PendingReward.bPresent; })) return;
        Pending=Cast<UHorizonAchievementPendingDelivery>(UGameplayStatics::CreateSaveGameObject(UHorizonAchievementPendingDelivery::StaticClass()));
        Pending->ProjectScope=Project; Pending->UserId=User; Pending->Keys=Keys;
        if (!UGameplayStatics::SaveGameToSlot(Pending,Slot,0))
        { UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Pending keys could not be persisted; nothing sent")); return; }
    }
    const auto OriginalKeys=Pending->Keys; bDelivering=true;
    SDK->Achievements->DeliverRewards(User,OriginalKeys,FOnAchievementDeliveryComplete::CreateWeakLambda(this,
        [this,Slot,Project,User,OriginalKeys](bool Ok,const FHorizonAchievementDeliveryResponse& R,const FString& Code,const FString&)
        {
            bDelivering=false;
            if (!Ok) { UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Original pending keys retained: %s"),*Code); return; }
            const bool Finished=R.Results.Num()==OriginalKeys.Num() && !R.Results.ContainsByPredicate([](const FHorizonAchievementDeliveryResult& V) { return V.State==TEXT("pending") || !V.Code.IsEmpty(); });
            auto* Stored=Cast<UHorizonAchievementPendingDelivery>(UGameplayStatics::LoadGameFromSlot(Slot,0));
            if (Finished && HorizonAchievementExampleStorage::Valid(Stored,Project,User) && Stored->Keys==OriginalKeys)
                if (!UGameplayStatics::DeleteGameInSlot(Slot,0)) UE_LOG(LogTemp,Warning,TEXT("[AchievementsExample] Retain original keys for safe replay"));
            // INBOX_FULL remains pending. Delivered IDs link to Inbox; only its explicit Claim changes values.
        }));
}
