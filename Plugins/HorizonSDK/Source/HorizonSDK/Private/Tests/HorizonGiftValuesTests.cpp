// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Examples/HorizonGiftCodesExample.h"
#include "Kismet/GameplayStatics.h"
#include "HorizonConfig.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Transport/HorizonGiftCodeTransportContract.h"

struct FHorizonGiftV2Native : TSharedFromThis<FHorizonGiftV2Native>
{
    FAutomationTestBase* Test; UHorizonHttpClient* Http; UHorizonAuthManager* Auth; UHorizonGiftCodeManager* Gifts; UHorizonPlayerValuesManager* Values;
    TArray<FString> Hosts; FString Key; int32 Retries; float Delay; int32 Stage=0; bool Pending=false; bool PublicChecked=false;
    FHorizonGiftCodeIntent Original,Second,Two,TwoSecond;
    double Deadline=FPlatformTime::Seconds()+90;
    const FString User=TEXT("00000000-0000-4000-8000-000000001007");
    FHorizonGiftV2Native(FAutomationTestBase* T,const FString& Host):Test(T)
    {
        auto* C=GetMutableDefault<UHorizonConfig>(); Hosts=C->Hosts; Key=C->ApiKey; Retries=C->MaxRetryAttempts; Delay=C->RetryDelaySeconds;
        C->Hosts={Host}; C->ApiKey=TEXT("fixture-key-1007"); C->MaxRetryAttempts=2; C->RetryDelaySeconds=.05f;
        Http=NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth=NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        Values=NewObject<UHorizonPlayerValuesManager>(); Values->AddToRoot(); Values->Initialize(Http,Auth);
        Gifts=NewObject<UHorizonGiftCodeManager>(); Gifts->AddToRoot(); Gifts->Initialize(Http,Auth); Gifts->SetPlayerValuesManager(Values);
    }
    void Cleanup()
    {
        UGameplayStatics::DeleteGameInSlot(TEXT("horizon-gift-v2-before-http"),0); Auth->SignOut(); Http->Disconnect(); Gifts->RemoveFromRoot(); Values->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
        auto* C=GetMutableDefault<UHorizonConfig>(); C->Hosts=Hosts; C->ApiKey=Key; C->MaxRetryAttempts=Retries; C->RetryDelaySeconds=Delay;
    }
    void Next() { ++Stage; Pending=false; }
    void Prepare(const FString& Code)
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonGiftV2Native> Weak=AsShared();
        Gifts->PrepareRedemption(Code,FOnGiftCodePrepareComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonGiftCodeIntent& I,const FString& Error,const FString&)
        {
            if (auto S=Weak.Pin())
            {
                S->Test->TestEqual(TEXT("Prepared success"),Ok,Sent<25);
                if(Sent>=25) S->Test->TestEqual(TEXT("Malformed canonical preparation refused"),Error,FString(TEXT("INVALID_RESPONSE")));
                if(Sent==2)
                {
                    S->Original=I;
                    auto* Save=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::CreateSaveGameObject(UHorizonGiftCodePendingRedeem::StaticClass())); Save->Intent=I;
                    S->Test->TestTrue(TEXT("Durable original tuple before first HTTP redemption"),UGameplayStatics::SaveGameToSlot(Save,TEXT("horizon-gift-v2-before-http"),0));
                }
                if(Sent==3) { S->Second=I; S->Test->TestTrue(TEXT("Two devices prepare identical slot"),I.SameIdentity(S->Original)); }
                if(Sent==6) S->Test->TestNotEqual(TEXT("Visible code reused for new immutable campaign"),I.CampaignId,S->Original.CampaignId);
                if(Sent==21) S->Two=I;
                if(Sent==22) S->TwoSecond=I;
                if(Ok) { S->Test->TestEqual(TEXT("Count exceeds JS safe integer exactly"),I.ExpectedCount,FString(TEXT("9007199254740993"))); S->Test->TestEqual(TEXT("Revision at Int64 max exactly"),I.ConfigRevision,FString(TEXT("9223372036854775807"))); }
                S->Next();
            }
        }));
    }
    void Redeem(const FHorizonGiftCodeIntent& I)
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonGiftV2Native> Weak=AsShared();
        Gifts->RedeemPrepared(I,FOnGiftCodeRedeemResultComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonGiftCodeRedeemResult& R,const FString& Error,const FString&)
        {
            if(auto S=Weak.Pin())
            {
                const bool Expected=Sent==4 || Sent==5 || Sent==7 || Sent==23 || Sent==24;
                S->Test->TestEqual(FString::Printf(TEXT("V2 redeem stage%d"),Sent),Ok,Expected);
                if(Expected)
                {
                    S->Test->TestEqual(TEXT("Current balance adopted without adding grants"),S->Values->GetCurrentState().Values[0].Balance,int64(100));
                    if(Sent!=23) { S->Test->TestTrue(TEXT("Replay marks consumed sequence"),R.bReplayed && R.bAlreadyApplied); S->Test->TestTrue(TEXT("No replay rewards"),R.GiftData.IsEmpty() && R.GrantedValues.IsEmpty() && R.GrantedUnlocks.IsEmpty()); }
                    else S->Test->TestTrue(TEXT("Fresh immediate grant"),!R.bAlreadyApplied && R.GrantedValues.Num()==1 && R.GrantedValues[0].Amount==7);
                }
                if(Sent>=8 && Sent<=12) { const TCHAR* Errors[]={TEXT("COUNT_AHEAD"),TEXT("CAMPAIGN_CHANGED"),TEXT("UPGRADE_REQUIRED"),TEXT("MIGRATION_REQUIRED"),TEXT("COUNTER_EXHAUSTED")}; S->Test->TestEqual(TEXT("Domain code preserved with no new intent"),Error,FString(Errors[Sent-8])); }
                if(Sent>=13 && Sent<=15) S->Test->TestEqual(TEXT("Malformed exact result/history rejected"),Error,FString(TEXT("INVALID_RESPONSE")));
                if(Sent>=16 && Sent<=19) S->Test->TestEqual(TEXT("In-flight scope/cache fence"),Error,FString(TEXT("STALE_RESPONSE")));
                S->Next();
            }
        }));
    }
    bool Update()
    {
        if(FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("V2 timeout stage%d"),Stage)); Cleanup(); return true; }
        if(Pending) return false; Pending=true; const TWeakPtr<FHorizonGiftV2Native> Weak=AsShared();
        if(Stage==0) Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if(auto S=Weak.Pin()) { S->Test->TestTrue(TEXT("Connected"),Ok); S->Next(); } }));
        else if(Stage==1 && !PublicChecked) Gifts->Validate(TEXT("PUBLIC"),FOnGiftCodeValidateComplete::CreateLambda([Weak](bool Ok,bool Valid) { if(auto S=Weak.Pin()) { S->Test->TestTrue(TEXT("Public validate needs no player session"),Ok && Valid); S->PublicChecked=true; S->Pending=false; } }));
        else if(Stage==1 || Stage==20) Auth->SignInAnonymous(TEXT("fixture-anonymous-1007"),FOnAuthComplete::CreateLambda([Weak](bool Ok) { if(auto S=Weak.Pin()) { S->Test->TestTrue(TEXT("Original player restored"),Ok); S->Next(); } }));
        else if(Stage==2 || Stage==3 || Stage==6) Prepare(TEXT("VALUES"));
        else if(Stage==4 || Stage==7) Redeem(Original);
        else if(Stage==5)
        {
            auto* Loaded=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(TEXT("horizon-gift-v2-before-http"),0));
            Test->TestTrue(TEXT("Read original durable identity after uncertain outcome"),Loaded && Loaded->Intent.SameIdentity(Original));
            if(!Loaded) { Cleanup(); return true; } Redeem(Loaded->Intent);
        }
        else if(Stage>=8 && Stage<=19)
        {
            const TCHAR* Codes[]={TEXT("COUNT_AHEAD"),TEXT("CAMPAIGN_CHANGED"),TEXT("UPGRADE_REQUIRED"),TEXT("MIGRATION_REQUIRED"),TEXT("COUNTER_EXHAUSTED"),TEXT("NUMERIC"),TEXT("OVERFLOW"),TEXT("HISTORY"),TEXT("CACHE"),TEXT("SESSION"),TEXT("PROJECT"),TEXT("SIGNOUT")};
            auto I=Original; I.Code=Codes[Stage-8]; Redeem(I);
            if(Stage==16) Values->ClearCache();
            if(Stage==17) { Http->SetSessionToken(TEXT("temporary")); Http->SetSessionToken(TEXT("fixture-session-1007")); }
            if(Stage==18) { auto* C=GetMutableDefault<UHorizonConfig>(); C->ApiKey=TEXT("other"); Http->Initialize(); C->ApiKey=TEXT("fixture-key-1007"); Http->Initialize(); }
            if(Stage==19) Auth->SignOut();
        }
        else if(Stage==21 || Stage==22) Prepare(TEXT("TWO"));
        else if(Stage==23) Redeem(Two);
        else if(Stage==24) Redeem(TwoSecond);
        else if(Stage==25 || Stage==26) Prepare(Stage==25?TEXT("NUMERIC"):TEXT("OVERFLOW"));
        else
        {
            bool Rejected=false;
            Gifts->RedeemWithIdempotency(TEXT("DO-NOT-SEND"),TEXT("old-id"),FOnGiftCodeRedeemResultComplete::CreateLambda([&](bool Ok,const FHorizonGiftCodeRedeemResult&,const FString& E,const FString&) { Rejected=!Ok && E==TEXT("UPGRADE_REQUIRED"); }));
            Test->TestTrue(TEXT("V1 explicit upgrade without translating ID"),Rejected);
            auto Wrong=Original; Wrong.UserId=TEXT("other"); Rejected=false;
            Gifts->RedeemPrepared(Wrong,FOnGiftCodeRedeemResultComplete::CreateLambda([&](bool Ok,const FHorizonGiftCodeRedeemResult&,const FString& E,const FString&) { Rejected=!Ok && E==TEXT("INVALID_REQUEST"); }));
            Test->TestTrue(TEXT("Cannot move intent to another owner"),Rejected);
            const FString Slot=TEXT("horizon-gift-v2-native-artificial");
            auto* Save=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::CreateSaveGameObject(UHorizonGiftCodePendingRedeem::StaticClass())); Save->Intent=Original;
            Test->TestTrue(TEXT("Persist original tuple"),UGameplayStatics::SaveGameToSlot(Save,Slot,0));
            auto* Loaded=Cast<UHorizonGiftCodePendingRedeem>(UGameplayStatics::LoadGameFromSlot(Slot,0)); Test->TestTrue(TEXT("Restart roundtrip keeps every exact field"),Loaded && Loaded->Intent.SameIdentity(Original)); UGameplayStatics::DeleteGameInSlot(Slot,0);
            Auth->SignOut(); Rejected=false;
            Gifts->RedeemPrepared(Original,FOnGiftCodeRedeemResultComplete::CreateLambda([&](bool Ok,const FHorizonGiftCodeRedeemResult&,const FString& E,const FString&) { Rejected=!Ok && E==TEXT("SESSION_REQUIRED"); }));
            Test->TestTrue(TEXT("Signed-out intent never sent"),Rejected);
            Cleanup(); return true;
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForGiftV2,TSharedPtr<FHorizonGiftV2Native>,State);
bool FWaitForGiftV2::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGiftV2Transport,"Horizon.GiftValues.NativeTransport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGiftV2Transport::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_GIFT_VALUES_FIXTURE_URL"));
    if(!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Artificial loopback fixture required")); return false; }
    TestNotNull(TEXT("Blueprint prepare"),UHorizonGiftCodeManager::StaticClass()->FindFunctionByName(TEXT("PrepareRedemptionCode")));
    TestNotNull(TEXT("Blueprint send prepared"),UHorizonGiftCodeManager::StaticClass()->FindFunctionByName(TEXT("RedeemPreparedCode")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForGiftV2(MakeShared<FHorizonGiftV2Native>(this,Host))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGiftV2Models,"Horizon.GiftValues.StrictModels",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGiftV2Models::RunTest(const FString&)
{
    for(const auto* Bad:{TEXT(""),TEXT("00"),TEXT("01"),TEXT("+1"),TEXT("-1"),TEXT("1.0"),TEXT("1e2"),TEXT("9223372036854775808")}) TestFalse(TEXT("Invalid Int64 lexeme"),HorizonTransportContract::GiftCount(TCHAR_TO_UTF8(Bad)));
    auto Parse=[](const FString& S) { TSharedPtr<FJsonObject> J; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),J,FJsonSerializer::EFlags::StoreNumbersAsStrings); return J; };
    const FString Valid=TEXT("{\"valid\":true,\"protocolVersion\":2,\"campaignId\":\"00000000-0000-4000-8000-000000101007\",\"expectedCount\":\"9007199254740993\",\"configRevision\":\"9223372036854775807\"}");
    FHorizonGiftCodeIntent I;
    TestTrue(TEXT("Exact strings parsed natively"),FHorizonGiftCodeIntent::TryPrepare(Parse(Valid),TEXT("scope"),TEXT("user"),TEXT("code"),I));
    TestFalse(TEXT("JSON number never coerces to a count string"),FHorizonGiftCodeIntent::TryPrepare(Parse(Valid.Replace(TEXT("\"9007199254740993\""),TEXT("9007199254740993"))),TEXT("scope"),TEXT("user"),TEXT("code"),I));
    TestFalse(TEXT("Overflow rejected"),FHorizonGiftCodeIntent::TryPrepare(Parse(Valid.Replace(TEXT("9007199254740993"),TEXT("9223372036854775808"))),TEXT("scope"),TEXT("user"),TEXT("code"),I));
    return true;
}
#endif
