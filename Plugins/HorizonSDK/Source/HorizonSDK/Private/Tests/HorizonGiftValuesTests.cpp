#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "HorizonConfig.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

struct FHorizonGiftValuesNative : TSharedFromThis<FHorizonGiftValuesNative>
{
    FAutomationTestBase* Test;
    UHorizonHttpClient* Http;
    UHorizonAuthManager* Auth;
    UHorizonGiftCodeManager* Gifts;
    UHorizonPlayerValuesManager* Values;
    TArray<FString> OriginalHosts;
    FString OriginalKey;
    int32 OriginalRetries;
    float OriginalDelay;
    int32 Stage=0;
    bool bPending=false;
    double Deadline=FPlatformTime::Seconds()+60;
    const FString User=TEXT("00000000-0000-4000-8000-000000001007");
    FHorizonGiftValuesNative(FAutomationTestBase* InTest,const FString& Host):Test(InTest)
    {
        auto* Config=GetMutableDefault<UHorizonConfig>();
        OriginalHosts=Config->Hosts; OriginalKey=Config->ApiKey; OriginalRetries=Config->MaxRetryAttempts; OriginalDelay=Config->RetryDelaySeconds;
        Config->Hosts={Host}; Config->ApiKey=TEXT("fixture-key-1007"); Config->MaxRetryAttempts=2; Config->RetryDelaySeconds=.1f;
        Http=NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth=NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        Values=NewObject<UHorizonPlayerValuesManager>(); Values->AddToRoot(); Values->Initialize(Http,Auth);
        Gifts=NewObject<UHorizonGiftCodeManager>(); Gifts->AddToRoot(); Gifts->Initialize(Http,Auth); Gifts->SetPlayerValuesManager(Values);
    }
    void Cleanup()
    {
        Auth->SignOut(); Http->Disconnect(); Gifts->RemoveFromRoot(); Values->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
        auto* Config=GetMutableDefault<UHorizonConfig>(); Config->Hosts=OriginalHosts; Config->ApiKey=OriginalKey; Config->MaxRetryAttempts=OriginalRetries; Config->RetryDelaySeconds=OriginalDelay;
    }
    void Next() { ++Stage; bPending=false; }
    FOnGiftCodeRedeemResultComplete Callback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonGiftValuesNative> Weak=AsShared();
        return FOnGiftCodeRedeemResultComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonGiftCodeRedeemResult& Result,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                const bool Expected=Sent==3 || Sent==4 || Sent==11 || Sent==17 || Sent==18 || Sent==23;
                const FString ExpectedCode=Expected?TEXT(""):Sent==6?TEXT("PV_MAX_BALANCE"):(Sent==7 || Sent==8 || Sent==21 || Sent==22)?TEXT("INVALID_RESPONSE"):Sent==20?TEXT("REDEEM_REJECTED"):TEXT("STALE_RESPONSE");
                Self->Test->TestEqual(FString::Printf(TEXT("Typed redeem stage %d success"),Sent),Ok,Expected);
                Self->Test->TestEqual(TEXT("Stable typed error"),Code,ExpectedCode);
                if (Sent==3 || Sent==4)
                {
                    Self->Test->TestTrue(TEXT("Lost response and explicit replay preserve server marker"),Result.bReplayed);
                    Self->Test->TestEqual(TEXT("Opaque giftData unchanged"),Result.GiftData,FString(TEXT("{\"original\":\"opaque\"}")));
                    Self->Test->TestEqual(TEXT("Granted amount"),Result.GrantedValues.Num()==1?Result.GrantedValues[0].Amount:int64(-1),int64(7));
                    const auto State=Self->Values->GetCurrentState();
                    Self->Test->TestEqual(TEXT("Complete authoritative balance; no delta addition on replay"),State.Values.Num()==1?State.Values[0].Balance:int64(-1),int64(100));
                }
                if (Sent==6 || Sent==7 || Sent==8 || Sent==9) Self->Test->TestEqual(TEXT("Rejected receipt preserves previous balance revision"),Self->Values->GetCurrentState().Revision,int64(30));
                if (Sent==10) Self->Test->TestFalse(TEXT("PV clear fences grant adoption"),Self->Values->GetCurrentState().bPresent);
                if (Sent==12 || Sent==13 || Sent==14 || Sent==15) Self->Test->TestTrue(TEXT("Invalidated receipt not cached"),Self->Gifts->GetLastGrantedUnlocks().IsEmpty());
                if (Sent==18)
                {
                    Self->Test->TestEqual(TEXT("Literal null string stays a string"),Result.GiftData,FString(TEXT("null")));
                    Self->Test->TestTrue(TEXT("Missing additive fields default empty"),Result.GrantedValues.IsEmpty() && !Result.PlayerValues.bPresent && !Result.bReplayed);
                }
                Self->Next();
            }
        });
    }
    bool Update()
    {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("GiftValues timeout stage %d"),Stage)); Cleanup(); return true; }
        if (bPending) return false;
        bPending=true; const TWeakPtr<FHorizonGiftValuesNative> Weak=AsShared();
        if (Stage==0) Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Loopback connected"),Ok); Self->Next(); } }));
        else if (Stage==1 || Stage==16) Auth->SignInAnonymous(TEXT("fixture-anonymous-1007"),FOnAuthComplete::CreateLambda([Weak](bool Ok) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Existing identity restored"),Ok); Self->Next(); } }));
        else if (Stage==2 || Stage==5 || Stage==19)
        {
            const int32 Sent=Stage;
            Gifts->Redeem(Stage==2?TEXT("LEGACY"):Stage==5?TEXT("LEGACY-503"):TEXT("NULL"),FOnGiftCodeRedeemComplete::CreateLambda([Weak,Sent](bool Ok,const FString& Data,const FString&)
            {
                if (auto Self=Weak.Pin())
                {
                    Self->Test->TestEqual(TEXT("Legacy callback remains compatible"),Ok,Sent!=5);
                    if (Sent==2) Self->Test->TestEqual(TEXT("Original legacy payload"),Data,FString(TEXT("{\"untouched\":true}")));
                    if (Sent==19) Self->Test->TestTrue(TEXT("Nullable giftData maps to legacy empty string"),Data.IsEmpty());
                    Self->Next();
                }
            }));
        }
        else if (Stage==3 || Stage==4) Gifts->RedeemWithIdempotency(TEXT("VALUES"),TEXT("original-1007"),Callback());
        else if (Stage==6) Gifts->RedeemWithIdempotency(TEXT("MAX"),TEXT("max-1007"),Callback());
        else if (Stage==7) Gifts->RedeemWithIdempotency(TEXT("FRACTION"),TEXT("fraction-1007"),Callback());
        else if (Stage==8) Gifts->RedeemWithIdempotency(TEXT("WRONG-USER"),TEXT("wrong-user-1007"),Callback());
        else if (Stage==9) Gifts->RedeemWithIdempotency(TEXT("OLD"),TEXT("old-1007"),Callback());
        else if (Stage==10) { Gifts->RedeemWithIdempotency(TEXT("PV-CLEAR"),TEXT("pv-clear-1007"),Callback()); Values->ClearCache(); }
        else if (Stage==11) Gifts->RedeemWithIdempotency(TEXT("BASELINE"),TEXT("baseline-1007"),Callback());
        else if (Stage==12) { Gifts->RedeemWithIdempotency(TEXT("GIFT-CLEAR"),TEXT("gift-clear-1007"),Callback()); Gifts->ClearCache(); }
        else if (Stage==13) { Gifts->RedeemWithIdempotency(TEXT("SESSION-ABA"),TEXT("session-aba-1007"),Callback()); Http->SetSessionToken(TEXT("other")); Http->SetSessionToken(TEXT("fixture-session-1007")); }
        else if (Stage==14)
        {
            Gifts->RedeemWithIdempotency(TEXT("PROJECT-ABA"),TEXT("project-aba-1007"),Callback());
            auto* Config=GetMutableDefault<UHorizonConfig>(); Config->ApiKey=TEXT("other"); Http->Initialize(); Config->ApiKey=TEXT("fixture-key-1007"); Http->Initialize();
        }
        else if (Stage==15) { Gifts->RedeemWithIdempotency(TEXT("SIGNOUT"),TEXT("signout-1007"),Callback()); Auth->SignOut(); }
        else if (Stage==17) Gifts->RedeemWithIdempotency(TEXT("RESTORED"),TEXT("restored-1007"),Callback());
        else if (Stage==18) Gifts->RedeemWithIdempotency(TEXT("OLD-TYPED"),TEXT("old-typed-1007"),Callback());
        else if (Stage==20) Gifts->RedeemWithIdempotency(TEXT("REJECTED"),TEXT("rejected-1007"),Callback());
        else if (Stage==21) Gifts->RedeemWithIdempotency(TEXT("BAD-REPLAY"),TEXT("bad-replay-1007"),Callback());
        else if (Stage==22) Gifts->RedeemWithIdempotency(TEXT("NO-STATE"),TEXT("no-state-1007"),Callback());
        else if (Stage==23)
        {
            Gifts->RedeemWithIdempotency(TEXT("BUSY"),TEXT("busy-1007"),Callback());
            bool Rejected=false;
            Gifts->RedeemWithIdempotency(TEXT("DO-NOT-SEND"),TEXT("other-intent"),FOnGiftCodeRedeemResultComplete::CreateLambda([&](bool Ok,const FHorizonGiftCodeRedeemResult&,const FString& Code,const FString&) { Rejected=!Ok && Code==TEXT("REQUEST_IN_PROGRESS"); }));
            Test->TestTrue(TEXT("Concurrent redeem does not invent or overtake original intent"),Rejected);
        }
        else
        {
            for (const FString& Id:{FString(),FString(TEXT(" ")),FString(TEXT("\u00a0")),FString(TEXT("\u2007")),FString::ChrN(129,'x')})
            {
                bool Rejected=false;
                Gifts->RedeemWithIdempotency(TEXT("DO-NOT-SEND"),Id,FOnGiftCodeRedeemResultComplete::CreateLambda([&](bool Ok,const FHorizonGiftCodeRedeemResult&,const FString& Code,const FString&) { Rejected=!Ok && Code==TEXT("INVALID_REQUEST"); }));
                Test->TestTrue(TEXT("Invalid original IDs rejected before HTTP"),Rejected);
            }
            Auth->SignOut(); bool Rejected=false;
            Gifts->RedeemWithIdempotency(TEXT("DO-NOT-SEND"),TEXT("original"),FOnGiftCodeRedeemResultComplete::CreateLambda([&](bool Ok,const FHorizonGiftCodeRedeemResult&,const FString& Code,const FString&) { Rejected=!Ok && Code==TEXT("SESSION_REQUIRED"); }));
            Test->TestTrue(TEXT("Signed-out requests stay local"),Rejected);
            Test->TestTrue(TEXT("Signed-out last grants are hidden"),Gifts->GetLastGrantedUnlocks().IsEmpty());
            Cleanup(); return true;
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonGiftValues,TSharedPtr<FHorizonGiftValuesNative>,State);
bool FWaitForHorizonGiftValues::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonGiftValuesTransportTest,"Horizon.GiftValues.NativeTransport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonGiftValuesTransportTest::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_GIFT_VALUES_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set loopback HORIZON_GIFT_VALUES_FIXTURE_URL for native proof")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only artificial loopback accepted")); return false; }
    TestNotNull(TEXT("Blueprint explicit-ID method"),UHorizonGiftCodeManager::StaticClass()->FindFunctionByName(TEXT("RedeemCodeWithIdempotency")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonGiftValues(MakeShared<FHorizonGiftValuesNative>(this,Host)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonGiftValuesModelTest,"Horizon.GiftValues.StrictModels",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonGiftValuesModelTest::RunTest(const FString&)
{
    auto Parse=[](const FString& Text,FHorizonGiftCodeRedeemResult& Result)
    { TSharedPtr<FJsonObject> Json; return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings) && FHorizonGiftCodeRedeemResult::TryFromJson(Json,Result); };
    const FString Base=TEXT("{\"success\":true,\"message\":\"accepted\",\"giftData\":\"{\\\"opaque\\\":true}\",\"grantedValues\":[{\"key\":\"0.gold-test\",\"amount\":%s}],\"playerValues\":{\"userId\":\"00000000-0000-4000-8000-000000001007\",\"day\":\"2026-10-16\",\"revision\":1,\"definitionVersion\":1,\"values\":[],\"operationWindow\":{\"serverTime\":\"2026-10-16T00:00:01Z\",\"oldestAcceptedAt\":\"2026-09-16T00:00:01Z\"}},\"replayed\":true}");
    for (const FString& Number:{FString(TEXT("9007199254740991")),FString(TEXT("9007199254740992")),FString(TEXT("9007199254740991.1")),FString(TEXT("1e3")),FString(TEXT("0")),FString(TEXT("-1"))})
    {
        FHorizonGiftCodeRedeemResult Result; const bool Expected=Number==TEXT("9007199254740991");
        TestEqual(TEXT("Positive safe integer from original token"),Parse(Base.Replace(TEXT("%s"),*Number),Result),Expected);
        if (Expected) TestEqual(TEXT("Exact int64 preserved"),Result.GrantedValues[0].Amount,int64(9007199254740991LL));
    }
    FHorizonGiftCodeRedeemResult Result;
    TestTrue(TEXT("Older response without additive fields"),Parse(TEXT("{\"success\":true,\"message\":\"old\",\"giftData\":\"null\"}"),Result));
    TestEqual(TEXT("Opaque literal null unchanged"),Result.GiftData,FString(TEXT("null")));
    TestTrue(TEXT("Older defaults empty"),Result.GrantedValues.IsEmpty() && !Result.PlayerValues.bPresent && !Result.bReplayed);
    for (const FString& Bad:{FString(TEXT("{\"success\":true,\"message\":\"bad\",\"grantedValues\":null}")),FString(TEXT("{\"success\":true,\"message\":\"bad\",\"replayed\":\"true\"}")),Base.Replace(TEXT("%s"),TEXT("1")).Replace(TEXT("\"amount\":1}"),TEXT("\"amount\":1},{\"key\":\"0.gold-test\",\"amount\":1}")),Base.Replace(TEXT("%s"),TEXT("1")).Replace(TEXT("\"success\":true"),TEXT("\"success\":\"true\"")),Base.Replace(TEXT("%s"),TEXT("1")).Replace(TEXT("\"message\":\"accepted\""),TEXT("\"message\":7")),Base.Replace(TEXT("%s"),TEXT("1")).Replace(TEXT("\"key\":\"0.gold-test\""),TEXT("\"key\":7"))})
        TestFalse(TEXT("Malformed additive fields and duplicates rejected"),Parse(Bad,Result));
    return true;
}
#endif
