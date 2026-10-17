#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonConfig.h"

struct FHorizonRateLimitNativeState : TSharedFromThis<FHorizonRateLimitNativeState>
{
    FAutomationTestBase* Test;
    UHorizonHttpClient* Http;
    TArray<FString> OriginalHosts;
    FString OriginalKey;
    FString FixtureHost;
    int32 OriginalRetries;
    float OriginalDelay;
    int32 Stage=0;
    bool bPending=false,bSwitched=false,bHostRestored=false;
    double Started=0,Deadline=FPlatformTime::Seconds()+45;
    FHorizonRateLimitNativeState(FAutomationTestBase* InTest,const FString& Host):Test(InTest)
    {
        FixtureHost=Host;
        auto* Config=GetMutableDefault<UHorizonConfig>();
        OriginalHosts=Config->Hosts; OriginalKey=Config->ApiKey; OriginalRetries=Config->MaxRetryAttempts; OriginalDelay=Config->RetryDelaySeconds;
        Config->Hosts={Host}; Config->ApiKey=TEXT("fixture-key-996"); Config->MaxRetryAttempts=2; Config->RetryDelaySeconds=.1f;
        Http=NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize(); Http->SetSessionToken(TEXT("fixture-session-996"));
    }
    void Cleanup()
    {
        Http->Disconnect(); Http->RemoveFromRoot();
        auto* Config=GetMutableDefault<UHorizonConfig>(); Config->Hosts=OriginalHosts; Config->ApiKey=OriginalKey; Config->MaxRetryAttempts=OriginalRetries; Config->RetryDelaySeconds=OriginalDelay;
    }
    FOnHttpResponse Callback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonRateLimitNativeState> Weak=AsShared();
        return FOnHttpResponse::CreateLambda([Weak,Sent](const FHorizonNetworkResponse& Response)
        {
            if (auto Self=Weak.Pin())
            {
                const bool Expected=Sent==1 || Sent==5 || Sent==6 || Sent==10 || Sent>=17;
                Self->Test->TestEqual(FString::Printf(TEXT("Rate transport result stage %d"),Sent),Response.bSuccess,Expected);
                const int32 Status=Expected?200:Sent<=7?429:Sent>=12?0:503;
                Self->Test->TestEqual(TEXT("Original HTTP status preserved"),Response.StatusCode,Status);
                if (Sent>=2 && Sent<=4)
                {
                    const FString Code=Sent==2?TEXT("FUTURE_FEATURE_POLICY"):Sent==3?TEXT("RUN_RATE_LIMITED"):TEXT(" ");
                    Self->Test->TestEqual(TEXT("Any nonempty feature code delivered immediately"),Response.GetErrorCodeString(),Code);
                    Self->Test->TestEqual(TEXT("Feature header retained"),Response.GetHeader(TEXT("retry-after")),FString(TEXT("100000")));
                    Self->Test->TestTrue(TEXT("Feature JSON retained for every transport"),Response.JsonData.IsValid());
                }
                if (Sent==5 || Sent==6) Self->Test->TestEqual(TEXT("Negotiated/binary success bytes retained"),Response.BinaryData.Num(),Sent==5?3:2);
                if (Sent==7)
                {
                    Self->Test->TestEqual(TEXT("Exhausted global code remains HTTP fallback"),Response.GetErrorCodeString(),FString(TEXT("RATE_LIMITED")));
                    Self->Test->TestEqual(TEXT("Exhausted Retry-After retained"),Response.GetHeader(TEXT("Retry-After")),FString(TEXT("1")));
                    Self->Test->TestFalse(TEXT("Global 429 has an explanation"),Response.ErrorMessage.IsEmpty());
                }
                if (Sent>=12 && Sent<=16) Self->Test->TestEqual(TEXT("Session change including ABA cancels scheduled retry"),Response.GetErrorCodeString(),FString(TEXT("SESSION_CHANGED")));
                if (Sent>=17) Self->Test->TestTrue(TEXT("Session-independent calls survive session ABA"),Response.bSuccess);
                if (Sent==14) Self->Test->TestFalse(TEXT("An ABA response exposes no old payload"),Response.JsonData.IsValid());
                if (Sent==16)
                {
                    Self->Test->TestTrue(TEXT("Both host changes completed before retry"),Self->bHostRestored);
                    Self->Test->TestEqual(TEXT("Host returned to original A"),Self->Http->GetActiveHost(),Self->FixtureHost);
                }
                if (Sent>=12) { Self->Http->SetSessionToken(TEXT("fixture-session-996")); Self->bSwitched=false; }
                Self->Stage++; Self->bPending=false;
            }
        });
    }
    bool Update()
    {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Rate fixture timeout stage %d"),Stage)); Cleanup(); return true; }
        if (bPending)
        {
            if (Stage>=12 && !bSwitched && FPlatformTime::Seconds()-Started>.2)
            {
                bSwitched=true;
                if (Stage<=13 || Stage==17)
                {
                    Http->SetSessionToken(TEXT("new-session"));
                    if (Stage==13 || Stage==17) Http->SetSessionToken(TEXT("fixture-session-996"));
                }
                else if (Stage==15)
                {
                    auto* Config=GetMutableDefault<UHorizonConfig>(); Config->ApiKey=TEXT("transient-project"); Http->Initialize();
                    Config->ApiKey=TEXT("fixture-key-996"); Http->Initialize();
                }
                else if (Stage==16)
                {
                    const TWeakPtr<FHorizonRateLimitNativeState> Weak=AsShared();
                    GetMutableDefault<UHorizonConfig>()->Hosts={FixtureHost+TEXT("/")};
                    Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&)
                    {
                        if (auto Self=Weak.Pin())
                        {
                            Self->Test->TestTrue(TEXT("Transient loopback host B connected"),Ok);
                            GetMutableDefault<UHorizonConfig>()->Hosts={Self->FixtureHost};
                            Self->Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Restored,const FString&) { if (auto State=Weak.Pin()) State->bHostRestored=Restored; }));
                        }
                    }));
                }
            }
            return false;
        }
        bPending=true; Started=FPlatformTime::Seconds(); const TWeakPtr<FHorizonRateLimitNativeState> Weak=AsShared();
        if (Stage==0)
        {
            Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Loopback health"),Ok); Self->Stage++; Self->bPending=false; } })); return false;
        }
        if (Stage>18) { Cleanup(); return true; }
        auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("intent"),TEXT("original"));
        if (Stage==1)
        {
            TMap<FString,FString> Headers; Headers.Add(TEXT("If-Match"),TEXT("17"));
            Http->PostJson(Body,TEXT("/fixture/global/json"),true,Callback(),Headers);
            Body->SetStringField(TEXT("intent"),TEXT("caller-mutated")); Headers.Add(TEXT("If-Match"),TEXT("18"));
        }
        else if (Stage==2) Http->PostJson(Body,TEXT("/fixture/feature/json"),true,Callback());
        else if (Stage==3 || Stage==5) Http->PostBinary(Stage==3?TEXT("/fixture/feature/binary"):TEXT("/fixture/global/binary"),{1,2,3},true,Callback());
        else if (Stage==4 || Stage==6) Http->PostJsonForBinary(Body,Stage==4?TEXT("/fixture/feature/negotiate"):TEXT("/fixture/global/negotiate"),true,Callback());
        else if (Stage==7) Http->PostJson(Body,TEXT("/fixture/exhaust"),true,Callback());
        else if (Stage>=8 && Stage<=10)
        {
            Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("code"),Stage==8?TEXT("legacy"):Stage==9?TEXT("blank"):TEXT("explicit"));
            if(Stage==9) Body->SetStringField(TEXT("idempotencyId"),TEXT("\u00a0"));
            if(Stage==10) { Body->SetNumberField(TEXT("protocolVersion"),2); Body->SetStringField(TEXT("campaignId"),TEXT("00000000-0000-4000-8000-000000001007")); Body->SetStringField(TEXT("expectedCount"),TEXT("9007199254740993")); Body->SetStringField(TEXT("configRevision"),TEXT("9223372036854775807")); }
            Http->PostJson(Body,TEXT("/api/v1/app/gift-codes/redeem"),true,Callback());
            Body->SetStringField(TEXT("expectedCount"),TEXT("caller-replaced-count"));
        }
        else if (Stage==11)
        {
            Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("type"),TEXT("ANONYMOUS"));
            Http->PostJsonForBinary(Body,TEXT("/api/v1/app/user-management/signup"),true,Callback());
        }
        else
        {
            const FString Route=Stage==12?TEXT("/fixture/session-switch"):Stage==13?TEXT("/fixture/session-aba"):Stage==14?TEXT("/fixture/response-aba"):Stage==15?TEXT("/fixture/project-aba"):Stage==16?TEXT("/fixture/host-aba"):Stage==17?TEXT("/fixture/session-independent"):TEXT("/fixture/session-independent-response");
            Http->PostJson(Body,Route,Stage<17,Callback());
            if (Stage==14 || Stage==18) { bSwitched=true; Http->SetSessionToken(TEXT("new-session")); Http->SetSessionToken(TEXT("fixture-session-996")); }
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonRateLimit,TSharedPtr<FHorizonRateLimitNativeState>,State);
bool FWaitForHorizonRateLimit::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonRateLimitTransportTest,"Horizon.Transport.RateLimit.NativeHTTP",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonRateLimitTransportTest::RunTest(const FString& Parameters)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_RATE_LIMIT_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set artificial loopback HORIZON_RATE_LIMIT_FIXTURE_URL for rate transport proof")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only loopback fixtures accepted")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonRateLimit(MakeShared<FHorizonRateLimitNativeState>(this,Host)));
    return true;
}
#endif
