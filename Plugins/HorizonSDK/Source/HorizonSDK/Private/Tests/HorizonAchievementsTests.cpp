#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonAchievementsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Managers/HorizonStatisticsManager.h"
#include "Managers/HorizonRealtimeManager.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

struct FHorizonAchievementsNative : TSharedFromThis<FHorizonAchievementsNative>
{
    FAutomationTestBase* Test;
    UHorizonHttpClient* Http;
    UHorizonAuthManager* Auth;
    UHorizonAchievementsManager* Achievements;
    UHorizonPlayerValuesManager* Values;
    UHorizonStatisticsManager* Statistics;
    TArray<FString> OriginalHosts;
    FString OriginalKey,VersionA,VersionG;
    int32 OriginalRetries;
    float OriginalDelay;
    int32 Stage=0,RaceCallbacks=0;
    bool bPending=false,bNewDone=false;
    double Deadline=FPlatformTime::Seconds()+60;
    const FString User=TEXT("00000000-0000-4000-8000-000000001008");
    FHorizonAchievementsNative(FAutomationTestBase* InTest,const FString& Host):Test(InTest)
    {
        auto* Config=GetMutableDefault<UHorizonConfig>();
        OriginalHosts=Config->Hosts; OriginalKey=Config->ApiKey; OriginalRetries=Config->MaxRetryAttempts; OriginalDelay=Config->RetryDelaySeconds;
        Config->Hosts={Host}; Config->ApiKey=TEXT("fixture-key-1008"); Config->MaxRetryAttempts=2; Config->RetryDelaySeconds=.1f;
        Http=NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth=NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        Values=NewObject<UHorizonPlayerValuesManager>(); Values->AddToRoot(); Values->Initialize(Http,Auth);
        Achievements=NewObject<UHorizonAchievementsManager>(); Achievements->AddToRoot(); Achievements->Initialize(Http,Auth,Values);
        Statistics=NewObject<UHorizonStatisticsManager>(); Statistics->AddToRoot(); Statistics->Initialize(Http,Auth,Values,Achievements);
    }
    void Cleanup()
    {
        Auth->SignOut(); Http->Disconnect(); Statistics->RemoveFromRoot(); Values->RemoveFromRoot(); Achievements->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
        auto* Config=GetMutableDefault<UHorizonConfig>(); Config->Hosts=OriginalHosts; Config->ApiKey=OriginalKey; Config->MaxRetryAttempts=OriginalRetries; Config->RetryDelaySeconds=OriginalDelay;
    }
    void Next() { ++Stage; bPending=false; }
    FOnAchievementsComplete ReadCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonAchievementsNative> Weak=AsShared();
        return FOnAchievementsComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonAchievementsSnapshot& S,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                const bool Expected=Sent==2 || Sent==3 || Sent==4 || Sent==5 || Sent==6 || Sent==13 || Sent==15 || Sent==18 || Sent==20 || Sent==22 || Sent==25 || Sent==30 || Sent==34 || Sent==35;
                const FString Error=Expected?TEXT(""):(Sent==17 || Sent==26 || Sent==27)?TEXT("INVALID_RESPONSE"):TEXT("STALE_RESPONSE");
                Self->Test->TestEqual(FString::Printf(TEXT("Achievement read stage %d"),Sent),Ok,Expected);
                Self->Test->TestEqual(TEXT("Stable snapshot error"),Code,Error);
                if (Sent==2) Self->VersionA=S.DefinitionVersion;
                if (Sent==15) Self->VersionG=S.DefinitionVersion;
                if (Sent==3) Self->Test->TestTrue(TEXT("Null metadata materializes only matching cached version"),!S.bDefinitionsIncluded && S.Definitions.Num()==1);
                if (Sent==4) Self->Test->TestTrue(TEXT("Missing materialization causes exactly one full GET"),S.bDefinitionsIncluded && S.Definitions.Num()==1);
                if (Sent==5 || Sent==6) Self->Test->TestTrue(TEXT("Empty definitions replace, then remain empty on null"),S.Definitions.IsEmpty());
                if (Sent==11 || Sent==14) Self->Test->TestEqual(TEXT("Revision/time regression preserves accepted snapshot"),Self->Achievements->GetCurrentAchievements().Revision,int64(4));
                if (Sent==12 || Sent==16 || Sent==17 || Sent==19 || Sent==21 || Sent==23) Self->Test->TestFalse(TEXT("Invalidated reply cannot refill cache"),Self->Achievements->GetCurrentAchievements().bPresent);
                if (Sent==26 || Sent==27) Self->Test->TestEqual(TEXT("Malformed token/type preserves current snapshot"),Self->Achievements->GetCurrentAchievements().Revision,int64(8));
                if (Sent==34 || Sent==35) Self->Test->TestTrue(TEXT("Admin raised capacity and retained rows after lower limits"),S.Progress.Num()==701 && S.Definitions.Num()==701 && S.Progress[0].UnlockedAt.IsEmpty());
                Self->Next();
            }
        });
    }
    FOnAchievementUnlockComplete UnlockCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonAchievementsNative> Weak=AsShared();
        return FOnAchievementUnlockComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonAchievementUnlockResponse& R,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                Self->Test->TestEqual(TEXT("Manual unlock result"),Ok,Sent!=28);
                Self->Test->TestEqual(TEXT("Feature rejection stays terminal"),Code,Sent==28?FString(TEXT("ACHIEVEMENT_NOT_MANUAL")):FString());
                if (Ok)
                {
                    Self->Test->TestEqual(TEXT("Duplicate/lost reply exposes no second unlock"),R.bReplayed,Sent==8);
                    Self->Test->TestEqual(TEXT("New unlock notification count"),R.NewlyUnlocked.Num(),Sent==8?0:1);
                    const auto* P=R.Achievements.Progress.FindByPredicate([](const FHorizonAchievementProgress& V) { return V.Key==TEXT("welcome"); });
                    Self->Test->TestTrue(TEXT("Full Inbox retains original accepted reward"),P && P->RewardState==TEXT("pending") && P->PendingReward.Values.Num()==1 && P->PendingReward.Values[0].Amount==7);
                    Self->Test->TestEqual(TEXT("Per-key full Inbox outcome"),R.DeliveryResults.Num()?R.DeliveryResults[0].Code:FString(),FString(TEXT("INBOX_FULL")));
                    if (Sent==8)
                    {
                        const auto* D=R.Achievements.Definitions.FindByPredicate([](const FHorizonAchievementDefinition& V) { return V.Key==TEXT("welcome"); });
                        Self->Test->TestTrue(TEXT("Catalogue edit is separate from original pending values"),D && D->Rewards.Values.Num()==1 && D->Rewards.Values[0].Amount==99);
                    }
                }
                Self->Next();
            }
        });
    }
    FOnAchievementDeliveryComplete DeliveryCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonAchievementsNative> Weak=AsShared();
        return FOnAchievementDeliveryComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonAchievementDeliveryResponse& R,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                Self->Test->TestEqual(TEXT("Explicit pending delivery result"),Ok,Sent!=29);
                Self->Test->TestEqual(TEXT("Conflict retains concrete code and original keys"),Code,Sent==29?FString(TEXT("STATE_CONFLICT")):FString());
                if (Ok)
                {
                    Self->Test->TestEqual(TEXT("Per-key results preserved"),R.Results.Num(),2);
                    Self->Test->TestEqual(TEXT("Second original key outcome"),R.Results.Num()==2?R.Results[1].State:FString(),Sent==9?FString(TEXT("pending")):FString(TEXT("delivered")));
                    if (Sent==9) Self->Test->TestEqual(TEXT("Partial INBOX_FULL stays pending"),R.Results[1].Code,FString(TEXT("INBOX_FULL")));
                }
                Self->Next();
            }
        });
    }
    bool Update()
    {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Achievements timeout stage %d"),Stage)); Cleanup(); return true; }
        if (bPending) return false;
        bPending=true; const TWeakPtr<FHorizonAchievementsNative> Weak=AsShared();
        if (Stage==0) Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Loopback connected"),Ok); Self->Next(); } }));
        else if (Stage==1 || Stage==24) Auth->SignInAnonymous(TEXT("fixture-anonymous-1008"),FOnAuthComplete::CreateLambda([Weak](bool Ok) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Original identity restored"),Ok); Self->Next(); } }));
        else if (Stage==7 || Stage==8 || Stage==28) Achievements->Unlock(User,Stage==28?TEXT("statistic"):TEXT("welcome"),UnlockCallback());
        else if (Stage==9 || Stage==10 || Stage==29) Achievements->DeliverRewards(User,{TEXT("welcome"),TEXT("second")},DeliveryCallback());
        else if (Stage==4) { Achievements->ClearCache(); Achievements->GetAchievements(User,ReadCallback(),VersionA); }
        else if (Stage==12) { Achievements->ClearCache(); Achievements->GetAchievements(User,ReadCallback()); }
        else if (Stage==16) { Achievements->GetAchievements(User,ReadCallback()); Achievements->ClearCache(); }
        else if (Stage==17) Achievements->GetAchievements(User,ReadCallback(),VersionG);
        else if (Stage==19) { Achievements->GetAchievements(User,ReadCallback()); Http->SetSessionToken(TEXT("other")); Http->SetSessionToken(TEXT("fixture-session-1008")); }
        else if (Stage==21)
        {
            Achievements->GetAchievements(User,ReadCallback()); auto* Config=GetMutableDefault<UHorizonConfig>();
            Config->ApiKey=TEXT("other"); Http->Initialize(); Config->ApiKey=TEXT("fixture-key-1008"); Http->Initialize();
        }
        else if (Stage==23) { Achievements->GetAchievements(User,ReadCallback()); Auth->SignOut(); }
        else if (Stage==30)
        {
            Achievements->GetAchievements(User,ReadCallback()); const auto Rejected=MakeShared<bool>(false);
            Achievements->GetAchievements(User,FOnAchievementsComplete::CreateLambda([Rejected](bool Ok,const FHorizonAchievementsSnapshot&,const FString& Code,const FString&) { *Rejected=!Ok && Code==TEXT("REQUEST_IN_PROGRESS"); }));
            Test->TestTrue(TEXT("Overlapping read bounded locally"),*Rejected);
        }
        else if (Stage==31)
        {
            Achievements->GetAchievements(User,FOnAchievementsComplete::CreateLambda([Weak](bool Ok,const FHorizonAchievementsSnapshot&,const FString& Code,const FString&)
            {
                if (auto Self=Weak.Pin())
                {
                    Self->Test->TestTrue(TEXT("Cleared old ticket rejected"),!Ok && Code==TEXT("STALE_RESPONSE"));
                    if (!Self->bNewDone)
                    {
                        const auto Rejected=MakeShared<bool>(false);
                        Self->Achievements->GetAchievements(Self->User,FOnAchievementsComplete::CreateLambda([Rejected](bool Success,const FHorizonAchievementsSnapshot&,const FString& Error,const FString&) { *Rejected=!Success && Error==TEXT("REQUEST_IN_PROGRESS"); }));
                        Self->Test->TestTrue(TEXT("Old callback cannot release newer active slot"),*Rejected);
                    }
                    else Self->Test->TestEqual(TEXT("Old callback cannot erase new snapshot"),Self->Achievements->GetCurrentAchievements().Revision,int64(11));
                    if (++Self->RaceCallbacks==2) Self->Next();
                }
            }));
            Achievements->ClearCache();
            Achievements->GetAchievements(User,FOnAchievementsComplete::CreateLambda([Weak](bool Ok,const FHorizonAchievementsSnapshot& S,const FString& Code,const FString&)
            {
                if (auto Self=Weak.Pin())
                {
                    Self->bNewDone=true; Self->Test->TestTrue(TEXT("New cache generation accepted"),Ok && Code.IsEmpty() && S.Revision==11);
                    if (++Self->RaceCallbacks==2) Self->Next();
                }
            }));
        }
        else if (Stage==32 || Stage==33)
        {
            const int64 ValuesEpoch=Values->GetCacheEpoch(); const int32 Sent=Stage;
            Achievements->Unlock(User,TEXT("welcome"),FOnAchievementUnlockComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonAchievementUnlockResponse& R,const FString& Code,const FString&)
            {
                if (auto Self=Weak.Pin())
                {
                    Self->Test->TestTrue(TEXT("Compact unlock and replay accepted without delivery"),Ok && Code.IsEmpty() && R.DeliveryResults.IsEmpty() && R.bReplayed==(Sent==33));
                    Self->Test->TestTrue(TEXT("Compact progress has no timestamp or reward payload"),R.Achievements.Progress.Num()==1 && R.Achievements.Progress[0].RewardState==TEXT("complete") && R.Achievements.Progress[0].UnlockedAt.IsEmpty() && !R.Achievements.Progress[0].PendingReward.bPresent);
                    Self->Next();
                }
            }));
            Test->TestTrue(TEXT("Value cache invalidated before unlock reply"),Values->GetCacheEpoch()>ValuesEpoch);
        }
        else if (Stage==36)
        {
            const int64 ValuesEpoch=Values->GetCacheEpoch();
            Test->TestTrue(TEXT("Achievement snapshot populated before uncertain statistics mutation"),Achievements->GetCurrentAchievements().bPresent);
            FHorizonStatisticUpdate Update; Update.Key=TEXT("wins"); Update.Value=1;
            Statistics->UpdateStatistics({Update},TEXT("1792195200000:00000000-0000-4000-8000-000000001008"),FOnStatisticsLoaded::CreateLambda([Weak](bool Ok,const FHorizonStatisticsSnapshot&,const FString& Code,const FString&)
            { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Uncertain statistics reply rejected"),!Ok && Code==TEXT("INVALID_RESPONSE")); Self->Next(); } }));
            Test->TestTrue(TEXT("Statistics clears reward and achievement caches before HTTP completes"),Values->GetCacheEpoch()>ValuesEpoch && !Achievements->GetCurrentAchievements().bPresent);
        }
        else if (Stage<37) Achievements->GetAchievements(User,ReadCallback());
        else
        {
            const auto Wrong=MakeShared<bool>(false);
            Achievements->GetAchievements(TEXT("00000000-0000-4000-8000-000000009999"),FOnAchievementsComplete::CreateLambda([Wrong](bool Ok,const FHorizonAchievementsSnapshot&,const FString& Code,const FString&) { *Wrong=!Ok && Code==TEXT("SESSION_FORBIDDEN"); }));
            Test->TestTrue(TEXT("Foreign user rejected before HTTP"),*Wrong);
            for (const TArray<FString>& Keys:{TArray<FString>{},TArray<FString>{TEXT("welcome"),TEXT("welcome")},TArray<FString>{TEXT("bad key")},TArray<FString>{TEXT("a"),TEXT("b"),TEXT("c"),TEXT("d"),TEXT("e"),TEXT("f"),TEXT("g"),TEXT("h"),TEXT("i")}})
            {
                const auto Rejected=MakeShared<bool>(false);
                Achievements->DeliverRewards(User,Keys,FOnAchievementDeliveryComplete::CreateLambda([Rejected](bool Ok,const FHorizonAchievementDeliveryResponse&,const FString& Code,const FString&) { *Rejected=!Ok && Code==TEXT("INVALID_REQUEST"); }));
                Test->TestTrue(TEXT("Only 1..8 unique original keys may be sent"),*Rejected);
            }
            Auth->SignOut(); const auto SignedOut=MakeShared<bool>(false);
            Achievements->GetAchievements(User,FOnAchievementsComplete::CreateLambda([SignedOut](bool Ok,const FHorizonAchievementsSnapshot&,const FString& Code,const FString&) { *SignedOut=!Ok && Code==TEXT("SESSION_REQUIRED"); }));
            Test->TestTrue(TEXT("Signout stays local and clears metadata"),*SignedOut && !Achievements->GetCurrentAchievements().bPresent);
            Cleanup(); return true;
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonAchievements,TSharedPtr<FHorizonAchievementsNative>,State);
bool FWaitForHorizonAchievements::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonAchievementsTransportTest,"Horizon.Achievements.NativeTransport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonAchievementsTransportTest::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_ACHIEVEMENTS_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set loopback HORIZON_ACHIEVEMENTS_FIXTURE_URL for native proof")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only artificial loopback accepted")); return false; }
    TestNotNull(TEXT("Subsystem Achievements property"),UHorizonSubsystem::StaticClass()->FindPropertyByName(TEXT("Achievements")));
    TestNotNull(TEXT("Blueprint snapshot"),UHorizonAchievementsManager::StaticClass()->FindFunctionByName(TEXT("RequestAchievements")));
    TestNotNull(TEXT("Blueprint delivery"),UHorizonAchievementsManager::StaticClass()->FindFunctionByName(TEXT("DeliverAchievementRewards")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonAchievements(MakeShared<FHorizonAchievementsNative>(this,Host)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonAchievementsModelTest,"Horizon.Achievements.StrictModels",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonAchievementsModelTest::RunTest(const FString&)
{
    const FString Template=TEXT("{\"userId\": \"00000000-0000-4000-8000-000000001008\", \"revision\": %s, \"definitionVersion\": \"6b23c0d5f35d1b11f9b683f0b0a617355deb11277d91ae091d399c655b87940d\", \"definitions\": [{\"key\": \"statistic\", \"name\": \"statistic\", \"description\": \"fixture\", \"imageUrl\": null, \"hidden\": false, \"active\": true, \"mode\": \"statistic\", \"condition\": {\"statisticKey\": \"wins\", \"threshold\": 3}, \"rewards\": {\"values\": [{\"key\": \"gold\", \"amount\": 7}]}}, {\"key\": \"welcome\", \"name\": \"welcome\", \"description\": \"fixture\", \"imageUrl\": null, \"hidden\": true, \"active\": true, \"mode\": \"manual\", \"condition\": null, \"rewards\": {\"values\": [{\"key\": \"gold\", \"amount\": 7}]}}, {\"key\": \"second\", \"name\": \"second\", \"description\": \"fixture\", \"imageUrl\": null, \"hidden\": false, \"active\": true, \"mode\": \"manual\", \"condition\": null, \"rewards\": {\"values\": [{\"key\": \"gold\", \"amount\": 5}]}}], \"progress\": [{\"key\": \"welcome\", \"unlockedAt\": \"2026-10-16T00:00:01Z\", \"rewardState\": \"pending\", \"inboxEntryId\": null, \"pendingReward\": {\"issuedAt\": \"2026-10-16T00:00:01Z\", \"values\": [{\"key\": \"gold\", \"amount\": 7}]}}], \"limits\": {\"maxDefinitions\": 20, \"maxLifetimeKeys\": 500}, \"serverTime\": \"2026-10-16T00:00:06Z\"}");
    auto Parse=[](const FString& Text,FHorizonAchievementsSnapshot& Out)
    { TSharedPtr<FJsonObject> Json; return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings) && FHorizonAchievementsSnapshot::TryFromJson(Json,Out); };
    for (const FString& Number:{FString(TEXT("9007199254740991")),FString(TEXT("9007199254740992")),FString(TEXT("9007199254740991.1")),FString(TEXT("1e3")),FString(TEXT("-1"))})
    {
        FHorizonAchievementsSnapshot S; const bool Expected=Number==TEXT("9007199254740991");
        TestEqual(TEXT("Original safe revision integer token"),Parse(Template.Replace(TEXT("%s"),*Number),S),Expected);
        if (Expected) TestEqual(TEXT("Exact revision preserved"),S.Revision,int64(9007199254740991LL));
    }
    const FString Valid=Template.Replace(TEXT("%s"),TEXT("1"));
    FHorizonAchievementsSnapshot S;
    TestTrue(TEXT("Original pending reward accepted"),Parse(Valid,S));
    TestTrue(TEXT("Original amount remains separate from definition"),S.Progress.Num()==1 && S.Progress[0].PendingReward.Values.Num()==1 && S.Progress[0].PendingReward.Values[0].Amount==7);
    for (const FString& Bad:{Valid.Replace(TEXT("\"hidden\": false"),TEXT("\"hidden\": \"true\"")),Valid.Replace(TEXT("\"amount\": 7"),TEXT("\"amount\": 9007199254740991.1")),Valid.Replace(TEXT("\"rewardState\": \"pending\""),TEXT("\"rewardState\": \"unknown\"")),Valid.Replace(TEXT("\"statisticKey\": \"wins\""),TEXT("\"statisticKey\": 7"))})
        TestFalse(TEXT("Malformed booleans, safe numbers, states and keys rejected"),Parse(Bad,S));
    TSharedPtr<FJsonObject> Compact;
    TestTrue(TEXT("Compact fixture parse"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Valid),Compact,FJsonSerializer::EFlags::StoreNumbersAsStrings));
    auto Row=Compact->GetArrayField(TEXT("progress"))[0]->AsObject();
    Row->SetStringField(TEXT("rewardState"),TEXT("complete")); Row->SetField(TEXT("unlockedAt"),MakeShared<FJsonValueNull>()); Row->SetField(TEXT("pendingReward"),MakeShared<FJsonValueNull>());
    TestTrue(TEXT("Only compact complete state accepts absent date"),FHorizonAchievementsSnapshot::TryFromJson(Compact,S));
    Row->SetStringField(TEXT("unlockedAt"),TEXT("2026-10-16T00:00:01Z"));
    TestFalse(TEXT("Mixed compact date rejected"),FHorizonAchievementsSnapshot::TryFromJson(Compact,S));
    Row->SetField(TEXT("unlockedAt"),MakeShared<FJsonValueNull>()); Row->SetStringField(TEXT("rewardState"),TEXT("none"));
    TestFalse(TEXT("Legacy state still requires original date"),FHorizonAchievementsSnapshot::TryFromJson(Compact,S));
    auto* Realtime=NewObject<UHorizonRealtimeManager>();
    TestTrue(TEXT("Optional own-player achievement invalidation topic supported"),Realtime->Subscribe(TEXT("achievements:00000000-0000-4000-8000-000000001008")));
    TestFalse(TEXT("Unknown feature topics remain rejected"),Realtime->Subscribe(TEXT("unknown:00000000-0000-4000-8000-000000001008")));
    Realtime->Close();
    return true;
}
#endif
