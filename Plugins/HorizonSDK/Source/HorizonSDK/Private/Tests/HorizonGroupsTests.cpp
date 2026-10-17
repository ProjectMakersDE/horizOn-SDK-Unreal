#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonGroupsManager.h"
#include "Managers/HorizonRealtimeManager.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Examples/HorizonGroupsExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
struct FHorizonGroupsNative : TSharedFromThis<FHorizonGroupsNative>
{
    FAutomationTestBase* Test; UGameInstance* Instance;UHorizonHttpClient* Http;UHorizonAuthManager* Auth;UHorizonGroupsManager* Groups;AHorizonGroupsExample *ActorA,*ActorB;
    TArray<FString> OriginalHosts;FString OriginalKey,SampleSlot;int32 OriginalRetries;float OriginalDelay;int32 Stage=0,RaceCallbacks=0;bool bPending=false,bOther=false;double Deadline=FPlatformTime::Seconds()+90;FHorizonGroupMetadata Metadata;
    const FString User=TEXT("00000000-0000-4000-8000-000000001010"),Peer=TEXT("00000000-0000-4000-8000-000000002010"),Group=TEXT("10000000-0000-4000-8000-000000001010"),Request=TEXT("20000000-0000-4000-8000-000000001010");
    FHorizonGroupsNative(FAutomationTestBase* T,const FString& Host):Test(T)
    {
        auto* C=GetMutableDefault<UHorizonConfig>();OriginalHosts=C->Hosts;OriginalKey=C->ApiKey;OriginalRetries=C->MaxRetryAttempts;OriginalDelay=C->RetryDelaySeconds;
        C->Hosts={Host};C->ApiKey=TEXT("fixture-key-1010");C->MaxRetryAttempts=2;C->RetryDelaySeconds=.05f;
        Instance=NewObject<UGameInstance>(GEngine);Instance->AddToRoot();Instance->InitializeStandalone();auto* SDK=Instance->GetSubsystem<UHorizonSubsystem>();Http=SDK->GetHttpClient();Auth=SDK->Auth;Groups=SDK->Groups;
        ActorA=Instance->GetWorld()->SpawnActor<AHorizonGroupsExample>();ActorB=Instance->GetWorld()->SpawnActor<AHorizonGroupsExample>();
        SampleSlot=TEXT("horizon-group-original-")+UHorizonExampleIdentity::Project(UHorizonExampleIdentity::Project(C->ApiKey)+TEXT("|")+User);if(UGameplayStatics::DoesSaveGameExist(SampleSlot,0))UGameplayStatics::DeleteGameInSlot(SampleSlot,0);
        Metadata.Name=TEXT("Native groups");Metadata.Description=FString(3,TEXT("x\0a"));Metadata.JoinMode=TEXT("request");Test->TestTrue(TEXT("Lossless input parsed"),FHorizonGroupJsonData::TryFromJson(TEXT(R"({"":null,"a\u0000b":7,"a\u0000c":9,"A":10,"a":8,".$":{"large":9007199254740993,"precise":0.12345678901234567890123456789,"array":[true,false,null,"😀"]}})"),Metadata.Data));
    }
    void Next(){++Stage;bPending=false;}
    void Cleanup(){Auth->SignOut();Http->Disconnect();if(UGameplayStatics::DoesSaveGameExist(SampleSlot,0))UGameplayStatics::DeleteGameInSlot(SampleSlot,0);auto* World=Instance->GetWorld();Instance->Shutdown();World->DestroyWorld(false);GEngine->DestroyWorldContext(World);Instance->RemoveFromRoot();auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts=OriginalHosts;C->ApiKey=OriginalKey;C->MaxRetryAttempts=OriginalRetries;C->RetryDelaySeconds=OriginalDelay;}
    void SignIn(bool Other){bOther=Other;Auth->SignInAnonymous(Other?TEXT("fixture-peer-1010"):TEXT("fixture-anonymous-1010"),FOnAuthComplete::CreateLambda([W=TWeakPtr<FHorizonGroupsNative>(AsShared())](bool Ok){if(auto S=W.Pin()){S->Test->TestTrue(TEXT("Existing player session restored"),Ok);S->Next();}}));}
    void Result(bool Ok,const FString& Code){const TMap<int32,FString> Errors={{76,TEXT("INVALID_RESPONSE")},{23,TEXT("SERVER_ERROR")} ,{24,TEXT("SERVER_ERROR")} ,{25,TEXT("SERVER_ERROR")} ,{26,TEXT("SERVER_ERROR")} ,{27,TEXT("SERVER_ERROR")} ,{28,TEXT("SERVER_ERROR")} ,{29,TEXT("SERVER_ERROR")} ,{30,TEXT("SERVER_ERROR")} ,{31,TEXT("SERVER_ERROR")} ,{32,TEXT("SERVER_ERROR")} ,{33,TEXT("SERVER_ERROR")} ,{34,TEXT("SERVER_ERROR")} ,{35,TEXT("SERVER_ERROR")} ,{36,TEXT("SERVER_ERROR")} ,{37,TEXT("SERVER_ERROR")} ,{38,TEXT("SERVER_ERROR")} ,{39,TEXT("STALE_RESPONSE")} ,{40,TEXT("INVALID_RESPONSE")} ,{42,TEXT("STALE_RESPONSE")} ,{43,TEXT("STALE_RESPONSE")} ,{45,TEXT("STALE_RESPONSE")} ,{47,TEXT("STALE_RESPONSE")} ,{49,TEXT("STALE_RESPONSE")} ,{53,TEXT("CONNECTION_FAILED")} ,{54,TEXT("REVISION_CONFLICT")} ,{70,TEXT("INBOX_FULL")} ,{71,TEXT("GROUP_MEMBER_LIMIT")} ,{72,TEXT("PLAYER_GROUP_LIMIT")} ,{73,TEXT("REQUEST_NOT_FOUND")} ,{74,TEXT("GROUP_PERMISSION_DENIED")} };const auto* Failure=Errors.Find(Stage);Test->TestEqual(FString::Printf(TEXT("Groups stage %d"),Stage),Ok,Failure==nullptr);Test->TestEqual(TEXT("Original feature/transport code"),Code,Failure?*Failure:FString());Next();}
    FOnGroupsComplete Own(){return FOnGroupsComplete::CreateLambda([W=TWeakPtr<FHorizonGroupsNative>(AsShared())](bool Ok,const FHorizonMyGroupsView& R,const FString& C,const FString&){if(auto S=W.Pin()){if(Ok)S->Test->TestEqual(TEXT("Snapshot bound to actual session"),R.UserId,S->bOther?S->Peer:S->User);S->Result(Ok,C);}});}
    FOnGroupComplete Detail(){return FOnGroupComplete::CreateLambda([W=TWeakPtr<FHorizonGroupsNative>(AsShared())](bool Ok,const FHorizonGroupView& R,const FString& C,const FString&){if(auto S=W.Pin()){if(Ok){S->Test->TestEqual(TEXT("Explicit public/private field presence"),R.bHasData,!S->bOther);if(R.bHasData){S->Test->TestTrue(TEXT("Embedded NUL key survives response DOM"),R.Data.GetJson().Contains(TEXT("a\\u0000b"))&&R.Data.GetJson().Contains(TEXT("a\\u0000c"))&&R.Data.GetJson().Contains(TEXT("\"A\":10")));S->Test->TestTrue(TEXT("Original >safe integer and decimal lexemes preserved"),R.Data.GetJson().Contains(TEXT("9007199254740993"))&&R.Data.GetJson().Contains(TEXT("0.12345678901234567890123456789")));}}S->Result(Ok,C);}});}
    FOnGroupMembersComplete Members(){return FOnGroupMembersComplete::CreateLambda([W=TWeakPtr<FHorizonGroupsNative>(AsShared())](bool Ok,const FHorizonGroupMembersView& R,const FString& C,const FString&){if(auto S=W.Pin()){if(Ok)S->Test->TestEqual(TEXT("One explicitly requested member page"),R.Members.Num(),1);S->Result(Ok,C);}});}
    FOnGroupPendingComplete Pending(){return FOnGroupPendingComplete::CreateLambda([W=TWeakPtr<FHorizonGroupsNative>(AsShared())](bool Ok,const FHorizonGroupPendingSnapshot&,const FString& C,const FString&){if(auto S=W.Pin())S->Result(Ok,C);});}
    FOnGroupMutationComplete Mutation(){return FOnGroupMutationComplete::CreateLambda([W=TWeakPtr<FHorizonGroupsNative>(AsShared())](bool Ok,const FHorizonGroupMutationView& R,const FString& C,const FString&){if(auto S=W.Pin()){if(Ok)S->Test->TestTrue(TEXT("Full typed authoritative mutation adopted"),R.Groups.bPresent&&R.bHasGroup&&S->Groups->GetCurrentGroups().Revision==R.Groups.Revision);S->Result(Ok,C);}});}
    void LocalChecks()
    {
        auto Reject=[this](const TFunction<void(FOnGroupMutationComplete)>& Call){bool Rejected=false;Call(FOnGroupMutationComplete::CreateLambda([&](bool Ok,const FHorizonGroupMutationView&,const FString& C,const FString&){Rejected=!Ok&&C==TEXT("INVALID_REQUEST");}));Test->TestTrue(TEXT("Invalid tuple rejected before HTTP"),Rejected);};
        Reject([this](FOnGroupMutationComplete C){Groups->AcceptInvite(User,Group,-1,Request,C);});Reject([this](FOnGroupMutationComplete C){Groups->AcceptInvite(User,Group,1,TEXT(""),C);});Reject([this](FOnGroupMutationComplete C){Groups->InvitePlayer(User,Group,User,1,C);});Reject([this](FOnGroupMutationComplete C){Groups->CreateGroup(User,9007199254740992LL,Metadata,C);});
        bool NoSession=false;Groups->GetGroups(User,FOnGroupsComplete::CreateLambda([&](bool Ok,const FHorizonMyGroupsView&,const FString& C,const FString&){NoSession=!Ok&&C==TEXT("SESSION_REQUIRED");}));Test->TestTrue(TEXT("No automatic guest creation"),NoSession);
    }
    void CheckLaterIntent(){auto* P=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(SampleSlot,0));Test->TestTrue(TEXT("Delayed receipt preserves later saved original"),P&&P->Metadata.Description.Len()==3&&P->Metadata.Description[2]==TEXT('b'));}
    bool Update()
    {
        if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("Groups timeout stage %d"),Stage));Cleanup();return true;}
        if(bPending){const int32 WaitingStage=Stage;if(WaitingStage==66&&!ActorA->bRefreshing){Test->TestTrue(TEXT("Sample private membership refreshed"),ActorA->GetGroup().bPresent);Next();}if(WaitingStage==52&&!ActorA->bRefreshing){Test->TestTrue(TEXT("Explicit sample read adopted"),ActorA->GetGroups().bPresent&&ActorA->GetGroup().bPresent);Next();}if(WaitingStage==53&&!ActorA->bSending){auto* P=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(SampleSlot,0));Test->TestTrue(TEXT("Original revision persisted across unknown outcome/retry"),P&&P->ExpectedRevision==31&&P->SendSerial==1);Next();}if(WaitingStage==54&&!ActorB->bSending){auto* P=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(SampleSlot,0));Test->TestTrue(TEXT("Original revision persisted across unknown outcome/retry"),P&&P->ExpectedRevision==31&&P->SendSerial==2);Next();}if(WaitingStage==55&&!ActorA->bRefreshing){Test->TestTrue(TEXT("Explicit sample read adopted"),ActorA->GetGroups().bPresent&&ActorA->GetGroup().bPresent);Next();}if(WaitingStage==57&&!ActorA->bSending)Next(); return false;}bPending=true;const TWeakPtr<FHorizonGroupsNative> W=AsShared();switch(Stage){case 0:{Http->ConnectToServer(FOnRequestComplete::CreateLambda([W](bool Ok,const FString&){if(auto S=W.Pin()){S->Test->TestTrue(TEXT("Fixture ready"),Ok);S->Next();}}));break;}
case 1:{SignIn(false);break;}
case 2:{Groups->GetGroups(User,Own());break;}
case 3:{Groups->GetGroup(User,Group,Detail());break;}
case 4:{Groups->GetMembers(User,Group,Members(),TEXT(""),1);break;}
case 5:{Groups->GetMembers(User,Group,Members(),User,1);break;}
case 6:{Groups->GetPending(User,Group,Pending());break;}
case 7:{Groups->CreateGroup(User,1,Metadata,Mutation());break;}
case 8:{Groups->UpdateGroup(User,Group,11,Metadata,Mutation());break;}
case 9:{Groups->JoinGroup(User,Group,12,Mutation());break;}
case 10:{Groups->RequestJoin(User,Group,13,Mutation());break;}
case 11:{Groups->InvitePlayer(User,Group,Peer,14,Mutation());break;}
case 12:{Groups->AcceptInvite(User,Group,15,Request,Mutation());break;}
case 13:{Groups->DeclineInvite(User,Group,16,Request,Mutation());break;}
case 14:{Groups->CancelInvite(User,Group,Peer,17,Request,Mutation());break;}
case 15:{Groups->AcceptRequest(User,Group,Peer,18,Request,Mutation());break;}
case 16:{Groups->DeclineRequest(User,Group,Peer,19,Request,Mutation());break;}
case 17:{Groups->CancelRequest(User,Group,20,Request,Mutation());break;}
case 18:{Groups->LeaveGroup(User,Group,21,Mutation());break;}
case 19:{Groups->RemoveMember(User,Group,Peer,22,Mutation());break;}
case 20:{Groups->SetMemberRole(User,Group,Peer,23,TEXT("officer"),Mutation());break;}
case 21:{Groups->TransferLeadership(User,Group,Peer,24,Mutation());break;}
case 22:{Groups->DeleteGroup(User,Group,25,Mutation());break;}
case 23:{Groups->CreateGroup(User,9007199254740991LL,Metadata,Mutation());break;}
case 24:{Groups->UpdateGroup(User,Group,9007199254740991LL,Metadata,Mutation());break;}
case 25:{Groups->JoinGroup(User,Group,9007199254740991LL,Mutation());break;}
case 26:{Groups->RequestJoin(User,Group,9007199254740991LL,Mutation());break;}
case 27:{Groups->InvitePlayer(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 28:{Groups->AcceptInvite(User,Group,9007199254740991LL,Request,Mutation());break;}
case 29:{Groups->DeclineInvite(User,Group,9007199254740991LL,Request,Mutation());break;}
case 30:{Groups->CancelInvite(User,Group,Peer,9007199254740991LL,Request,Mutation());break;}
case 31:{Groups->AcceptRequest(User,Group,Peer,9007199254740991LL,Request,Mutation());break;}
case 32:{Groups->DeclineRequest(User,Group,Peer,9007199254740991LL,Request,Mutation());break;}
case 33:{Groups->CancelRequest(User,Group,9007199254740991LL,Request,Mutation());break;}
case 34:{Groups->LeaveGroup(User,Group,9007199254740991LL,Mutation());break;}
case 35:{Groups->RemoveMember(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 36:{Groups->SetMemberRole(User,Group,Peer,9007199254740991LL,TEXT("officer"),Mutation());break;}
case 37:{Groups->TransferLeadership(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 38:{Groups->DeleteGroup(User,Group,9007199254740991LL,Mutation());break;}
case 39:{Groups->GetGroups(User,Own());break;}
case 40:{Groups->GetGroups(User,Own());break;}
case 41:{Groups->GetGroups(User,Own());break;}
case 42:{Groups->GetGroups(User,Own());break;}
case 43:{Groups->GetGroups(User,Own());Groups->ClearCache();break;}
case 44:{Groups->GetGroups(User,Own());break;}
case 45:{Groups->GetGroups(User,Own());Http->SetSessionToken(TEXT("other"));Http->SetSessionToken(TEXT("fixture-session-1010"));break;}
case 46:{Groups->GetGroups(User,Own());break;}
case 47:{Groups->GetGroups(User,Own());auto* C=GetMutableDefault<UHorizonConfig>();C->ApiKey=TEXT("other");Http->Initialize();C->ApiKey=TEXT("fixture-key-1010");Http->Initialize();break;}
case 48:{Groups->GetGroups(User,Own());break;}
case 49:{Groups->GetGroups(User,Own());Auth->SignOut();break;}
case 50:{LocalChecks();Next();break;}
case 51:{SignIn(false);break;}
case 52:{ActorA->Refresh(Group);break;}
case 53:{Test->TestEqual(TEXT("Sample reviewed group revision"),ActorA->GetGroup().Revision,int64(31));Test->TestEqual(TEXT("Sample snapshot context"),ActorA->SnapshotContext,Http->GetRequestContextEpoch());ActorA->StartAction(TEXT("invite"),Group,Peer,31,TEXT(""),TEXT(""),Metadata);break;}
case 54:{ActorB->RetrySavedAction();Test->TestFalse(TEXT("No acknowledgement before exact refresh"),ActorA->AcknowledgePendingAfterRefresh());break;}
case 55:{ActorA->Refresh(Group);break;}
case 56:{auto* Original=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(SampleSlot,0));Test->TestNotNull(TEXT("Exact original before acknowledgement"),Original);if(Original){const FString Description=Original->Metadata.Description;Original->Metadata.Description=FString(3,TEXT("x\0b"));UGameplayStatics::SaveGameToSlot(Original,SampleSlot,0);Test->TestFalse(TEXT("Unchanged ID/serial cannot acknowledge a changed original payload"),ActorA->AcknowledgePendingAfterRefresh());Original->Metadata.Description=Description;UGameplayStatics::SaveGameToSlot(Original,SampleSlot,0);}Test->TestTrue(TEXT("Exact refresh allows explicit original acknowledgement"),ActorA->AcknowledgePendingAfterRefresh());Test->TestFalse(TEXT("Only matched original removed"),UGameplayStatics::DoesSaveGameExist(SampleSlot,0));Next();break;}
case 57:{ActorA->StartAction(TEXT("cancel-invite"),Group,Peer,32,Request,TEXT(""),Metadata);auto* P=Cast<UHorizonGroupPendingAction>(UGameplayStatics::LoadGameFromSlot(SampleSlot,0));Test->TestNotNull(TEXT("Actual original persisted before HTTP"),P);if(P){P->Metadata.Description=FString(3,TEXT("x\0b"));UGameplayStatics::SaveGameToSlot(P,SampleSlot,0);}break;}
case 58:{CheckLaterIntent();Next();break;}
case 59:{SignIn(true);break;}
case 60:{Groups->GetGroups(Peer,Own());break;}
case 61:{Groups->GetGroup(Peer,Group,Detail());break;}
case 62:{Groups->AcceptInvite(Peer,Group,35,Request,Mutation());break;}
case 63:{SignIn(false);break;}
case 64:{Groups->GetGroups(User,Own());break;}
case 65:{Groups->TransferLeadership(User,Group,Peer,36,Mutation());break;}
case 66:{ActorA->Refresh(Group);break;}
case 67:{Groups->ClearCache();Groups->GetGroups(User,Own());break;}
case 68:{Test->TestFalse(TEXT("Cache clear/refill cannot revive private sample data"),ActorA->GetGroup().bPresent);Test->TestFalse(TEXT("Cache clear/refill cannot revive old acknowledgement"),ActorA->AcknowledgePendingAfterRefresh());Test->TestTrue(TEXT("Saved original survives invalidated acknowledgement"),UGameplayStatics::DoesSaveGameExist(SampleSlot,0));Next();break;}
case 69:{
Groups->GetGroups(User,FOnGroupsComplete::CreateLambda([W](bool Ok,const FHorizonMyGroupsView&,const FString& C,const FString&){if(auto S=W.Pin()){S->Test->TestTrue(TEXT("Old ticket cannot refill cleared cache"),!Ok&&C==TEXT("STALE_RESPONSE"));bool Busy=false;S->Groups->GetGroups(S->User,FOnGroupsComplete::CreateLambda([&](bool Good,const FHorizonMyGroupsView&,const FString& Code,const FString&){Busy=!Good&&Code==TEXT("REQUEST_IN_PROGRESS");}));S->Test->TestTrue(TEXT("Old callback cannot release newer active request"),Busy);if(++S->RaceCallbacks==2)S->Next();}}));
Groups->ClearCache();Groups->GetGroups(User,FOnGroupsComplete::CreateLambda([W](bool Ok,const FHorizonMyGroupsView& R,const FString& C,const FString&){if(auto S=W.Pin()){S->Test->TestTrue(TEXT("New cache generation retains its own result"),Ok&&C.IsEmpty()&&R.Revision==32);if(++S->RaceCallbacks==2)S->Next();}}));break;}
case 70:{Groups->InvitePlayer(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 71:{Groups->InvitePlayer(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 72:{Groups->InvitePlayer(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 73:{Groups->AcceptInvite(User,Group,9007199254740991LL,Request,Mutation());break;}
case 74:{Groups->InvitePlayer(User,Group,Peer,9007199254740991LL,Mutation());break;}
case 75:{Test->TestFalse(TEXT("Authorization loss clears private cache"),Groups->GetCurrentGroups().bPresent);Groups->GetGroups(User,Own());break;}
case 76:{Groups->UpdateGroup(User,Group,40,Metadata,Mutation());break;}
case 77:{Test->TestFalse(TEXT("Mixed-revision mutation cannot repopulate cache"),Groups->GetCurrentGroups().bPresent);Groups->GetGroups(User,Own());break;}
case 78:{Groups->GetMembers(User,Group,Members());break;}
case 79:{Test->TestFalse(TEXT("Newer partial group revision invalidates older full private snapshot"),Groups->GetCurrentGroups().bPresent);Cleanup();return true;break;} default:Test->AddError(TEXT("Unknown stage"));Cleanup();return true;}return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonGroups,TSharedPtr<FHorizonGroupsNative>,State);
bool FWaitForHorizonGroups::Update(){return State->Update();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonGroupsTransportTest,"Horizon.Groups.NativeTransport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonGroupsTransportTest::RunTest(const FString&)
{
 const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_GROUPS_FIXTURE_URL"));if(Host.IsEmpty()){AddWarning(TEXT("Set isolated HORIZON_GROUPS_FIXTURE_URL"));return true;}if(!Host.StartsWith(TEXT("http://127.0.0.1:"))){AddError(TEXT("Loopback only"));return false;}
 TestNotNull(TEXT("Groups subsystem"),UHorizonSubsystem::StaticClass()->FindPropertyByName(TEXT("Groups")));for(const TCHAR* N:{TEXT("RequestGroups"),TEXT("RequestGroupMembers"),TEXT("CreatePlayerGroup"),TEXT("AcceptInviteAction"),TEXT("DeleteGroupAction"),TEXT("ParseGroupData")})TestNotNull(TEXT("Blueprint surface"),UHorizonGroupsManager::StaticClass()->FindFunctionByName(N));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonGroups(MakeShared<FHorizonGroupsNative>(this,Host)));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonGroupsModelsTest,"Horizon.Groups.StrictModels",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonGroupsModelsTest::RunTest(const FString&)
{
 FHorizonGroupJsonData D;const FString Exact=TEXT(R"({"":null,"a\u0000b":7,"a\u0000c":9,"A":10,"a":8,".$":{"n":9007199254740993,"p":0.12345678901234567890123456789}})");TestTrue(TEXT("Free data preserves original numeric lexemes"),FHorizonGroupJsonData::TryFromJson(Exact,D)&&D.GetJson().Contains(TEXT("9007199254740993"))&&D.GetJson().Contains(TEXT("0.12345678901234567890123456789")));
 TestTrue(TEXT("Pretty whitespace does not consume compact size quota"),FHorizonGroupJsonData::TryFromJson(FString::ChrN(3000,' ')+Exact,D));
 for(const FString& Bad:{FString(TEXT("[]")),FString(TEXT("{\"n\":1e309}")),FString(TEXT("{\"n\":NaN}")),FString(TEXT("{\"n\":1,}")),FString(TEXT("{\"n\":\"\\ud800\"}"))})TestFalse(TEXT("Invalid/non-finite JSON rejected"),FHorizonGroupJsonData::TryFromJson(Bad,D));
 TestTrue(TEXT("2048 compact UTF8 bytes allowed"),FHorizonGroupJsonData::TryFromJson(TEXT("{\"x\":\"")+FString::ChrN(2040,'a')+TEXT("\"}"),D));TestTrue(TEXT("Configured data can exceed former 2048 bytes"),FHorizonGroupJsonData::TryFromJson(TEXT("{\"x\":\"")+FString::ChrN(2041,'a')+TEXT("\"}"),D));
 FString Deep=TEXT("0");for(int32 I=0;I<15;++I)Deep=TEXT("[")+Deep+TEXT("]");TestTrue(TEXT("Server depth16 accepted"),FHorizonGroupJsonData::TryFromJson(TEXT("{\"x\":")+Deep+TEXT("}"),D));TestTrue(TEXT("Configured depth can exceed former 16"),FHorizonGroupJsonData::TryFromJson(TEXT("{\"x\":[")+Deep+TEXT("]}"),D));

 const FString Model=TEXT(R"HORIZON({"userId":"00000000-0000-4000-8000-000000001010","revision":__REV__,"groups":[{"id":"10000000-0000-4000-8000-000000001010","revision":38,"name":"Native groups","description":"Lossless data","joinMode":"request","leaderId":"00000000-0000-4000-8000-000000001010","memberCount":21,"createdAt":"2026-10-16T00:00:00Z","updatedAt":"2026-10-16T00:00:01Z","data":{"":null,"a\u0000b":7,"a\u0000c":9,"A":10,"a":8,".$":{"large":9007199254740993,"precise":0.12345678901234567890123456789,"array":[true,false,null,"😀"]}},"myRole":"member"}],"incomingInvites":[],"outgoingRequests":[],"limits":{"maxMembers":20,"maxGroupsPerPlayer":1,"maxInvitations":20,"maxJoinRequests":20,"maxDataBytes":2048,"memberPageSize":50,"maxPlayerInvitations":2,"maxPlayerRequests":2,"maxNameBytes":120,"maxDescriptionBytes":1024,"maxDataDepth":16},"serverTime":"2026-10-16T00:00:59Z"})HORIZON");auto Parse=[](const FString& Text){TSharedPtr<FJsonObject> J;FHorizonMyGroupsView Out;return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J,FJsonSerializer::EFlags::StoreNumbersAsStrings)&&FHorizonMyGroupsView::TryFromJson(J,Out);};
 for(const FString& Number:{FString(TEXT("9007199254740991")),FString(TEXT("9007199254740992")),FString(TEXT("9007199254740991.1")),FString(TEXT("1e3")),FString(TEXT("-1"))})TestEqual(TEXT("Original safe revision token validated"),Parse(Model.Replace(TEXT("__REV__"),*Number)),Number==TEXT("9007199254740991"));
 const FString Valid=Model.Replace(TEXT("__REV__"),TEXT("32"));TestTrue(TEXT("Retained state above downgraded limits allowed"),Parse(Valid));
 for(const FString& Bad:{Valid.Replace(TEXT("\"maxMembers\":20"),TEXT("\"maxMembers\":\"20\"")),Valid.Replace(TEXT("\"myRole\":\"member\""),TEXT("\"myRole\":true")),Valid.Replace(TEXT("\"data\":{"),TEXT("\"unused\":{"))})TestFalse(TEXT("Malformed type/privacy shape rejected"),Parse(Bad));
 auto* RT=NewObject<UHorizonRealtimeManager>();TestTrue(TEXT("Optional group invalidation subscription"),RT->Subscribe(TEXT("groups:10000000-0000-4000-8000-000000001010")));RT->Close();return true;
}
#endif
