// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "HorizonSessionSave.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"
namespace HorizonExampleIdentityStorage
{
const FString Binding=TEXT("horizon_example_session_binding_v1");
bool Hash(const FString& S) { if (S.Len()!=64) return false; for (TCHAR C:S) if (!(C>='a' && C<='f') && !(C>='0' && C<='9')) return false; return true; }
FString Part(const FString& S) { return FString::Printf(TEXT("%d:"),S.Len())+S; }
}
FString UHorizonExampleIdentity::Project(const FString& Value)
{ FTCHARToUTF8 Bytes(*Value); return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length()).c_str()); }
FString UHorizonExampleIdentity::ProjectKey(UHorizonSubsystem* SDK) { return SDK && SDK->GetHttpClient()?SDK->GetHttpClient()->GetApiKey():FString(); }
FString UHorizonExampleIdentity::Slot(const FString& Key) { return TEXT("horizon_example_guest_v1_")+Project(Key); }
FString UHorizonExampleIdentity::CacheHash() const
{ using namespace HorizonExampleIdentityStorage; return Project(Part(UserId)+Part(AccessToken)+Part(AnonymousToken)+Part(DisplayName)+(bAnonymous?TEXT("1"):TEXT("0"))); }
bool UHorizonExampleIdentity::Valid() const
{
    using namespace HorizonExampleIdentityStorage; FGuid Id;
    if (!ProjectHash.IsEmpty() && !Hash(ProjectHash) || !PreviousCacheHash.IsEmpty() && !Hash(PreviousCacheHash)) return false;
    if (bSignupUncertain) return Hash(ProjectHash) && UserId.IsEmpty() && AccessToken.IsEmpty() && AnonymousToken.IsEmpty();
    return (FGuid::ParseExact(UserId,EGuidFormats::DigitsWithHyphens,Id) || ProjectHash.IsEmpty() && UserId.IsEmpty() && bAnonymous && !AnonymousToken.IsEmpty()) &&
        (!AccessToken.IsEmpty() || bAnonymous && !AnonymousToken.IsEmpty()) && AccessToken.Len()<=8192 && AnonymousToken.Len()<=4096 && DisplayName.Len()<=1024;
}
bool UHorizonExampleIdentity::Same(const UHorizonExampleIdentity* R) const
{ return R && ProjectHash==R->ProjectHash && CacheHash()==R->CacheHash() && bSignupUncertain==R->bSignupUncertain && PreviousCacheHash==R->PreviousCacheHash; }
UHorizonExampleIdentity* UHorizonExampleIdentity::Read(const FString& Name,bool& Blocked)
{
    Blocked=false; if (!UGameplayStatics::DoesSaveGameExist(Name,0)) return nullptr;
    auto* R=Cast<UHorizonExampleIdentity>(UGameplayStatics::LoadGameFromSlot(Name,0)); Blocked=!R || !R->Valid(); return Blocked?nullptr:R;
}
bool UHorizonExampleIdentity::Store(const FString& Name) const
{
    if (!Valid() || !UGameplayStatics::SaveGameToSlot(const_cast<UHorizonExampleIdentity*>(this),Name,0)) return false;
    bool Blocked; return Same(Read(Name,Blocked)) && !Blocked;
}
bool UHorizonExampleIdentity::Backup() const
{
    const FString Name=TEXT("horizon_example_legacy_v1_")+CacheHash(); bool Blocked; auto* Old=Read(Name,Blocked);
    // A legacy backup stays unbound. Validated ownership lives in the binding and guest records.
    return !Blocked && (Old?Old->CacheHash()==CacheHash():Store(Name));
}
UHorizonExampleIdentity* UHorizonExampleIdentity::Cache()
{
    if (!UHorizonSessionSave::HasSavedSession()) return nullptr; auto* Saved=UHorizonSessionSave::LoadFromDisk(); if (!Saved) return nullptr;
    auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=Saved->CachedUserId; R->AccessToken=Saved->CachedAccessToken; R->AnonymousToken=Saved->CachedAnonymousToken;
    R->DisplayName=Saved->CachedDisplayName; R->bAnonymous=Saved->bIsAnonymous || Saved->CachedUserId.IsEmpty() && !Saved->CachedAnonymousToken.IsEmpty(); return R->Valid()?R:nullptr;
}
UHorizonExampleIdentity* UHorizonExampleIdentity::Bound(const FString& Key,const UHorizonExampleIdentity* R)
{ if (!R || Key.IsEmpty()) return nullptr; auto* Copy=DuplicateObject<UHorizonExampleIdentity>(R,GetTransientPackage()); Copy->ProjectHash=Project(Key); Copy->bSignupUncertain=false; Copy->PreviousCacheHash.Reset(); return Copy->Valid()?Copy:nullptr; }
bool UHorizonExampleIdentity::Stage(bool Force) const
{
    if (!Valid() || bSignupUncertain) return false; if (!Force && UHorizonSessionSave::HasSavedSession()) return true;
    auto* Saved=NewObject<UHorizonSessionSave>(); Saved->CachedUserId=UserId; Saved->CachedAccessToken=AccessToken; Saved->CachedAnonymousToken=AnonymousToken;
    Saved->CachedDisplayName=DisplayName; Saved->bIsAnonymous=bAnonymous; return Saved->SaveToDisk();
}
bool UHorizonExampleIdentity::SaveGuest(const FString& Key,const UHorizonExampleIdentity* Record)
{
    auto* R=Bound(Key,Record); if (!R || !R->bAnonymous) return false;
    bool Blocked; auto* Previous=Read(Slot(Key),Blocked); if (Blocked || Previous && !Previous->bSignupUncertain && Previous->UserId!=R->UserId) return false;
    return R->Store(Slot(Key));
}
bool UHorizonExampleIdentity::CaptureBeforeAuth(UHorizonSubsystem* SDK,UHorizonExampleIdentity*& Original)
{
    Original=nullptr; if (!SDK || ProjectKey(SDK).IsEmpty()) return false;
    auto* Current=Cache(); if (!Current) return !UHorizonSessionSave::HasSavedSession();
    bool Blocked; auto* Binding=Read(HorizonExampleIdentityStorage::Binding,Blocked); if (Blocked) return false;
    if (Binding && !Binding->bSignupUncertain && Binding->CacheHash()==Current->CacheHash()) Current->ProjectHash=Binding->ProjectHash;
    if (!Current->Backup()) return false; Original=Current; return true;
}
bool UHorizonExampleIdentity::ShouldRestoreGuest(UHorizonSubsystem* SDK)
{
    const FString Key=ProjectKey(SDK); if (Key.IsEmpty()) return true;
    if (UGameplayStatics::DoesSaveGameExist(Slot(Key),0)) return true;
    auto* Current=Cache(); if (!Current) return UHorizonSessionSave::HasSavedSession();
    bool Blocked; auto* Binding=Read(HorizonExampleIdentityStorage::Binding,Blocked); if (Blocked) return true;
    if (Binding && Binding->CacheHash()==Current->CacheHash() && Binding->ProjectHash!=Project(Key)) return false;
    return Current->bAnonymous;
}
bool UHorizonExampleIdentity::BeforeSignup(UHorizonSubsystem* SDK,const FString& Key)
{
    if (Key.IsEmpty() || ProjectKey(SDK)!=Key || UGameplayStatics::DoesSaveGameExist(Slot(Key),0)) return false;
    auto* Current=Cache(); if (!Current && UHorizonSessionSave::HasSavedSession()) return false;
    bool Blocked; auto* Binding=Read(HorizonExampleIdentityStorage::Binding,Blocked); if (Blocked) return false;
    // A known other project is preserved separately; unbound/same-project guests must be restored.
    if (Current && Current->bAnonymous && !(Binding && Binding->CacheHash()==Current->CacheHash() && Binding->ProjectHash!=Project(Key))) return false;
    auto* Marker=NewObject<UHorizonExampleIdentity>(); Marker->ProjectHash=Project(Key); Marker->bSignupUncertain=true; Marker->PreviousCacheHash=Current?Current->CacheHash():FString();
    return Marker->Store(Slot(Key));
}
bool UHorizonExampleIdentity::RecordIssuedSignup(const FString& Key,const FHorizonUserData& Issued)
{
    bool Blocked; auto* Marker=Read(Slot(Key),Blocked);
    if (Key.IsEmpty() || Blocked || !Marker || !Marker->bSignupUncertain || Marker->ProjectHash!=Project(Key)) return false;
    auto* Record=NewObject<UHorizonExampleIdentity>(); Record->UserId=Issued.UserId; Record->AnonymousToken=Issued.AnonymousToken;
    Record->DisplayName=Issued.DisplayName; Record->bAnonymous=Issued.bIsAnonymous;
    // Provenance comes exclusively from this request's response, never from the mutable shared cache.
    return Record->Valid() && Record->bAnonymous && Record->Backup() && SaveGuest(Key,Record);
}
bool UHorizonExampleIdentity::ShouldRestoreSession(UHorizonSubsystem* SDK)
{
    if (ShouldRestoreGuest(SDK)) return true;
    auto* Current=Cache(); if (!Current) return UHorizonSessionSave::HasSavedSession();
    bool Blocked; auto* Binding=Read(HorizonExampleIdentityStorage::Binding,Blocked);
    return Blocked || !(Binding && Binding->CacheHash()==Current->CacheHash() && Binding->ProjectHash!=Project(ProjectKey(SDK)));
}
bool UHorizonExampleIdentity::BeforeRestore(UHorizonSubsystem* SDK,const FString& Key,bool GuestOnly,UHorizonExampleIdentity*& Original)
{
    if (Key.IsEmpty() || ProjectKey(SDK)!=Key) return false;
    bool Blocked; auto* Guest=Read(Slot(Key),Blocked); if (Blocked) return false;
    // A cache found on a later launch is unbound until authentication proves its project.
    // Only the original live signup callback may capture newly issued credentials before sign-in.
    if (Guest && Guest->bSignupUncertain && !UHorizonSessionSave::HasSavedSession()) return false;
    if (Guest && !Guest->bSignupUncertain && (GuestOnly || !UHorizonSessionSave::HasSavedSession()))
    {
        if (Blocked || !Guest || Guest->ProjectHash!=Project(Key) || Guest->bSignupUncertain || !Guest->Stage(true)) return false;
        Original=Guest; return true;
    }
    auto* Current=Cache(); if (!Current) return false;
    auto* Binding=Read(HorizonExampleIdentityStorage::Binding,Blocked); if (Blocked) return false;
    if (Binding && Binding->CacheHash()==Current->CacheHash())
    { if (Binding->ProjectHash!=Project(Key)) return false; Current->ProjectHash=Binding->ProjectHash; }
    // Unbound legacy credentials are tried without assigning ownership; only successful authentication binds them.
    Original=Current; return Current->Backup();
}
bool UHorizonExampleIdentity::AfterAuth(UHorizonSubsystem* SDK,const FString& Key,UHorizonExampleIdentity* Original,bool SignupSent,bool Restoring,bool Success)
{
    if (!SDK || !SDK->Auth) return false;
    if (!Success)
    { if (Original) Original->Stage(); return false; }
    const auto& User=SDK->Auth->GetCurrentUser();
    auto* Current=NewObject<UHorizonExampleIdentity>(); Current->UserId=User.UserId; Current->AccessToken=User.AccessToken;
    Current->AnonymousToken=User.AnonymousToken; Current->DisplayName=User.DisplayName; Current->bAnonymous=User.bIsAnonymous;
    if (!Current->Valid() || ProjectKey(SDK)!=Key || Restoring && Original && !Original->UserId.IsEmpty() && Current->UserId!=Original->UserId)
    { SDK->Auth->SignOut(); if (Original) Original->Stage(); return false; }
    auto* Scoped=Bound(Key,Current);
    if (!Scoped || !Current->Backup() || Current->bAnonymous && !SaveGuest(Key,Current) || !Scoped->Store(HorizonExampleIdentityStorage::Binding))
    { SDK->Auth->SignOut(); if (Original) Original->Stage(); return false; }
    return true;
}
