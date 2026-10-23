// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Models/HorizonMatches.h"
#include "Models/HorizonMatchesWire.h"
#include "Managers/HorizonMatchesManager.h"
#include "Managers/HorizonRealtimeManager.h"
#include "AsyncActions/HorizonAsync_Matches.h"
#include "Examples/HorizonMatchesExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
namespace HorizonMatchStorage { UHorizonMatchOriginal* Load(const FString&); bool Save(const UHorizonMatchOriginal*,const FString&); bool Delete(const FString&); bool Exists(const FString&); }
namespace HorizonMatchesTest
{
const FString A=TEXT("00000000-0000-4000-8000-000000001013"),B=TEXT("00000000-0000-4000-8000-000000002013"),C=TEXT("00000000-0000-4000-8000-000000003013"),Id=TEXT("10000000-0000-4000-8000-000000001013");
FString Exact(const TCHAR* S,int32 Count){FString R;auto& Out=R.GetCharArray();Out.SetNumUninitialized(Count+1);if(Count)FMemory::Memcpy(Out.GetData(),S,Count*sizeof(TCHAR));Out[Count]=0;return R;}
FString Summary(){return TEXT(R"({"id":"10000000-0000-4000-8000-000000001013","name":"a\u0000雪","revision":1,"creatorId":"00000000-0000-4000-8000-000000001013","capacity":3,"visibility":"private","strength":-9007199254740991,"status":"lobby","participantIds":["00000000-0000-4000-8000-000000001013"],"invitedIds":["00000000-0000-4000-8000-000000002013"],"forfeitedIds":[],"currentPlayerId":null,"turnDeadline":null,"createdAt":"2026-10-16T00:00:00Z","startedAt":null,"finishedAt":null,"purgeAt":null,"finalStateRevision":null,"hasDisagreement":false,"ranked":false,"timeoutDue":false})");}
FString View(){auto S=Summary();S.RemoveAt(S.Len()-1);return S+TEXT(R"(,"turnOrder":[],"turnSeconds":60,"timeoutPolicy":"skip","dataBase64":"AP8A","reports":[],"reportDeadline":null,"absoluteDeadline":null,"rankedLeaderboardKey":null,"ownRankedRunId":null,"notificationPending":false})");}
FString Envelope(const FString& View){return TEXT("{\"match\":")+View+TEXT(",\"pollAfterSeconds\":120,\"serverTime\":\"2026-10-16T00:00:30Z\"}");}
bool Read(const FString& Text,FHorizonMatchRead& R){TSharedPtr<FJsonObject> J;return HorizonMatchesWire::Parse(Text,J)&&FHorizonMatchRead::TryFromJson(J,R);}
FString Ledger(const FString& Recent=TEXT("[]")){return TEXT("{\"userId\":\"")+A+TEXT("\",\"revision\":0,\"active\":[],\"invites\":[],\"recent\":")+Recent+TEXT(",\"limits\":{\"activeMatches\":2,\"stateBytes\":1024,\"retainedMatches\":20,\"pendingInvites\":20,\"settlementMatches\":100,\"historyRetentionSeconds\":604800,\"rankedSettlementSeconds\":604800},\"pendingResults\":[],\"pollAfterSeconds\":120,\"serverTime\":\"2026-10-16T00:00:30Z\"}");}
bool My(const FString& Text,FHorizonMyMatches& R){TSharedPtr<FJsonObject> J;return HorizonMatchesWire::Parse(Text,J)&&FHorizonMyMatches::TryFromJson(J,R);}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonMatchesModelsTest,"Horizon.Matches.StrictModels",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonMatchesModelsTest::RunTest(const FString&)
{
    using namespace HorizonMatchesTest;const auto V=View();FHorizonMatchRead R;TestTrue(TEXT("Typed opaque match view"),Read(Envelope(V),R));TestTrue(TEXT("Name keeps Unicode behind NUL"),R.Match.Name.Len()==3&&R.Match.Name[0]=='a'&&R.Match.Name[1]==0&&R.Match.Name[2]==TCHAR(0x96ea));TestTrue(TEXT("State is exact opaque binary"),R.Match.Data==TArray<uint8>({0,255,0})&&R.Match.bHasData);TestEqual(TEXT("Original safe strength minimum"),R.Match.Strength,-9007199254740991LL);TestFalse(TEXT("Null current player explicit"),R.Match.bHasCurrentPlayerId);TestFalse(TEXT("False never becomes omitted"),R.Match.bRanked);TestTrue(TEXT("Full Inbox notice pending remains explicit metadata"),Read(Envelope(V.Replace(TEXT("\"notificationPending\":false"),TEXT("\"notificationPending\":true"))),R)&&R.Match.bNotificationPending);
    TestTrue(TEXT("Invite-only metadata differs from empty accepted bytes"),Read(Envelope(V.Replace(TEXT("\"AP8A\""),TEXT("null"))),R)&&!R.Match.bHasData);TestTrue(TEXT("Empty accepted state is present"),Read(Envelope(V.Replace(TEXT("AP8A"),TEXT(""))),R)&&R.Match.bHasData&&R.Match.Data.IsEmpty());
    for(const FString& Name:{TEXT("\\u0000a"),TEXT("a\\u0000"),TEXT("\\u0000"),TEXT("a\\u0000雪")})TestTrue(TEXT("Leading/trailing/sole NUL remains valid server text"),Read(Envelope(V.Replace(TEXT("a\\u0000雪"),*Name)),R));
    for(const FString& Bad:{V.Replace(TEXT("-9007199254740991"),TEXT("-9007199254740991.1")),V.Replace(TEXT("-9007199254740991"),TEXT("-9007199254740992")),V.Replace(TEXT("\"revision\":1"),TEXT("\"revision\":0")),V.Replace(TEXT("a\\u0000雪"),TEXT("\\ud800")),V.Replace(TEXT("AP8A"),TEXT("AB==")),V.Replace(TEXT("AP8A"),TEXT("AP8A\\n")),V.Replace(TEXT("\"ranked\":false"),TEXT("\"ranked\":null")),V.Replace(TEXT("\"currentPlayerId\":null,"),TEXT("")),V.Replace(TEXT("\"capacity\":3"),TEXT("\"capacity\":9")),V.Replace(TEXT("\"turnSeconds\":60"),TEXT("\"turnSeconds\":59")),V.Replace(TEXT("\"name\":\"a\\u0000雪\""),TEXT("\"name\":\"first\",\"na\\u006de\":\"last\"")),V.Replace(TEXT("\"status\":\"lobby\""),TEXT("\"status\":\"won\""))})TestFalse(FString::Printf(TEXT("Malformed source rejected before coercion: %s"),*Bad.Left(220)),Read(Envelope(Bad),R));
    TArray<uint8> Bytes;Bytes.Init(255,int32(HorizonMatchesContract::MaxStateBytes));TestTrue(TEXT("Maximum configured retained state readable after downgrade"),Read(Envelope(V.Replace(TEXT("AP8A"),*FBase64::Encode(Bytes))),R)&&R.Match.Data==Bytes);Bytes.Add(0);TestFalse(TEXT("Hard state cap enforced"),Read(Envelope(V.Replace(TEXT("AP8A"),*FBase64::Encode(Bytes))),R));
    const FString Participants=TEXT("\"participantIds\":[\"")+A+TEXT("\",\"")+B+TEXT("\",\"")+C+TEXT("\"]");FString Forfeited=Summary().Replace(*(TEXT("\"participantIds\":[\"")+A+TEXT("\"]")),*Participants).Replace(*(TEXT("\"invitedIds\":[\"")+B+TEXT("\"]")),TEXT("\"invitedIds\":[]")).Replace(TEXT("\"status\":\"lobby\""),TEXT("\"status\":\"active\"")).Replace(TEXT("\"forfeitedIds\":[]"),*(TEXT("\"forfeitedIds\":[\"")+A+TEXT("\"]"))).Replace(TEXT("\"currentPlayerId\":null"),*(TEXT("\"currentPlayerId\":\"")+B+TEXT("\""))).Replace(TEXT("\"turnDeadline\":null"),TEXT("\"turnDeadline\":\"2026-10-16T00:01:30Z\""));FHorizonMyMatches M;
    TestTrue(TEXT("Forfeited player recent reference may still be active for two others"),My(Ledger(TEXT("[")+Forfeited+TEXT("]")),M)&&M.Recent.Num()==1&&M.Recent[0].Status==TEXT("active"));TestFalse(TEXT("Active recent requires own forfeit"),My(Ledger(TEXT("[")+Forfeited.Replace(*(TEXT("\"forfeitedIds\":[\"")+A+TEXT("\"]")),TEXT("\"forfeitedIds\":[]"))+TEXT("]")),M));TestTrue(TEXT("Absent ledger revision zero is valid"),My(Ledger(),M)&&M.Revision==0);
    FString Confirmed=V.Replace(TEXT("\"status\":\"lobby\""),TEXT("\"status\":\"confirmed\"")).Replace(TEXT("\"finalStateRevision\":null"),TEXT("\"finalStateRevision\":1")).Replace(TEXT("\"reports\":[]"),*(TEXT("\"reports\":[{\"userId\":\"")+A+TEXT("\",\"finalStateRevision\":1,\"resultBase64\":\"AP8A\",\"winnerIds\":[],\"reportedAt\":\"2026-10-16T00:00:30Z\"}]")));TestTrue(TEXT("Opaque agreed draw does not fabricate winner"),Read(Envelope(Confirmed),R)&&R.Match.Reports[0].WinnerIds.IsEmpty());TestFalse(TEXT("Report requires immutable final revision"),Read(Envelope(Confirmed.Replace(TEXT("\"finalStateRevision\":1,\"resultBase64\""),TEXT("\"finalStateRevision\":2,\"resultBase64\""))),R));return true;
}
struct FHorizonMatchesNative : TSharedFromThis<FHorizonMatchesNative>
{
    FAutomationTestBase* Test;UGameInstance *GA,*GB;UHorizonSubsystem *SA,*SB;AHorizonMatchesExample *A,*B;TArray<FString> OldHosts;FString OldKey,HostA,HostB,SlotA,SlotB;int32 OldRetries;float OldDelay;
    TArray<TPair<FString,TFunction<void()>>> Steps;TFunction<bool()> Ready;int32 Stage=0,Race=0;bool Waiting=false,NewCompleted=false;double Deadline=FPlatformTime::Seconds()+120;
    const FString User=HorizonMatchesTest::A,Peer=HorizonMatchesTest::B,Id=HorizonMatchesTest::Id,Other=TEXT("10000000-0000-4000-8000-000000002013"),InviteId=TEXT("10000000-0000-4000-8000-000000003013"),PurgeId=TEXT("10000000-0000-4000-8000-000000004013"),Cursor=TEXT("b3BhcXVlOnN0cmVuZ3RoOjE=");FHorizonMatchCreateInput Input;
    FHorizonMatchesNative(FAutomationTestBase* T,const FString& H,const FString& H2):Test(T),HostA(H),HostB(H2)
    {
        auto* C=GetMutableDefault<UHorizonConfig>();OldHosts=C->Hosts;OldKey=C->ApiKey;OldRetries=C->MaxRetryAttempts;OldDelay=C->RetryDelaySeconds;C->Hosts={H};C->ApiKey=TEXT("fixture-key-1013");C->MaxRetryAttempts=2;C->RetryDelaySeconds=.05f;
        GA=NewObject<UGameInstance>(GEngine);GA->AddToRoot();GA->InitializeStandalone();SA=GA->GetSubsystem<UHorizonSubsystem>();GB=NewObject<UGameInstance>(GEngine);GB->AddToRoot();GB->InitializeStandalone();SB=GB->GetSubsystem<UHorizonSubsystem>();A=GA->GetWorld()->SpawnActor<AHorizonMatchesExample>();B=GB->GetWorld()->SpawnActor<AHorizonMatchesExample>();
        const FString Project=UHorizonExampleIdentity::Project(C->ApiKey);SlotA=TEXT("horizon-match-original-")+UHorizonExampleIdentity::Project(Project+TEXT("|")+User);SlotB=TEXT("horizon-match-original-")+UHorizonExampleIdentity::Project(Project+TEXT("|")+Peer);for(const auto& S:{SlotA,SlotB})HorizonMatchStorage::Delete(S);
        Input.Name=HorizonMatchesTest::Exact(TEXT("a\0雪"),3);Input.Capacity=2;Input.Strength=-9007199254740991LL;Input.TurnSeconds=60;Input.bRanked=true;Input.Data={0,255,0};Input.InviteeIds={Peer};Setup();
    }
    void Add(const FString& Name,TFunction<void()> Action){Steps.Emplace(Name,MoveTemp(Action));}
    void Next(){++Stage;Waiting=false;Ready={};}
    void Check(bool Ok,const FString& Code,const FString& Expected=TEXT("")){Test->TestEqual(Steps[Stage].Key+TEXT(" result"),Ok,Expected.IsEmpty());Test->TestEqual(TEXT("Original error code preserved"),Code,Expected);Next();}
    void Wait(AHorizonMatchesExample* Actor){Ready=[Actor]{return !Actor->Busy;};}
    void Read(AHorizonMatchesExample* Actor,const FString& MatchId=TEXT("")){// Advance only this test's refresh clock, rather than waiting120 real seconds per simulated turn.
        Actor->RefreshAfter.Empty();Actor->Refresh(MatchId);Wait(Actor);
    }
    UHorizonMatchOriginal* Saved(const FString& Slot){return HorizonMatchStorage::Load(Slot);}
    FOnMatchesComplete Ledger(const FString& Error=TEXT("")){return FOnMatchesComplete::CreateLambda([this,Error](bool Ok,const FHorizonMyMatches& R,const FString& Code,const FString&){if(Ok)Test->TestEqual(TEXT("Ledger session actor exact"),R.UserId,User);Check(Ok,Code,Error);});}
    FOnMatchComplete Match(const FString& Error=TEXT("")){return FOnMatchComplete::CreateLambda([this,Error](bool Ok,const FHorizonMatchRead& R,const FString& Code,const FString&){if(Ok)Test->TestTrue(TEXT("Typed authoritative match returned"),R.bPresent&&R.Match.bPresent);Check(Ok,Code,Error);});}
    FOnMatchLimitsComplete Limits(){return FOnMatchLimitsComplete::CreateLambda([this](bool Ok,const FHorizonMatchLimitsView& R,const FString& Code,const FString&){if(Ok)Test->TestTrue(TEXT("Limits and capabilities exact"),R.Limits.StateBytes==1024&&R.Capabilities.bMatchesRanked&&!R.Config.bHasRankedLeaderboardKey);Check(Ok,Code);});}
    FOnOpenMatchesComplete Open(){return FOnOpenMatchesComplete::CreateLambda([this](bool Ok,const FHorizonMatchPage& R,const FString& Code,const FString&){if(Ok)Test->TestTrue(TEXT("One explicit opaque page only"),R.bPresent);Check(Ok,Code);});}
    FOnValidatedRunStarted Run(const FString& Error){return FOnValidatedRunStarted::CreateLambda([this,Error](bool Ok,const FHorizonValidatedRun&,const FString& Code,const FString&){Check(Ok,Code,Error);});}
    void Setup()
    {
        Add(TEXT("connect A"),[this]{SA->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([this](bool Ok,const FString&){Test->TestTrue(TEXT("Region A connected"),Ok);Next();}));});
        Add(TEXT("restore A existing identity"),[this]{SA->Auth->SignInAnonymous(TEXT("fixture-anonymous-a-1013"),FOnAuthComplete::CreateLambda([this](bool Ok){Test->TestTrue(TEXT("Actor A restored"),Ok);Next();}));});
        Add(TEXT("connect B"),[this]{SB->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([this](bool Ok,const FString&){Test->TestTrue(TEXT("Second independent client connected"),Ok);Next();}));});
        Add(TEXT("restore B existing identity"),[this]{SB->Auth->SignInAnonymous(TEXT("fixture-anonymous-b-1013"),FOnAuthComplete::CreateLambda([this](bool Ok){Test->TestTrue(TEXT("Different actor B restored"),Ok);Next();}));});
        Add(TEXT("limits"),[this]{SA->Matches->GetLimits(User,Limits());});
        Add(TEXT("public page"),[this]{SB->Matches->FindOpenMatches(Peer,-9007199254740991LL,9007199254740991LL,Open());});
        Add(TEXT("explicit next empty page"),[this]{SB->Matches->FindOpenMatches(Peer,-9007199254740991LL,9007199254740991LL,Open(),20,Cursor);});
        Add(TEXT("read-only503 retries"),[this]{SA->Matches->GetMatches(User,Ledger());});
        Add(TEXT("sample ledger before original"),[this]{Read(A);});
        Add(TEXT("crash before first create transport"),[this]{Test->TestTrue(TEXT("Sample has authoritative ledger"),A->GetMatches().bPresent);auto* P=A->Prepare(TEXT("create"),TEXT(""),0,Input,{},{},false);Test->TestTrue(TEXT("Original definitely-unsent serial zero saved"),P&&Saved(SlotA)&&Saved(SlotA)->SendSerial==0);A->Destroy();A=GA->GetWorld()->SpawnActor<AHorizonMatchesExample>();A->RetryOriginal();Wait(A);});
        Add(TEXT("host change preserves original slot"),[this]{const auto* P=Saved(SlotA);Test->TestTrue(TEXT("Lost create retains original ledger0/send1"),P&&P->ExpectedRevision==0&&P->SendSerial==1);auto* Damaged=NewObject<UHorizonMatchOriginal>();Damaged->StorageGeneration=999;Damaged->StorageHash=TEXT("incomplete-generation");UGameplayStatics::SaveGameToSlot(Damaged,SlotA,0);Test->TestTrue(TEXT("Interrupted alternate generation retains complete dispatched original"),Saved(SlotA)&&Saved(SlotA)->ExpectedRevision==0&&Saved(SlotA)->SendSerial==1);auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts={HostB};SA->GetHttpClient()->Initialize();SA->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([this](bool Ok,const FString&){Test->TestTrue(TEXT("Region B same identity connected"),Ok);Next();}));});
        Add(TEXT("uncertain create reconciles only ledger"),[this]{A->RetryOriginal();Wait(A);});
        Add(TEXT("new ledger cannot replace original create"),[this]{Test->TestTrue(TEXT("New region ledger1"),A->GetMatches().bPresent&&A->GetMatches().Revision==1);const auto* P=Saved(SlotA);const FString Intent=P?P->IntentId:FString();A->CreateLobby(Input);Test->TestTrue(TEXT("No second create at fresh revision1"),Saved(SlotA)&&Saved(SlotA)->IntentId==Intent&&Saved(SlotA)->ExpectedRevision==0&&Saved(SlotA)->SendSerial==1);Test->TestFalse(TEXT("Unknown create cannot blindly acknowledge"),A->AcknowledgeAfterRefresh());Read(A,Id);});
        Add(TEXT("exact original Unicode/binary recovery"),[this]{const auto* P=Saved(SlotA);Test->TestTrue(TEXT("Recovery name keeps NUL suffix"),A->GetMatch().Match.Name.Len()==54&&P&&P->Create.Name.Len()==54&&P->Create.Name[52]==0&&P->Create.Name[53]==TCHAR(0x96ea));Test->TestTrue(TEXT("Ledger+point exact nonce reconciliation"),A->ConfirmRecoveredCreate(Id));Test->TestFalse(TEXT("Resolved original removed"),HorizonMatchStorage::Exists(SlotA));A->Refresh(Id);Test->TestFalse(TEXT("Repeated refresh respects poll interval"),A->Busy);Next();});
        Add(TEXT("invitee metadata read"),[this]{Read(B,Id);});
        Add(TEXT("sample accepts invitation"),[this]{Test->TestFalse(TEXT("Invitee does not see binary game state"),B->GetMatch().Match.bHasData);B->StartAction(TEXT("join"),Id,1,{},{});Wait(B);});
        Add(TEXT("creator reviews accepted lobby"),[this]{Read(A,Id);});
        Add(TEXT("sample creator starts exact order"),[this]{A->StartAction(TEXT("start"),Id,2,{}, {User,Peer});Wait(A);});
        Add(TEXT("wrong actor cannot turn"),[this]{SB->Matches->SubmitTurn(Peer,Id,3,{1,255},false,Match(TEXT("MATCH_NOT_YOUR_TURN")));});
        Add(TEXT("A reads own turn"),[this]{Read(A,Id);});
        Add(TEXT("A replaces byte state"),[this]{A->StartAction(TEXT("turn"),Id,3,{0,255,0,1},{},false);Wait(A);});
        Add(TEXT("B reads own turn"),[this]{Read(B,Id);});
        Add(TEXT("B lost turn response"),[this]{B->StartAction(TEXT("turn"),Id,4,{1,0,255},{},false);Wait(B);});
        Add(TEXT("B explicit original retry"),[this]{Test->TestTrue(TEXT("Unknown move keeps revision4"),Saved(SlotB)&&Saved(SlotB)->ExpectedRevision==4&&Saved(SlotB)->SendSerial==1);B->RetryOriginal();Wait(B);});
        Add(TEXT("B reads after conflict"),[this]{Test->TestTrue(TEXT("Conflict cannot refresh original revision"),Saved(SlotB)&&Saved(SlotB)->ExpectedRevision==4&&Saved(SlotB)->SendSerial==2);Read(B,Id);});
        Add(TEXT("B explicitly reconciles original"),[this]{Test->TestTrue(TEXT("Fresh current byte state5"),B->GetMatch().Match.Revision==5&&B->GetMatch().Match.Data==TArray<uint8>({1,0,255}));Test->TestTrue(TEXT("Player acknowledges inspected original"),B->AcknowledgeAfterRefresh());Next();});
        Add(TEXT("A reads final turn"),[this]{Read(A,Id);});
        Add(TEXT("A freezes final bytes"),[this]{A->StartAction(TEXT("turn"),Id,5,{2,255,0},{},true);Wait(A);});
        Add(TEXT("B reads frozen final"),[this]{Read(B,Id);});
        Add(TEXT("B explicit opaque result"),[this]{B->StartAction(TEXT("result"),Id,6,{0,255},{User});Wait(B);});
        Add(TEXT("A reads peer report"),[this]{Read(A,Id);});
        Add(TEXT("A agrees same final bytes and winners"),[this]{A->StartAction(TEXT("result"),Id,6,{0,255},{User});Wait(A);});
        Add(TEXT("A ranked start response lost"),[this]{Test->TestEqual(TEXT("Both reports confirmed without inferring combat"),A->GetMatch().Match.Status,FString(TEXT("confirmed")));A->StartAction(TEXT("ranked"),Id,6,{},{});Wait(A);});
        Add(TEXT("A original ranked recovery"),[this]{Test->TestTrue(TEXT("Unknown run keeps immutable match/final6"),Saved(SlotA)&&Saved(SlotA)->Action==TEXT("ranked")&&Saved(SlotA)->ExpectedRevision==6);A->RetryOriginal();Wait(A);});
        Add(TEXT("ranked submit original lost"),[this]{Test->TestTrue(TEXT("Only recovered match-bound run adopted"),SA->ValidatedActions->GetCurrentRun().RunId==TEXT("30000000-0000-4000-8000-000000001013"));A->SubmitRankedResult(7,{1,2,3});Wait(A);});
        Add(TEXT("restart with unresolved ranked submit"),[this]{Test->TestTrue(TEXT("Submit original retained"),Saved(SlotA)&&Saved(SlotA)->bSubmitPrepared&&Saved(SlotA)->Score==7);SA->ValidatedActions->DiscardRun();A->Destroy();A=GA->GetWorld()->SpawnActor<AHorizonMatchesExample>();Read(A,Id);});
        Add(TEXT("local run absence never resolves saved submission"),[this]{Test->TestTrue(TEXT("Server still names original run"),A->GetMatch().Match.bHasOwnRankedRunId);Test->TestFalse(TEXT("Restart cannot acknowledge or replace unresolved score/log"),A->AcknowledgeAfterRefresh());A->SubmitRankedResult(8,{1,2,3});Test->TestFalse(TEXT("Changed score blocked after restart"),A->Busy);A->RetryOriginal();Wait(A);});
        Add(TEXT("malformed foreign submit receipt"),[this]{A->SubmitRankedResult(7,{1,2,3});Wait(A);});
        Add(TEXT("foreign receipt preserves immutable original"),[this]{Test->TestTrue(TEXT("Foreign run receipt cannot clear original"),Saved(SlotA)&&Saved(SlotA)->Score==7&&A->LastErrorCode==TEXT("INVALID_RESPONSE"));A->RetryOriginal();Wait(A);});
        Add(TEXT("same durable ranked submit only"),[this]{A->SubmitRankedResult(8,{1,2,3});Test->TestFalse(TEXT("Changed score cannot replace unknown submission"),A->Busy);A->SubmitRankedResult(7,{1,2,3});Wait(A);});
        Add(TEXT("consumed run cannot create another"),[this]{Test->TestFalse(TEXT("Accepted run receipt clears original"),HorizonMatchStorage::Exists(SlotA));SA->Matches->StartRankedRun(User,Id,6,Run(TEXT("RUN_ALREADY_USED")));});
        Add(TEXT("decline metadata-only departing ACK"),[this]{SB->Matches->DeclineInvite(Peer,Other,1,Match());});
        Add(TEXT("declined point read not found"),[this]{SB->Matches->GetMatch(Peer,Other,Match(TEXT("MATCH_NOT_FOUND")));});
        Add(TEXT("leave metadata-only departing ACK"),[this]{SB->Matches->LeaveMatch(Peer,Other,2,Match());});
        Add(TEXT("creator cancels lobby"),[this]{SA->Matches->CancelMatch(User,Other,3,Match());});
        Add(TEXT("timeout skip rotates active actor"),[this]{SA->Matches->AdvanceTimeout(User,Other,4,Match());});
        Add(TEXT("timeout forfeit never infers winner"),[this]{SA->Matches->AdvanceTimeout(User,Other,5,Match());});
        Add(TEXT("differing result disputed"),[this]{SB->Matches->ReportResult(Peer,Other,6,{1,255},{User},FOnMatchReportedComplete::CreateLambda([this](bool Ok,const FHorizonMatchReported& R,const FString& Code,const FString&){if(Ok)Test->TestEqual(TEXT("Disagreement explicit"),R.Match.Status,FString(TEXT("disputed")));Check(Ok,Code);}));});
        Add(TEXT("missing report unconfirmed"),[this]{SA->Matches->GetMatch(User,Other,Match());});
        Add(TEXT("503 create exactly once"),[this]{SA->Matches->CreateMatch(User,9007199254740991LL,Input,FOnMatchCreatedComplete::CreateLambda([this](bool Ok,const FHorizonMatchCreated&,const FString& Code,const FString&){Check(Ok,Code,TEXT("SERVER_ERROR"));}));});
        Add(TEXT("codefree429 mutation exactly once"),[this]{SA->Matches->SubmitTurn(User,Id,9007199254740991LL,{0,255,0},false,Match(TEXT("RATE_LIMITED")));});
        Add(TEXT("feature429 mutation exactly once"),[this]{SA->Matches->JoinMatch(User,Id,1,Match(TEXT("MATCH_ACTIVE_LIMIT")));});
        Add(TEXT("simpleServer ranked explicit unsupported"),[this]{SA->Matches->StartRankedRun(User,Id,6,Run(TEXT("FEATURE_NOT_SUPPORTED")));});
        Add(TEXT("sample fresh ledger before definitive rejection"),[this]{Read(A);});
        Add(TEXT("first create definitive rejection"),[this]{A->CreateLobby(Input);Wait(A);});
        Add(TEXT("rejected original requires fresh reconciliation"),[this]{Test->TestTrue(TEXT("Definitive first rejection preserved"),Saved(SlotA)&&Saved(SlotA)->RejectionCode==TEXT("ACTIVE_MATCH_LIMIT_REACHED"));Test->TestFalse(TEXT("No stale acknowledgement"),A->AcknowledgeAfterRefresh());Read(A);});
        Add(TEXT("explicit rejected-create acknowledgement"),[this]{Test->TestTrue(TEXT("Fresh definitive rejection can be acknowledged"),A->AcknowledgeAfterRefresh());Test->TestFalse(TEXT("Both resolved storage generations removed"),HorizonMatchStorage::Exists(SlotA));Next();});
        Add(TEXT("B reviews pending invitation ledger"),[this]{Read(B);});
        Add(TEXT("B reviews departure original match"),[this]{Read(B,InviteId);});
        Add(TEXT("B departure response lost"),[this]{B->StartAction(TEXT("decline"),InviteId,1,{},{});Wait(B);});
        Add(TEXT("B departure original retry is not found"),[this]{B->RetryOriginal();Wait(B);});
        Add(TEXT("B ledger no longer references departed match"),[this]{Read(B);});
        Add(TEXT("B point404 confirms departure without payload access"),[this]{Read(B,InviteId);});
        Add(TEXT("B explicit departure reconciliation"),[this]{Test->TestTrue(TEXT("Lost departure can be explicitly acknowledged"),B->AcknowledgeAfterRefresh());Test->TestFalse(TEXT("Departing original removed without new revision or action"),HorizonMatchStorage::Exists(SlotB));Next();});
        Add(TEXT("review lobby for later purge reconciliation"),[this]{Read(A,PurgeId);});
        Add(TEXT("cancel response lost then retention purged"),[this]{A->StartAction(TEXT("cancel"),PurgeId,1,{},{});Wait(A);});
        Add(TEXT("fresh ledger after known-match purge"),[this]{Read(A);});
        Add(TEXT("purged known-match point404"),[this]{Read(A,PurgeId);});
        Add(TEXT("explicit known-match purge acknowledgement"),[this]{Test->TestTrue(TEXT("Purged known-match original does not strand future actions"),A->AcknowledgeAfterRefresh());Next();});
        Add(TEXT("ranked match reviewed before config removal"),[this]{Read(A,Id);});
        Add(TEXT("first ranked start definitively unconfigured"),[this]{A->StartAction(TEXT("ranked"),Id,6,{},{});Wait(A);});
        Add(TEXT("fresh match after ranked rejection"),[this]{Test->TestTrue(TEXT("Definitive ranked rejection persisted without run"),Saved(SlotA)&&Saved(SlotA)->RejectionCode==TEXT("RANKED_NOT_CONFIGURED")&&!Saved(SlotA)->Run.IsValid());Read(A,Id);});
        Add(TEXT("explicit first-ranked-rejection acknowledgement"),[this]{Test->TestTrue(TEXT("Known rejection does not fabricate terminal run"),A->AcknowledgeAfterRefresh());Next();});
        Add(TEXT("ledger revision watermark"),[this]{SA->Matches->GetMatches(User,Ledger());});
        Add(TEXT("clearcache cannot erase revision watermark"),[this]{SA->Matches->ClearCache();SA->Matches->GetMatches(User,Ledger(TEXT("STALE_RESPONSE")));});
        Add(TEXT("server time cannot regress"),[this]{SA->Matches->GetMatches(User,Ledger(TEXT("STALE_RESPONSE")));});
        Add(TEXT("fractional original integer rejected"),[this]{SA->Matches->GetMatches(User,Ledger(TEXT("INVALID_RESPONSE")));});
        Add(TEXT("clearcache races response"),[this]{SA->Matches->GetMatches(User,Ledger(TEXT("STALE_RESPONSE")));SA->Matches->ClearCache();});
        Add(TEXT("session ABA races response"),[this]{auto* H=SA->GetHttpClient();SA->Matches->GetMatches(User,Ledger(TEXT("STALE_RESPONSE")));H->SetSessionToken(TEXT("other"));H->SetSessionToken(TEXT("fixture-session-a-1013"));});
        Add(TEXT("project ABA races response"),[this]{SA->Matches->GetMatches(User,Ledger(TEXT("STALE_RESPONSE")));auto* C=GetMutableDefault<UHorizonConfig>();C->ApiKey=TEXT("other");SA->GetHttpClient()->Initialize();C->ApiKey=TEXT("fixture-key-1013");SA->GetHttpClient()->Initialize();});
        Add(TEXT("context clean request"),[this]{SA->Matches->GetMatches(User,Ledger());});
        Add(TEXT("dispatch old request before race"),[this]{Race=0;NewCompleted=false;SA->Matches->GetMatches(User,FOnMatchesComplete::CreateLambda([this](bool Ok,const FHorizonMyMatches&,const FString& Code,const FString&){Test->TestTrue(TEXT("Old response stale"),!Ok&&Code==TEXT("STALE_RESPONSE"));Test->TestFalse(TEXT("Old callback precedes new completion"),NewCompleted);if(!NewCompleted){bool Busy=false;SA->Matches->GetMatches(User,FOnMatchesComplete::CreateLambda([&](bool Good,const FHorizonMyMatches&,const FString& C,const FString&){Busy=!Good&&C==TEXT("REQUEST_IN_PROGRESS");}));Test->TestTrue(TEXT("Old callback cannot unlock new request"),Busy);}if(++Race==2)Next();}));Ready=[]{return FPaths::FileExists(FPlatformMisc::GetEnvironmentVariable(TEXT("MOCK_MATCHES_RACE_STARTED_FILE")));};});
        Add(TEXT("old ticket cannot unlock newer request"),[this]{SA->Matches->ClearCache();SA->Matches->GetLimits(User,FOnMatchLimitsComplete::CreateLambda([this](bool Ok,const FHorizonMatchLimitsView& R,const FString&,const FString&){NewCompleted=true;Test->TestTrue(TEXT("New request authoritatively retained"),Ok&&R.bPresent);if(++Race==2)Next();}));});
        Add(TEXT("generic start excludes parallel match and generic starts"),[this]{SA->ValidatedActions->DiscardRun();SA->ValidatedActions->StartRun(TEXT(""),FOnValidatedRunStarted::CreateLambda([this](bool Ok,const FHorizonValidatedRun& R,const FString& C,const FString&){Test->TestTrue(TEXT("Generic original run kept"),Ok&&R.RunId==TEXT("40000000-0000-4000-8000-000000001013")&&C.IsEmpty());Next();}));BlockMatch();BlockGeneric();});
        Add(TEXT("active unrelated generic run cannot be overwritten by match"),[this]{Test->TestTrue(TEXT("Generic run still active after lease release"),SA->ValidatedActions->HasActiveRun());BlockMatch();SA->ValidatedActions->DiscardRun();Next();});
        Add(TEXT("cache clear cannot release match start lease"),[this]{SA->Matches->StartRankedRun(User,Id,6,FOnValidatedRunStarted::CreateLambda([this](bool Ok,const FHorizonValidatedRun&,const FString& C,const FString&){Test->TestTrue(TEXT("Invalidated old match start is stale"),!Ok&&C==TEXT("STALE_RESPONSE"));BlockGeneric();Test->TestFalse(TEXT("Stale response never adopted"),SA->ValidatedActions->HasActiveRun());Next();}));SA->Matches->ClearCache();BlockMatch();BlockGeneric();});
        Add(TEXT("original match run resumes after old lease completion"),[this]{SA->Matches->StartRankedRun(User,Id,6,FOnValidatedRunStarted::CreateLambda([this](bool Ok,const FHorizonValidatedRun& R,const FString& C,const FString&){Test->TestTrue(TEXT("Original match run adopted under its own lease"),Ok&&R.RunId==TEXT("30000000-0000-4000-8000-000000001013")&&C.IsEmpty());SA->ValidatedActions->DiscardRun();BlockGeneric();BlockMatch();Next();}));BlockGeneric();});
        Add(TEXT("generic start session ABA retains lease until old completion"),[this]{SA->ValidatedActions->StartRun(TEXT(""),FOnValidatedRunStarted::CreateLambda([this](bool Ok,const FHorizonValidatedRun&,const FString& C,const FString&){Test->TestTrue(TEXT("Generic ABA response stale"),!Ok&&C==TEXT("STALE_RESPONSE"));BlockGeneric();Next();}));SA->GetHttpClient()->SetSessionToken(TEXT("other"));SA->GetHttpClient()->SetSessionToken(TEXT("fixture-session-a-1013"));BlockGeneric();BlockMatch();});
        Add(TEXT("match start host change retains old lease"),[this]{Race=0;SA->Matches->StartRankedRun(User,Id,6,FOnValidatedRunStarted::CreateLambda([this](bool Ok,const FHorizonValidatedRun&,const FString& C,const FString&){Test->TestTrue(TEXT("Previous-host match response stale"),!Ok&&C==TEXT("STALE_RESPONSE"));BlockGeneric();if(++Race==2)Next();}));auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts={HostA};SA->GetHttpClient()->Initialize();BlockMatch();BlockGeneric();SA->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([this](bool Ok,const FString&){Test->TestTrue(TEXT("Original player changes region"),Ok);if(++Race==2)Next();}));});
        Add(TEXT("new region can recover after old lease is released"),[this]{SA->Matches->StartRankedRun(User,Id,6,FOnValidatedRunStarted::CreateLambda([this](bool Ok,const FHorizonValidatedRun& R,const FString& C,const FString&){Test->TestTrue(TEXT("Same original run on new region"),Ok&&R.RunId==TEXT("30000000-0000-4000-8000-000000001013")&&C.IsEmpty());Next();}));});
        Add(TEXT("active match-bound run excludes generic replacement"),[this]{BlockGeneric();Test->TestEqual(TEXT("Protected match run still selected"),SA->ValidatedActions->GetCurrentRun().RunId,FString(TEXT("30000000-0000-4000-8000-000000001013")));SA->ValidatedActions->DiscardRun();auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts={HostB};SA->GetHttpClient()->Initialize();SA->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([this](bool Ok,const FString&){Test->TestTrue(TEXT("Native fixture region restored"),Ok);Next();}));});
        Add(TEXT("Blueprint doubleactivation sends once"),[this]{auto* Node=UHorizonAsync_MatchLimits::GetLimits(A);Node->Activate();Node->Activate();Ready=[this]{return SA->Matches->GetCurrentLimits().bPresent;};});
        Add(TEXT("Blueprint frozen context and realtime invalidation"),[this]{auto* Node=UHorizonAsync_Matches::GetMatches(A);SA->GetHttpClient()->SetSessionToken(TEXT("other"));SA->GetHttpClient()->SetSessionToken(TEXT("fixture-session-a-1013"));Node->Activate();Local();Next();});
    }
    void BlockGeneric(){auto Rejected=MakeShared<bool>(false);SA->ValidatedActions->StartRun(TEXT(""),FOnValidatedRunStarted::CreateLambda([Rejected](bool Ok,const FHorizonValidatedRun&,const FString& C,const FString&){*Rejected=!Ok&&C==TEXT("RUN_IN_PROGRESS");}));Test->TestTrue(TEXT("Generic competing start rejected locally before HTTP"),*Rejected);}
    void BlockMatch(){auto Rejected=MakeShared<bool>(false);SA->Matches->StartRankedRun(User,Id,6,FOnValidatedRunStarted::CreateLambda([Rejected](bool Ok,const FHorizonValidatedRun&,const FString& C,const FString&){*Rejected=!Ok&&C==TEXT("RUN_IN_PROGRESS");}));Test->TestTrue(TEXT("Match competing start rejected locally before HTTP"),*Rejected);}
    void Local()
    {
        bool Bad=false;auto BadInput=Input;BadInput.InviteeIds={User};SA->Matches->CreateMatch(User,0,BadInput,FOnMatchCreatedComplete::CreateLambda([&](bool Ok,const FHorizonMatchCreated&,const FString& C,const FString&){Bad=!Ok&&C==TEXT("INVALID_REQUEST");}));Test->TestTrue(TEXT("Self invite rejected before HTTP"),Bad);Bad=false;SA->Matches->GetMatches(Peer,FOnMatchesComplete::CreateLambda([&](bool Ok,const FHorizonMyMatches&,const FString& C,const FString&){Bad=!Ok&&C==TEXT("SESSION_FORBIDDEN");}));Test->TestTrue(TEXT("Foreign user cannot use actor A session"),Bad);
        const uint64 E=SA->Matches->GetCacheGeneration();SA->Realtime->OnEvent.Broadcast(TEXT("{\"type\":\"event\",\"version\":1,\"topic\":\"matches:10000000-0000-4000-8000-000000001013\"}"));Test->TestTrue(TEXT("Realtime invalidates without replacing snapshot or HTTP poll"),SA->Matches->GetCacheGeneration()>E&&!SA->Matches->GetCurrentMatch().bPresent);
        SA->Auth->SignOut();Bad=false;SA->Matches->GetMatches(User,FOnMatchesComplete::CreateLambda([&](bool Ok,const FHorizonMyMatches&,const FString& C,const FString&){Bad=!Ok&&C==TEXT("SESSION_REQUIRED");}));Test->TestTrue(TEXT("No implicit auth or signup"),Bad);
    }
    void Cleanup(){for(auto* S:{SA,SB}){S->Auth->SignOut();S->GetHttpClient()->Disconnect();}for(const auto& S:{SlotA,SlotB})HorizonMatchStorage::Delete(S);for(auto* GI:{GA,GB}){auto* W=GI->GetWorld();GI->Shutdown();W->DestroyWorld(false);GEngine->DestroyWorldContext(W);GI->RemoveFromRoot();}auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts=OldHosts;C->ApiKey=OldKey;C->MaxRetryAttempts=OldRetries;C->RetryDelaySeconds=OldDelay;}
    bool Update(){if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("Matches timeout at step%d %s"),Stage,Stage<Steps.Num()?*Steps[Stage].Key:TEXT("done")));Cleanup();return true;}if(Stage>=Steps.Num()){Cleanup();return true;}if(Waiting){if(Ready&&Ready())Next();return false;}Waiting=true;UE_LOG(LogTemp,Display,TEXT("Matches native step%d %s"),Stage,*Steps[Stage].Key);Steps[Stage].Value();return false;}
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonMatches,TSharedPtr<FHorizonMatchesNative>,State);
bool FWaitForHorizonMatches::Update(){return State->Update();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonMatchesTransportTest,"Horizon.Matches.NativeTransport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonMatchesTransportTest::RunTest(const FString&){const FString A=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_MATCHES_FIXTURE_URL")),B=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_MATCHES_REGION_B_URL"));if(!A.StartsWith(TEXT("http://127.0.0.1:"))||!B.StartsWith(TEXT("http://127.0.0.1:"))){AddError(TEXT("Two explicit isolated loopback origins required"));return false;}TestNotNull(TEXT("Matches subsystem"),UHorizonSubsystem::StaticClass()->FindPropertyByName(TEXT("Matches")));for(const TCHAR* N:{TEXT("RequestMatches"),TEXT("RequestMatchLimits"),TEXT("RequestOpenMatches"),TEXT("RequestMatch"),TEXT("CreatePlayerMatch"),TEXT("JoinPlayerMatch"),TEXT("DeclineMatchInvite"),TEXT("StartPlayerMatch"),TEXT("SubmitPlayerTurn"),TEXT("AdvanceMatchTimeout"),TEXT("LeavePlayerMatch"),TEXT("CancelPlayerMatch"),TEXT("ReportMatchResult"),TEXT("RequestRankedMatchRun")})TestNotNull(TEXT("Blueprint reflected action"),UHorizonMatchesManager::StaticClass()->FindFunctionByName(N));ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonMatches(MakeShared<FHorizonMatchesNative>(this,A,B)));return true;}
#endif
