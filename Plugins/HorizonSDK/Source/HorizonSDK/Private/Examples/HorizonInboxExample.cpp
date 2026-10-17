#include "Examples/HorizonInboxExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonInboxManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"

UHorizonSubsystem* AHorizonInboxExample::Session() const
{
    auto* Instance=GetGameInstance(); auto* SDK=Instance?Instance->GetSubsystem<UHorizonSubsystem>():nullptr;
    return SDK && SDK->Auth && SDK->Auth->IsSignedIn()?SDK:nullptr;
}
void AHorizonInboxExample::LoadInbox()
{
    if (auto* SDK=Session()) SDK->Inbox->GetInbox(SDK->Auth->GetCurrentUser().UserId,FOnInboxComplete::CreateUObject(this,&AHorizonInboxExample::Loaded));
}
void AHorizonInboxExample::MarkRead(const FString& EntryId)
{
    if (auto* SDK=Session()) SDK->Inbox->MarkRead(SDK->Auth->GetCurrentUser().UserId,EntryId,FOnInboxComplete::CreateUObject(this,&AHorizonInboxExample::Loaded));
}
void AHorizonInboxExample::Remove(const FString& EntryId)
{
    if (auto* SDK=Session()) SDK->Inbox->Remove(SDK->Auth->GetCurrentUser().UserId,EntryId,FOnInboxComplete::CreateUObject(this,&AHorizonInboxExample::Loaded));
}
void AHorizonInboxExample::Loaded(bool Success,const FHorizonInboxSnapshot& Inbox,const FString& Code,const FString&)
{
    if (!Success) { UE_LOG(LogTemp,Warning,TEXT("[InboxExample] Snapshot/action failed: %s"),*Code); }
    else for (const auto& Entry:Inbox.Entries)
    {
        // Use a plain text widget (rich text disabled). Other messages use localization keys.
        FString Text; if (!Entry.TryGetFreeText(Text)) Text=Entry.TextKey;
        UE_LOG(LogTemp,Log,TEXT("[InboxExample] %s"),*Text);
        FHorizonInboxSource Source;
        if (Entry.TryGetSource(Source)) UE_LOG(LogTemp,Log,TEXT("[InboxExample] Source %s"),*Source.SourceId);
    }
}
FHorizonInboxSnapshot AHorizonInboxExample::GetCurrentInbox() const
{
    auto* SDK=Session(); return SDK?SDK->Inbox->GetCurrentInbox():FHorizonInboxSnapshot{};
}
FString AHorizonInboxExample::GetPendingClaimEntryId() const
{
    auto* SDK=Session(); if (!SDK) return {};
    const FString User=SDK->Auth->GetCurrentUser().UserId;
    const FString Scope=FMD5::HashAnsiString(*SDK->GetHttpClient()->GetApiKey());
    const FString Slot=TEXT("horizon-inbox-claim-")+FMD5::HashAnsiString(*(Scope+TEXT("|")+User));
    auto* Pending=Cast<UHorizonInboxPendingClaim>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    return Pending && Pending->UserId==User && Pending->ProjectScope==Scope && FHorizonInboxEntry::IsEntryId(Pending->EntryId)?Pending->EntryId:FString();
}
void AHorizonInboxExample::Claim(const FString& EntryId)
{
    auto* SDK=Session(); if (!SDK || bClaiming || !FHorizonInboxEntry::IsEntryId(EntryId)) return;
    const FString User=SDK->Auth->GetCurrentUser().UserId;
    const FString Scope=FMD5::HashAnsiString(*SDK->GetHttpClient()->GetApiKey());
    const FString Slot=TEXT("horizon-inbox-claim-")+FMD5::HashAnsiString(*(Scope+TEXT("|")+User));
    const bool Existing=UGameplayStatics::DoesSaveGameExist(Slot,0);
    auto* Pending=Cast<UHorizonInboxPendingClaim>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (Existing && (!Pending || Pending->UserId!=User || Pending->ProjectScope!=Scope || Pending->EntryId!=EntryId))
    { UE_LOG(LogTemp,Warning,TEXT("[InboxExample] Preserve and reconcile the original pending entry first")); return; }
    if (!Pending)
    {
        const auto Snapshot=SDK->Inbox->GetCurrentInbox();
        const auto* Item=Snapshot.Entries.FindByPredicate([&](const FHorizonInboxEntry& Value) { return Value.Id==EntryId; });
        if (!Item || !Item->CanClaim())
        { UE_LOG(LogTemp,Warning,TEXT("[InboxExample] Load the Inbox and select an unclaimed valuable entry first")); return; }
        Pending=Cast<UHorizonInboxPendingClaim>(UGameplayStatics::CreateSaveGameObject(UHorizonInboxPendingClaim::StaticClass()));
        Pending->UserId=User; Pending->ProjectScope=Scope; Pending->EntryId=EntryId;
        if (!UGameplayStatics::SaveGameToSlot(Pending,Slot,0)) { UE_LOG(LogTemp,Warning,TEXT("[InboxExample] Pending entry could not be persisted; nothing sent")); return; }
    }
    bClaiming=true;
    SDK->Inbox->Claim(User,Pending->EntryId,FOnInboxClaimComplete::CreateWeakLambda(this,[this,Slot](bool Success,const FHorizonInboxClaim& Result,const FString& Code,const FString&)
    {
        bClaiming=false;
        if (!Success) { UE_LOG(LogTemp,Warning,TEXT("[InboxExample] Original pending entry retained: %s"),*Code); return; }
        // Both managers already adopted complete server snapshots. Render via GetCurrentInbox().
        // Returned references are not full items. Refresh the authoritative inventory explicitly.
        for (const auto& Item:Result.Items) UE_LOG(LogTemp,Log,TEXT("[InboxExample] Returned %s: %s"),*Item.ItemKey,*Item.InstanceId);
        if (!UGameplayStatics::DeleteGameInSlot(Slot,0)) UE_LOG(LogTemp,Warning,TEXT("[InboxExample] Receipt accepted; original entry retained for safe replay"));
    }));
}
