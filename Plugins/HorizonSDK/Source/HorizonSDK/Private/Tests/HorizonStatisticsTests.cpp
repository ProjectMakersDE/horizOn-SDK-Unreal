#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonStatisticsManager.h"
#include "HorizonConfig.h"
#include "HorizonSessionSave.h"
#include "Kismet/GameplayStatics.h"

// Real FHttpModule + JSON serialization against Tests/mock_statistics_server.py.
struct FStatisticsTransportState : TSharedFromThis<FStatisticsTransportState>
{
	FAutomationTestBase* Test;
	UHorizonHttpClient* Http = nullptr;
	UHorizonAuthManager* Auth = nullptr;
	UHorizonStatisticsManager* Statistics = nullptr;
	TArray<FString> OriginalHosts;
	FString OriginalKey, Operation;
	double Deadline = FPlatformTime::Seconds() + 45, ClearAt = 0;
	int32 Stage = 0;
	bool bWaiting = false, bCleared = false, bFailed = false;
	bool bHadSessionSave = false, bStorageSafe = false;
	TArray<uint8> OriginalSessionSave;
	explicit FStatisticsTransportState(FAutomationTestBase* InTest, const FString& Host) : Test(InTest)
	{
		bHadSessionSave = UHorizonSessionSave::HasSavedSession();
		bStorageSafe = !bHadSessionSave || UGameplayStatics::LoadDataFromSlot(OriginalSessionSave, UHorizonSessionSave::SlotName, UHorizonSessionSave::UserIndex);
		if (!bStorageSafe) Test->AddError(TEXT("Cannot preserve existing auth save; fixture will not sign in"));
		auto* Config = GetMutableDefault<UHorizonConfig>(); OriginalHosts = Config->Hosts; OriginalKey = Config->ApiKey;
		Config->Hosts = {Host}; Config->ApiKey = TEXT("project-statistics");
		Http = NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
		Auth = NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
		Statistics = NewObject<UHorizonStatisticsManager>(); Statistics->AddToRoot(); Statistics->Initialize(Http, Auth);
	}
	void Cleanup()
	{
		Auth->SignOut(); Http->Disconnect(); Statistics->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
		if (bStorageSafe)
		{
			if (bHadSessionSave) Test->TestTrue(TEXT("Original auth save restored byte-for-byte"), UGameplayStatics::SaveDataToSlot(OriginalSessionSave, UHorizonSessionSave::SlotName, UHorizonSessionSave::UserIndex));
			else UHorizonSessionSave::DeleteFromDisk();
		}
		auto* Config = GetMutableDefault<UHorizonConfig>(); Config->Hosts = OriginalHosts; Config->ApiKey = OriginalKey;
	}
	void Snapshot(bool Success, const FHorizonStatisticsSnapshot& Result, const FString& Code, int32 RequestedStage)
	{
		const FString Expected = RequestedStage == 6 ? TEXT("OPERATION_CONFLICT") : RequestedStage == 8 || RequestedStage == 10 || RequestedStage == 11 ? TEXT("STATE_CONFLICT") : RequestedStage >= 13 ? TEXT("INVALID_RESPONSE") : TEXT("");
		Test->TestEqual(TEXT("Expected statistics outcome"), Success, Expected.IsEmpty());
		Test->TestEqual(TEXT("Stable statistics code"), Code, Expected);
		if (Success != Expected.IsEmpty() || Code != Expected) bFailed = true;
		if (Success)
		{
			Test->TestEqual(TEXT("Definitions available after conditional response"), Result.Definitions.Num(), 1);
			if (!Test->TestEqual(TEXT("One counter received"), Result.Values.Num(), 1)) { bFailed = true; bWaiting = false; return; }
			if (RequestedStage == 2) Test->TestEqual(TEXT("Exact negative safe integer"), Result.Values[0].Value, -9007199254740991LL);
			if (RequestedStage == 4) Test->TestEqual(TEXT("Exact positive safe integer"), Result.Values[0].Value, 9007199254740991LL);
			if (RequestedStage == 5) Test->TestTrue(TEXT("Original operation replayed"), Result.bReplayed);
		}
		bWaiting = false; ++Stage;
	}
	bool Update()
	{
		if (!bStorageSafe) { Http->Disconnect(); Statistics->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot(); auto* Config = GetMutableDefault<UHorizonConfig>(); Config->Hosts = OriginalHosts; Config->ApiKey = OriginalKey; return true; }
		if (bFailed) { Cleanup(); return true; }
		if (FPlatformTime::Seconds() > Deadline) { Test->AddError(FString::Printf(TEXT("Statistics fixture timeout at stage %d (%s)"), Stage, *Statistics->GetLastErrorCode())); Cleanup(); return true; }
		if (Stage == 11 && bWaiting && !bCleared && FPlatformTime::Seconds() >= ClearAt) { Statistics->ClearCache(); bCleared = true; }
		if (bWaiting) return false;
		const TWeakPtr<FStatisticsTransportState> Weak = AsShared();
		if (Stage == 0)
		{
			bWaiting = true;
			Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Success, const FString& Message) { if (auto Self = Weak.Pin()) { Self->Test->TestTrue(*Message, Success); Self->bWaiting = false; ++Self->Stage; } }));
		}
		else if (Stage == 1)
		{
			bWaiting = true;
			Auth->SignInAnonymous(TEXT("fixture-anonymous"), FOnAuthComplete::CreateLambda([Weak](bool Success) { if (auto Self = Weak.Pin()) { Self->Test->TestTrue(TEXT("Fixture sign-in"), Success); Self->bWaiting = false; ++Self->Stage; } }));
		}
		else if (Stage == 7)
		{
			bWaiting = true;
			Statistics->GetDefinitions(FOnStatisticsDefinitionsLoaded::CreateLambda([Weak](bool Success, const FHorizonStatisticsCatalog& Catalog, const FString&, const FString&) { if (auto Self = Weak.Pin()) { Self->Test->TestTrue(TEXT("New catalog loaded"), Success); Self->Test->TestEqual(TEXT("Catalog invalidates old projection"), Self->Statistics->GetCurrentStatistics().Revision, static_cast<int64>(-1)); Self->bWaiting = false; ++Self->Stage; } }));
		}
		else if (Stage >= 15)
		{
			const auto Cached = Statistics->GetCurrentStatistics();
			Test->TestEqual(TEXT("Invalid JSON cannot replace cache"), Cached.Values.Num(), 1);
			if (Cached.Values.Num() == 1) Test->TestEqual(TEXT("Fresh projection preserved"), Cached.Values[0].Value, static_cast<int64>(3));
			bool bLocalRejected = false;
			Statistics->UpdateStatistics({}, TEXT(""), FOnStatisticsLoaded::CreateLambda([&bLocalRejected](bool Success, const FHorizonStatisticsSnapshot&, const FString& Code, const FString&) { bLocalRejected = !Success && Code == TEXT("INVALID_REQUEST"); }));
			Test->TestTrue(TEXT("Malformed batch rejected without HTTP"), bLocalRejected); Cleanup(); return true;
		}
		else
		{
			bWaiting = true; const int32 RequestedStage = Stage;
			auto Callback = FOnStatisticsLoaded::CreateLambda([Weak, RequestedStage](bool Success, const FHorizonStatisticsSnapshot& Result, const FString& Code, const FString&) { if (auto Self = Weak.Pin()) Self->Snapshot(Success, Result, Code, RequestedStage); });
			if (Stage >= 4 && Stage <= 6)
			{
				if (Operation.IsEmpty()) Operation = Statistics->CreateOperationId();
				FHorizonStatisticUpdate Entry; Entry.Key = TEXT("kills"); Entry.Value = Stage == 6 ? 1 : 9007199254740991LL;
				Statistics->UpdateStatistics({Entry}, Operation, Callback);
			}
			else { if (Stage == 11) ClearAt = FPlatformTime::Seconds() + .08; Statistics->GetStatistics(Callback); }
		}
		return false;
	}
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForStatisticsTransport, TSharedPtr<FStatisticsTransportState>, State);
bool FWaitForStatisticsTransport::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonStatisticsNativeTest, "Horizon.SDK.Statistics.NativeTransport", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonStatisticsNativeTest::RunTest(const FString&)
{
	const FString Host = FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_STATISTICS_FIXTURE_URL"));
	if (Host.IsEmpty()) { AddWarning(TEXT("Set HORIZON_STATISTICS_FIXTURE_URL to the local artificial fixture.")); return true; }
	if (Host != TEXT("http://127.0.0.1:18806")) { AddError(TEXT("Only the dedicated loopback fixture is accepted")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForStatisticsTransport(MakeShared<FStatisticsTransportState>(this, Host))); return true;
}
#endif
