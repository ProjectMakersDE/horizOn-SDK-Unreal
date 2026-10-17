#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "HorizonConfig.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

struct FPlayerValuesTransportTestState : TSharedFromThis<FPlayerValuesTransportTestState>
{
    FAutomationTestBase* Test;
    UHorizonHttpClient* Http;
    UHorizonAuthManager* Auth;
    UHorizonPlayerValuesManager* Values;
    TArray<FString> OriginalHosts;
    FString OriginalKey;
    int32 OriginalRetries; float OriginalDelay; double RetryStarted = 0; bool bContextChanged = false;
    int32 Stage = 0;
    bool bPending = false;
    double Deadline = FPlatformTime::Seconds() + 45;
    const FString User = TEXT("00000000-0000-4000-8000-000000001004");
    const FString AdjustOperation = TEXT("1792065600000:00000000-0000-4000-8000-000000001005");
    const FString CompactOperation = TEXT("1792065600000:00000000-0000-4000-8000-000000001006");
    const FString Operation = TEXT("1792065600000:00000000-0000-4000-8000-000000001004");
    FPlayerValuesTransportTestState(FAutomationTestBase* InTest, const FString& Host) : Test(InTest)
    {
        auto* Config = GetMutableDefault<UHorizonConfig>();
        OriginalHosts = Config->Hosts; OriginalKey = Config->ApiKey; OriginalRetries = Config->MaxRetryAttempts; OriginalDelay = Config->RetryDelaySeconds;
        Config->Hosts = {Host}; Config->ApiKey = TEXT("fixture-key-1004"); Config->MaxRetryAttempts = 1; Config->RetryDelaySeconds = .5f;
        Http = NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth = NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        Values = NewObject<UHorizonPlayerValuesManager>(); Values->AddToRoot(); Values->Initialize(Http, Auth);
    }
    void Cleanup()
    {
        Auth->SignOut(); Http->Disconnect(); Values->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
        auto* Config = GetMutableDefault<UHorizonConfig>(); Config->Hosts = OriginalHosts; Config->ApiKey = OriginalKey; Config->MaxRetryAttempts = OriginalRetries; Config->RetryDelaySeconds = OriginalDelay;
    }
    FOnPlayerValuesComplete StateCallback()
    {
        const int32 SentStage = Stage; const TWeakPtr<FPlayerValuesTransportTestState> Weak = AsShared();
        return FOnPlayerValuesComplete::CreateLambda([Weak, SentStage](bool Success, const FHorizonPlayerValuesState& State, const FString& Code, const FString&)
        {
            if (auto Self = Weak.Pin())
            {
                const bool Expected = SentStage == 2 || SentStage == 5 || SentStage == 11 || SentStage == 14 || SentStage == 15 || SentStage == 16;
                Self->Test->TestEqual(TEXT("Expected mutation/read result"), Success, Expected);
                const FString ExpectedCode = SentStage == 4 ? TEXT("OPERATION_EXPIRED") : Expected ? TEXT("") : TEXT("STALE_RESPONSE");
                Self->Test->TestEqual(TEXT("Stable error code"), Code, ExpectedCode);
                if (SentStage == 2 && State.Values.Num() == 2)
                {
                    Self->Test->TestEqual(TEXT("Safe int64 preserved"), State.Values[0].Balance, int64(9007199254740991LL));
                    Self->Test->TestFalse(TEXT("Null daily cap"), State.Values[0].bHasDailyCap);
                    Self->Test->TestTrue(TEXT("Zero is real daily cap"), State.Values[1].bHasDailyCap);
                    Self->Test->TestTrue(TEXT("Server clock known"), Self->Values->HasServerClock());
                    Self->Test->TestTrue(TEXT("Operation ID format"), UHorizonPlayerValuesManager::ValidOperationId(Self->Values->CreateOperationId()));
                }
                else if (SentStage == 2) Self->Test->AddError(TEXT("Expected two balances"));
                if (SentStage >= 14 && SentStage <= 16)
                    Self->Test->TestEqual(TEXT("Configured values above old tariff defaults remain readable"), State.Values.Num(), 80);
                if (SentStage == 15)
                {
                    Self->Test->TestFalse(TEXT("Fresh purchase is not replayed"), State.Operation.bReplayed);
                    Self->Test->TestEqual(TEXT("Fresh purchase carries immediate deltas"), State.Operation.Applied.Num(), 2);
                    Self->Test->TestEqual(TEXT("Fresh purchase carries item reference"), State.Operation.Items.Num(), 1);
                }
                if (SentStage == 16)
                {
                    Self->Test->TestEqual(TEXT("Original operation identity preserved"), State.Operation.OperationId, Self->CompactOperation);
                    Self->Test->TestTrue(TEXT("Compact replay acknowledged"), State.Operation.bReplayed);
                    Self->Test->TestEqual(TEXT("Replay does not retain historical deltas"), State.Operation.Applied.Num(), 0);
                    Self->Test->TestEqual(TEXT("Replay does not retain historical item references"), State.Operation.Items.Num(), 0);
                    if (State.Values.Num() == 80)
                    {
                        Self->Test->TestEqual(TEXT("Replay adopts current balance instead of replaying credit"), State.Values[0].Balance, int64(33));
                        Self->Test->TestEqual(TEXT("Lowered daily cap retains already earned values"), State.Values[0].EarnedToday, int64(50));
                        Self->Test->TestTrue(TEXT("Zero cap remains explicit after lowering"), State.Values[0].bHasDailyCap && State.Values[0].DailyCap == 0);
                    }
                    Self->Test->TestEqual(TEXT("Current snapshot replaces cache"), Self->Values->GetCurrentState().Revision, int64(17));
                    Self->Test->TestFalse(TEXT("Changed definitions invalidate old offers"), Self->Values->GetCurrentOffers().bPresent);
                }
                if (SentStage == 4) Self->Test->TestEqual(TEXT("Business error carries current revision"), State.Revision, int64(11));
                if (!Expected && SentStage != 4) Self->Test->TestFalse(TEXT("Stale reply exposes no balance"), State.bPresent);
                if (SentStage == 12) Self->Http->SetSessionToken(TEXT("fixture-session-1004"));
                Self->Stage++; Self->bPending = false;
            }
        });
    }
    bool Update()
    {
        if (FPlatformTime::Seconds() > Deadline) { Test->AddError(FString::Printf(TEXT("Player values fixture timeout at stage%d"), Stage)); Cleanup(); return true; }
        if (bPending) { if (Stage == 12 && !bContextChanged && FPlatformTime::Seconds() - RetryStarted > .1) { Http->SetSessionToken(TEXT("different-session")); bContextChanged = true; } return false; }
        const TWeakPtr<FPlayerValuesTransportTestState> Weak = AsShared();
        bPending = true;
        if (Stage == 0) Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Success, const FString&) { if (auto Self = Weak.Pin()) { Self->Test->TestTrue(TEXT("HTTP connected"), Success); Self->Stage++; Self->bPending = false; } }));
        else if (Stage == 1) Auth->SignInAnonymous(TEXT("fixture-anonymous"), FOnAuthComplete::CreateLambda([Weak](bool Success) { if (auto Self = Weak.Pin()) { Self->Test->TestTrue(TEXT("Player signed in"), Success); Self->Stage++; Self->bPending = false; } }));
        else if (Stage == 3 || Stage == 8 || Stage == 13)
        {
            Values->GetOffers(User, FOnPlayerValuesOffersComplete::CreateLambda([Weak](bool Success, const FHorizonPlayerValuesOffers& Offers, const FString&, const FString&)
            { if (auto Self = Weak.Pin())
                {
                    Self->Test->TestTrue(TEXT("Offers loaded"), Success);
                    if (Self->Stage == 13)
                    {
                        Self->Test->TestEqual(TEXT("Configured definitions above old defaults"), Offers.Definitions.Num(), 80);
                        Self->Test->TestEqual(TEXT("Configured offers above old defaults"), Offers.Offers.Num(), 1201);
                        for (const auto& Offer : Offers.Offers)
                            Self->Test->TestFalse(TEXT("Removed offer day quota stays absent"), Offer.bHasPurchasesPerDay);
                    }
                    else Self->Test->TestTrue(TEXT("Offers contain safe costs"), Offers.Offers.Num() == 1 && Offers.Offers[0].Costs.Num() == 1 && Offers.Offers[0].Costs[0].Amount == 9007199254740991LL);
                    if (Self->Stage == 8) Self->Test->TestFalse(TEXT("Catalogue clears snapshot"), Self->Values->GetCurrentState().bPresent);
                    Self->Stage++; Self->bPending = false;
                } }));
        }
        else if (Stage == 4) Values->Purchase(User, TEXT("energy-pack"), Operation, StateCallback());
        else if (Stage == 5) Values->Adjust(User, TEXT("gold"), -9007199254740991LL, TEXT("debug"), AdjustOperation, StateCallback());
        else if (Stage == 12) { RetryStarted = FPlatformTime::Seconds(); Values->Purchase(User, TEXT("energy-pack"), Operation, StateCallback()); }
        else if (Stage == 15 || Stage == 16) Values->Purchase(User, TEXT("energy-pack"), CompactOperation, StateCallback());
        else if (Stage <= 17)
        {
            Values->GetState(User, StateCallback());
            if (Stage == 6) Values->ClearCache();
            if (Stage == 17) Auth->SignOut();
        }
        else
        {
            bool bLocal = false;
            Values->Adjust(User, TEXT("gold"), 9007199254740992LL, TEXT("debug"), Operation, FOnPlayerValuesComplete::CreateLambda([&](bool Success, const FHorizonPlayerValuesState&, const FString& Code, const FString&) { bLocal = !Success && Code == TEXT("INVALID_REQUEST"); }));
            Test->TestTrue(TEXT("Unsafe int rejected locally"), bLocal);
            Test->TestFalse(TEXT("Logout cleared balances"), Values->GetCurrentState().bPresent);
            Cleanup(); return true;
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForPlayerValuesTransport, TSharedPtr<FPlayerValuesTransportTestState>, State);
bool FWaitForPlayerValuesTransport::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonPlayerValuesTransportTest, "Horizon.PlayerValues.NativeTransport", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonPlayerValuesTransportTest::RunTest(const FString& Parameters)
{
    const FString Host = FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_PLAYER_VALUES_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set the local artificial HORIZON_PLAYER_VALUES_FIXTURE_URL for native REST transport tests")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only loopback fixture accepted")); return false; }
    TestNotNull(TEXT("Blueprint request entry point"), UHorizonPlayerValuesManager::StaticClass()->FindFunctionByName(TEXT("RequestState")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForPlayerValuesTransport(MakeShared<FPlayerValuesTransportTestState>(this, Host)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonPlayerValuesModelTest, "Horizon.PlayerValues.SafeIntegers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonPlayerValuesModelTest::RunTest(const FString& Parameters)
{
    const FString Template = TEXT("{\"userId\":\"player\",\"day\":\"2026-10-15\",\"revision\":1,\"definitionVersion\":1,\"values\":[{\"key\":\"gold\",\"balance\":%s,\"earnedToday\":0,\"dailyCap\":null},{\"key\":\"energy\",\"balance\":0,\"earnedToday\":0,\"dailyCap\":0}],\"operationWindow\":{\"serverTime\":\"2026-10-15T12:00:00Z\",\"oldestAcceptedAt\":\"2026-09-15T12:00:00Z\"}}");
    for (const FString& Lexeme : {FString(TEXT("9007199254740991")), FString(TEXT("9007199254740991.1")), FString(TEXT("9007199254740992")), FString(TEXT("1e3"))})
    {
        const FString Text = Template.Replace(TEXT("%s"), *Lexeme);
        TSharedPtr<FJsonObject> Json;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json, FJsonSerializer::EFlags::StoreNumbersAsStrings);
        FHorizonPlayerValuesState State;
        const bool Expected = Lexeme == TEXT("9007199254740991");
        TestEqual(TEXT("Exact integer lexeme policy"), FHorizonPlayerValuesState::TryFromJson(Json, State), Expected);
        if (Expected)
        {
            TestEqual(TEXT("Safe int64 unchanged"), State.Values[0].Balance, int64(9007199254740991LL));
            TestFalse(TEXT("Null cap remains absent"), State.Values[0].bHasDailyCap);
            TestTrue(TEXT("Zero cap remains present"), State.Values[1].bHasDailyCap);
        }
    }
    return true;
}
#endif
