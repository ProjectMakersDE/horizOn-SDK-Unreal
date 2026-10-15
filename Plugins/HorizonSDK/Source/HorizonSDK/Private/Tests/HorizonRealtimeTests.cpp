#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonRealtimeManager.h"
#include "Managers/HorizonAuthManager.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonConfig.h"
#include "IWebSocket.h"

// Uses the artificial Server realtime-gateway/test/sdk-fixture.js; never live credentials.
struct FRealtimeTransportTestState : TSharedFromThis<FRealtimeTransportTestState>
{
    FAutomationTestBase* Test;
    UHorizonHttpClient* Http = nullptr;
    UHorizonAuthManager* Auth = nullptr;
    UHorizonRealtimeManager* Realtime = nullptr;
    TArray<FString> OriginalHosts;
    FString OriginalKey;
    double Deadline = FPlatformTime::Seconds() + 30;
    int32 Stage = 0;
    bool bConnected = false, bAuthenticated = false, bEvent = false;
    explicit FRealtimeTransportTestState(FAutomationTestBase* InTest, const FString& Host) : Test(InTest)
    {
        auto* Config = GetMutableDefault<UHorizonConfig>();
        OriginalHosts = Config->Hosts; OriginalKey = Config->ApiKey;
        Config->Hosts = { Host }; Config->ApiKey = TEXT("fixture-key");
        Http = NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth = NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        Realtime = NewObject<UHorizonRealtimeManager>(); Realtime->AddToRoot(); Realtime->Initialize(Http, Auth);
    }
    void Cleanup()
    {
        Realtime->Close(); Http->Disconnect();
        Realtime->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
        auto* Config = GetMutableDefault<UHorizonConfig>();
        Config->Hosts = OriginalHosts; Config->ApiKey = OriginalKey;
    }
    bool Update()
    {
        if (FPlatformTime::Seconds() > Deadline)
        { Test->AddError(FString::Printf(TEXT("Realtime fixture timeout at stage %d (%s)"), Stage, *Realtime->LastErrorCode)); Cleanup(); return true; }
        const TWeakPtr<FRealtimeTransportTestState> Weak = AsShared();
        if (Stage == 0)
        {
            Stage = 1;
            Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Success, const FString&)
            { if (auto Self = Weak.Pin()) Self->bConnected = Success; }));
        }
        else if (Stage == 1 && bConnected)
        {
            Stage = 2;
            Auth->SignInAnonymous(TEXT("fixture-anonymous"), FOnAuthComplete::CreateLambda([Weak](bool Success)
            { if (auto Self = Weak.Pin()) Self->bAuthenticated = Success; }));
        }
        else if (Stage == 2 && bAuthenticated)
        {
            Stage = 3;
            Test->TestTrue(TEXT("Own inbox accepted"), Realtime->Subscribe(TEXT("inbox:00000000-0000-4000-8000-000000000003")));
        }
        else if (Stage == 3 && Realtime->bIsReady && !Realtime->Sent.IsEmpty())
        {
            Stage = 4;
            Realtime->Socket->OnMessage().AddLambda([Weak](const FString& Text)
            { if (auto Self = Weak.Pin()) if (Text.Contains(TEXT("fixture-entry"))) Self->bEvent = true; });
            Http->Get(TEXT("/fixture/event"), false, FOnHttpResponse());
        }
        else if (Stage == 4 && bEvent)
        {
            Test->TestTrue(TEXT("Native websocket received invalidation"), bEvent);
            Auth->SignOut(); Realtime->Tick(0);
            Test->TestFalse(TEXT("Sign-out closes realtime transport"), Realtime->bIsReady);
            Test->TestTrue(TEXT("Sign-out clears desired subscriptions"), Realtime->Topics.IsEmpty());
            Cleanup(); return true;
        }
        Realtime->Tick(0);
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForRealtimeTransport, TSharedPtr<FRealtimeTransportTestState>, State);
bool FWaitForRealtimeTransport::Update() { return State->Update(); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonRealtimeTransportTest, "Horizon.Realtime.NativeTransport", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonRealtimeTransportTest::RunTest(const FString& Parameters)
{
    const FString Host = FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_REALTIME_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set HORIZON_REALTIME_FIXTURE_URL to the local SDK fixture to run native websocket integration.")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only the local artificial SDK fixture is accepted.")); return false; }
    TestFalse(TEXT("Chat never polls"), UHorizonRealtimeManager::CanPoll(TEXT("chat:fixture")));
    TestTrue(TEXT("Reconnect jitter bounded"), UHorizonRealtimeManager::ReconnectDelay(50, 1) <= 30);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForRealtimeTransport(MakeShared<FRealtimeTransportTestState>(this, Host)));
    return true;
}
#endif
