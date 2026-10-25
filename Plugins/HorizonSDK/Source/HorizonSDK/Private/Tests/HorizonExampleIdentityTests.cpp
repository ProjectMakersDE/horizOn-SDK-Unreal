// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Examples/HorizonExampleIdentity.h"
#include "Examples/HorizonStatisticsExample.h"
#include "HorizonSubsystem.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "HorizonConfig.h"
#include "HorizonSessionSave.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"

struct FHorizonIdentityNative : TSharedFromThis<FHorizonIdentityNative>
{
    FAutomationTestBase* Test; UGameInstance* Instance; UHorizonSubsystem* SDK; AHorizonStatisticsExample* Actor=nullptr;
    TArray<FString> Hosts,Slots; FString Key; int32 Retries; float Delay; int32 Stage=0; bool Waiting=false; double WaitUntil=0,Deadline=FPlatformTime::Seconds()+30;
    TMap<FString,TArray<uint8>> Preserved;
    TArray<UObject*> Roots;
    const FString A=TEXT("00000000-0000-4000-8000-000000001005"),E=TEXT("00000000-0000-4000-8000-000000002005");
    const FString PA=TEXT("identity-issued-1005"),PB=TEXT("identity-foreign-1005"),PC=TEXT("identity-legacy-1005"),PU=TEXT("identity-unknown-1005"),PInline=TEXT("identity-inline-1005"),PJ=TEXT("identity-preservation-failed-1005"),PK=TEXT("identity-context-1005");
    UHorizonExampleIdentity* Original=nullptr;
    FHorizonIdentityNative(FAutomationTestBase* In,const FString& Host):Test(In)
    {
        auto* C=GetMutableDefault<UHorizonConfig>(); Hosts=C->Hosts; Key=C->ApiKey; Retries=C->MaxRetryAttempts; Delay=C->RetryDelaySeconds;
        C->Hosts={Host}; C->ApiKey=PA; C->MaxRetryAttempts=3; C->RetryDelaySeconds=.05f;
        Instance=NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone(); SDK=Instance->GetSubsystem<UHorizonSubsystem>();
        for (const FString& S:{UHorizonSessionSave::SlotName,FString(TEXT("horizon_example_session_binding_v1")),UHorizonExampleIdentity::Slot(PA),UHorizonExampleIdentity::Slot(PB),UHorizonExampleIdentity::Slot(PC),UHorizonExampleIdentity::Slot(PU),UHorizonExampleIdentity::Slot(PInline),UHorizonExampleIdentity::Slot(PJ),UHorizonExampleIdentity::Slot(PK)}) Keep(S);
        Keep(TEXT("horizon-statistics-")+FMD5::HashAnsiString(*(PA+TEXT(":")+A)));
        UHorizonSessionSave::DeleteFromDisk(); UGameplayStatics::DeleteGameInSlot(TEXT("horizon_example_session_binding_v1"),0);
        for (const FString& P:{PA,PB,PC,PU,PInline,PJ,PK}) UGameplayStatics::DeleteGameInSlot(UHorizonExampleIdentity::Slot(P),0);
    }
    void Keep(const FString& Slot)
    {
        if (Slots.Contains(Slot)) return; Slots.Add(Slot);
        if (UGameplayStatics::DoesSaveGameExist(Slot,0)) { TArray<uint8> Bytes; if (UGameplayStatics::LoadDataFromSlot(Bytes,Slot,0)) Preserved.Add(Slot,Bytes); else Test->AddError(TEXT("Cannot preserve preexisting fixture slot")); }
    }
    void Track(const UHorizonExampleIdentity* R) { if (R) Keep(TEXT("horizon_example_legacy_v1_")+R->CacheHash()); }
    void Project(const FString& P) { GetMutableDefault<UHorizonConfig>()->ApiKey=P; SDK->GetHttpClient()->Initialize(); }
    void Cache(const FString& User,const FString& Access,const FString& Token,bool Anonymous)
    { auto* S=NewObject<UHorizonSessionSave>(); S->CachedUserId=User; S->CachedAccessToken=Access; S->CachedAnonymousToken=Token; S->CachedDisplayName=TEXT("Fixture"); S->bIsAnonymous=Anonymous; Test->TestTrue(TEXT("Artificial auth cache saved"),S->SaveToDisk()); }
    void Capture() { Test->TestTrue(TEXT("Exact current identity backed up"),UHorizonExampleIdentity::CaptureBeforeAuth(SDK,Original)); Track(Original); if (Original && !Roots.Contains(Original)) { Original->AddToRoot(); Roots.Add(Original); } }
    void ConnectedActor()
    { Actor=Instance->GetWorld()->SpawnActor<AHorizonStatisticsExample>(); Actor->Connected(); }
    void Next() { ++Stage; Waiting=false; }
    FOnAuthComplete Callback(TFunction<void(bool)> Check)
    { const TWeakPtr<FHorizonIdentityNative> Weak=AsShared(); return FOnAuthComplete::CreateLambda([Weak,Check](bool Ok) { if (auto Self=Weak.Pin()) { Check(Ok); Self->Next(); } }); }
    void Cleanup()
    {
        SDK->Auth->SignOut(); SDK->GetHttpClient()->Disconnect();
        UWorld* W=Instance->GetWorld(); Instance->Shutdown(); W->DestroyWorld(false); GEngine->DestroyWorldContext(W); Instance->RemoveFromRoot();
        for (const FString& S:Slots) { if (auto* Bytes=Preserved.Find(S)) Test->TestTrue(TEXT("Previous local data restored byte-for-byte"),UGameplayStatics::SaveDataToSlot(*Bytes,S,0)); else if (UGameplayStatics::DoesSaveGameExist(S,0)) UGameplayStatics::DeleteGameInSlot(S,0); }
        for (auto* R:Roots) R->RemoveFromRoot();
        auto* C=GetMutableDefault<UHorizonConfig>(); C->Hosts=Hosts; C->ApiKey=Key; C->MaxRetryAttempts=Retries; C->RetryDelaySeconds=Delay;
    }
    bool Update()
    {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Identity timeout stage %d"),Stage)); Cleanup(); return true; }
        if (Waiting) { if (WaitUntil && FPlatformTime::Seconds()>WaitUntil) { WaitUntil=0; Next(); } return false; }
        Waiting=true; const TWeakPtr<FHorizonIdentityNative> Weak=AsShared();
        if (Stage==0)
        {
            // A restore before host selection must fail at once and keep the cached session for the real restore.
            Cache(A,TEXT("cached-access"),TEXT("cached-token"),true); bool Called=false,Restored=true;
            SDK->Auth->RestoreAnonymousSession(FOnAuthComplete::CreateLambda([&Called,&Restored](bool Ok) { Called=true; Restored=Ok; }));
            Test->TestTrue(TEXT("Restore without a host fails immediately"),Called && !Restored); Test->TestFalse(TEXT("No provisional user without a host"),SDK->Auth->IsSignedIn());
            auto* Kept=UHorizonSessionSave::LoadFromDisk(); Test->TestTrue(TEXT("Cached session survives a restore without a host"),Kept && Kept->CachedUserId==A && Kept->CachedAccessToken==TEXT("cached-access"));
            UHorizonSessionSave::DeleteFromDisk();
            SDK->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Loopback connected"),Ok); Self->Next(); } }));
        }
        else if (Stage==1)
        {
            Capture(); Test->TestTrue(TEXT("Uncertain signup marker persisted first"),UHorizonExampleIdentity::BeforeSignup(SDK,PA));
            SDK->Auth->SignUpAnonymous(TEXT("Fixture"),Callback([this](bool Ok) {
                Test->TestFalse(TEXT("Issued signup followed by rejected sign-in"),Ok);
                UHorizonExampleIdentity::AfterAuth(SDK,PA,Original,true,false,Ok);
                auto* Saved=Cast<UHorizonExampleIdentity>(UGameplayStatics::LoadGameFromSlot(UHorizonExampleIdentity::Slot(PA),0));
                Test->TestTrue(TEXT("Actual response credential survives failed follow-up"),Saved && !Saved->bSignupUncertain && Saved->UserId==A && Saved->AnonymousToken==TEXT("issued-token-1005"));
            }),TEXT(""),FOnAnonymousIdentityIssued::CreateLambda([this](const FString& P,const FHorizonUserData& U) {
                Test->TestEqual(TEXT("Immutable issuing project"),P,PA); Test->TestEqual(TEXT("Immutable issued user"),U.UserId,A);
                // A mutable unrelated cache cannot become the issued identity.
                Cache(E,TEXT("email-cache"),TEXT("unrelated-token"),false);
                auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; Track(R);
                return UHorizonExampleIdentity::RecordIssuedSignup(P,U);
            }));
        }
        else if (Stage==2)
        {
            Capture(); SDK->Auth->SignInEmail(TEXT("fixture@example.invalid"),TEXT("synthetic"),Callback([this](bool Ok) {
                Test->TestTrue(TEXT("Email session authenticated"),Ok); auto U=SDK->Auth->GetCurrentUser();
                Test->TestTrue(TEXT("Email may retain old anonymous token without changing type"),!U.bIsAnonymous && !U.AnonymousToken.IsEmpty());
                auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AccessToken=U.AccessToken; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; R->bAnonymous=false; Track(R);
                Test->TestTrue(TEXT("Successful email identity bound only after auth"),UHorizonExampleIdentity::AfterAuth(SDK,PA,Original,false,false,Ok));
                Test->TestTrue(TEXT("Explicit Guest overrides the actual email cache"),UHorizonExampleIdentity::BeforeRestore(SDK,PA,true,Original));
                Test->TestEqual(TEXT("Guest selection is original guest, never cached email"),UHorizonSessionSave::LoadFromDisk()->CachedUserId,A);
                Cache(E,TEXT("email-access"),TEXT("issued-token-1005"),false);
            }));
        }
        else if (Stage==3)
        {
            Project(PB); Capture(); Test->TestFalse(TEXT("Known foreign cache does not force restore"),UHorizonExampleIdentity::ShouldRestoreSession(SDK));
            Test->TestTrue(TEXT("Independent new project may start once"),UHorizonExampleIdentity::BeforeSignup(SDK,PB));
            SDK->Auth->SignUpAnonymous(TEXT("Foreign"),Callback([this](bool Ok) {
                Test->TestFalse(TEXT("Independent project follow-up sign-in fails without losing issued identity"),Ok);
                UHorizonExampleIdentity::AfterAuth(SDK,PB,Original,true,false,Ok);
                auto* G=Cast<UHorizonExampleIdentity>(UGameplayStatics::LoadGameFromSlot(UHorizonExampleIdentity::Slot(PB),0));
                auto* Previous=Cast<UHorizonExampleIdentity>(UGameplayStatics::LoadGameFromSlot(TEXT("horizon_example_legacy_v1_")+Original->CacheHash(),0));
                Test->TestTrue(TEXT("Both project-issued guest and prior email identity are preserved"),G && !G->bSignupUncertain && G->UserId!=A && Previous && Previous->UserId==E && !Previous->bAnonymous);
            }),TEXT(""),FOnAnonymousIdentityIssued::CreateLambda([this](const FString& P,const FHorizonUserData& U) { auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; Track(R); return UHorizonExampleIdentity::RecordIssuedSignup(P,U); }));
        }
        else if (Stage==4)
        {
            Project(PA); Capture();
            Test->TestTrue(TEXT("Explicit Guest selects protected original over email"),UHorizonExampleIdentity::BeforeRestore(SDK,PA,true,Original));
            Test->TestEqual(TEXT("Guest staged with original user"),UHorizonSessionSave::LoadFromDisk()->CachedUserId,A);
            SDK->Auth->RestoreAnonymousSession(Callback([this](bool Ok) {
                Test->TestTrue(TEXT("Protected guest restored"),Ok); Test->TestTrue(TEXT("Restored original accepted"),UHorizonExampleIdentity::AfterAuth(SDK,PA,Original,false,true,Ok));
                auto U=SDK->Auth->GetCurrentUser(); auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AccessToken=U.AccessToken; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; Track(R);
            }));
        }
        else if (Stage==5)
        {
            SDK->Auth->SignOut(); ConnectedActor();
            // Restore populates CurrentUser before its delayed check-auth completes.
            Test->TestTrue(TEXT("SDK has provisional user during auth verification"),SDK->Auth->IsSignedIn());
            Actor->Connected();
            WaitUntil=FPlatformTime::Seconds()+1.2;
        }
        else if (Stage==6)
        {
            SDK->Auth->SignOut(); Project(PC); Cache(A,TEXT(""),TEXT("legacy-original-token-1005"),true); Capture();
            Test->TestTrue(TEXT("Unbound legacy credential may be tried without project binding"),UHorizonExampleIdentity::BeforeRestore(SDK,PC,true,Original));
            SDK->Auth->RestoreAnonymousSession(Callback([this](bool Ok) {
                Test->TestFalse(TEXT("Wrong-project legacy restore rejected"),Ok); UHorizonExampleIdentity::AfterAuth(SDK,PC,Original,false,true,Ok);
                Test->TestFalse(TEXT("Rejected legacy token never poisons project guest slot"),UGameplayStatics::DoesSaveGameExist(UHorizonExampleIdentity::Slot(PC),0));
                Test->TestFalse(TEXT("Uncertainty never permits replacement signup"),UHorizonExampleIdentity::BeforeSignup(SDK,PC));
            }));
        }
        else if (Stage==7)
        {
            SDK->Auth->SignOut(); UHorizonSessionSave::DeleteFromDisk(); Project(PU); ConnectedActor();
            Actor->Connected(); WaitUntil=FPlatformTime::Seconds()+.6;
        }
        else if (Stage==8)
        {
            auto* Marker=Cast<UHorizonExampleIdentity>(UGameplayStatics::LoadGameFromSlot(UHorizonExampleIdentity::Slot(PU),0));
            Test->TestTrue(TEXT("Lost signup remains durably uncertain"),Marker && Marker->bSignupUncertain);
            Actor->Destroy(); ConnectedActor(); WaitUntil=FPlatformTime::Seconds()+.6;
        }
        else if (Stage==9)
        {
            Test->TestFalse(TEXT("Recreated sample cannot invent replacement signup"),SDK->Auth->IsSignedIn());
            Cache(A,TEXT(""),TEXT("legacy-original-token-1005"),true); Capture();
            Test->TestTrue(TEXT("Restart uncertainty may try unbound original without assigning project"),UHorizonExampleIdentity::BeforeRestore(SDK,PU,true,Original));
            SDK->Auth->RestoreAnonymousSession(Callback([this](bool Ok) {
                Test->TestFalse(TEXT("Unbound restore fails independently of marker"),Ok); UHorizonExampleIdentity::AfterAuth(SDK,PU,Original,false,true,Ok);
                auto* M=Cast<UHorizonExampleIdentity>(UGameplayStatics::LoadGameFromSlot(UHorizonExampleIdentity::Slot(PU),0));
                Test->TestTrue(TEXT("Failed legacy restore cannot replace uncertainty marker"),M && M->bSignupUncertain);
            }));
        }
        else if (Stage==10)
        {
            SDK->Auth->SignOut(); UHorizonSessionSave::DeleteFromDisk(); Project(PInline); Capture();
            Test->TestTrue(TEXT("Inline signup protected before HTTP"),UHorizonExampleIdentity::BeforeSignup(SDK,PInline));
            SDK->Auth->SignUpAnonymous(TEXT("Inline"),Callback([this](bool Ok) {
                Test->TestTrue(TEXT("Inline access token accepted without follow-up request"),Ok);
                auto U=SDK->Auth->GetCurrentUser(); auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AccessToken=U.AccessToken; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; Track(R);
                Test->TestTrue(TEXT("Inline identity validated and durably bound"),UHorizonExampleIdentity::AfterAuth(SDK,PInline,Original,true,false,Ok));
            }),TEXT(""),FOnAnonymousIdentityIssued::CreateLambda([this](const FString& P,const FHorizonUserData& U) { auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; Track(R); return UHorizonExampleIdentity::RecordIssuedSignup(P,U); }));
        }
        else if (Stage==11)
        {
            SDK->Auth->SignOut(); Project(PJ); Capture(); Test->TestTrue(TEXT("Preservation-failure signup marker ready"),UHorizonExampleIdentity::BeforeSignup(SDK,PJ));
            SDK->Auth->SignUpAnonymous(TEXT("PreservationFailure"),Callback([this](bool Ok) {
                Test->TestFalse(TEXT("Rejected preservation callback prevents follow-up sign-in"),Ok);
                Test->TestTrue(TEXT("Core still retains actual issued credential"),UHorizonSessionSave::LoadFromDisk()->CachedAnonymousToken==TEXT("preservation-token"));
            }),TEXT(""),FOnAnonymousIdentityIssued::CreateLambda([](const FString&,const FHorizonUserData&) { return false; }));
        }
        else if (Stage==12)
        {
            SDK->Auth->SignOut(); Capture(); UHorizonSessionSave::DeleteFromDisk(); Project(PK); Capture();
            Test->TestTrue(TEXT("Context-change signup protected first"),UHorizonExampleIdentity::BeforeSignup(SDK,PK));
            SDK->Auth->SignUpAnonymous(TEXT("Context"),Callback([this](bool Ok) { Test->TestFalse(TEXT("Observer context ABA stops follow-up authentication"),Ok); }),TEXT(""),
                FOnAnonymousIdentityIssued::CreateLambda([this](const FString& P,const FHorizonUserData& U) {
                    auto* R=NewObject<UHorizonExampleIdentity>(); R->UserId=U.UserId; R->AnonymousToken=U.AnonymousToken; R->DisplayName=U.DisplayName; Track(R);
                    const bool Saved=UHorizonExampleIdentity::RecordIssuedSignup(P,U); SDK->GetHttpClient()->Initialize(); return Saved;
                }));
        }
        else
        {
            const FString StatsSlot=TEXT("horizon-statistics-")+FMD5::HashAnsiString(*(PA+TEXT(":")+A));
            auto* Earlier=Cast<UHorizonStatisticsPendingBatch>(UGameplayStatics::LoadGameFromSlot(StatsSlot,0));
            Test->TestNotNull(TEXT("Sample preserved its expired original batch"),Earlier);
            if (Earlier)
            {
                auto* Late=Instance->GetWorld()->SpawnActor<AHorizonStatisticsExample>(); Late->Pending=Earlier; Late->Slot=StatsSlot;
                auto* Later=NewObject<UHorizonStatisticsPendingBatch>(); Later->UserId=Earlier->UserId; Later->OperationId=TEXT("1792123456789:10000000-0000-4000-8000-000000001005"); Later->Updates=Earlier->Updates;
                Test->TestTrue(TEXT("Newer artificial batch saved"),UGameplayStatics::SaveGameToSlot(Later,StatsSlot,0));
                Late->Applied(true,FHorizonStatisticsSnapshot{},TEXT(""),TEXT(""));
                auto* Retained=Cast<UHorizonStatisticsPendingBatch>(UGameplayStatics::LoadGameFromSlot(StatsSlot,0));
                Test->TestTrue(TEXT("Late receipt cannot delete a different actor's later original"),Retained && Retained->OperationId==Later->OperationId);
            }
            Cleanup(); return true;
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FHorizonIdentityCommand,TSharedPtr<FHorizonIdentityNative>,State);
bool FHorizonIdentityCommand::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonIdentityTest,"Horizon.Identity.NativeRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonIdentityTest::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_IDENTITY_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Optional identity fixture not configured")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(FHorizonIdentityCommand(MakeShared<FHorizonIdentityNative>(this,Host))); return true;
}
#endif
