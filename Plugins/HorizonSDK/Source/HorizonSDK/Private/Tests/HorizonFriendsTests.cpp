// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonFriendsManager.h"
#include "Managers/HorizonRealtimeManager.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Examples/HorizonFriendsExample.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

struct FHorizonFriendsNative : TSharedFromThis<FHorizonFriendsNative>
{
    FAutomationTestBase* Test; UHorizonHttpClient* Http; UHorizonAuthManager* Auth; UHorizonFriendsManager* Friends;
    UGameInstance* Instance; AHorizonFriendsExample* ActorA; AHorizonFriendsExample* ActorB; FString SampleSlot;
    TArray<FString> OriginalHosts; FString OriginalKey; int32 OriginalRetries; float OriginalDelay;
    int32 Stage=0,RaceCallbacks=0; bool bPending=false,bNewDone=false; double Deadline=FPlatformTime::Seconds()+60;
    const FString User=TEXT("00000000-0000-4000-8000-000000001009"),B=TEXT("00000000-0000-4000-8000-000000002009"),C=TEXT("00000000-0000-4000-8000-000000003009"),D=TEXT("00000000-0000-4000-8000-000000004009"),E=TEXT("00000000-0000-4000-8000-000000005009"),F=TEXT("00000000-0000-4000-8000-000000006009");
    const FString R1=TEXT("10000000-0000-4000-8000-000000001009"),R2=TEXT("20000000-0000-4000-8000-000000001009"),R3=TEXT("30000000-0000-4000-8000-000000001009"),R4=TEXT("40000000-0000-4000-8000-000000001009");
    FHorizonFriendsNative(FAutomationTestBase* InTest,const FString& Host):Test(InTest)
    {
        auto* Config=GetMutableDefault<UHorizonConfig>(); OriginalHosts=Config->Hosts; OriginalKey=Config->ApiKey; OriginalRetries=Config->MaxRetryAttempts; OriginalDelay=Config->RetryDelaySeconds;
        Config->Hosts={Host}; Config->ApiKey=TEXT("fixture-key-1009"); Config->MaxRetryAttempts=2; Config->RetryDelaySeconds=.1f;
        Instance=NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
        auto* SDK=Instance->GetSubsystem<UHorizonSubsystem>(); Http=SDK->GetHttpClient(); Auth=SDK->Auth; Friends=SDK->Friends;
        ActorA=Instance->GetWorld()->SpawnActor<AHorizonFriendsExample>(); ActorB=Instance->GetWorld()->SpawnActor<AHorizonFriendsExample>();
        const FString Project=FMD5::HashAnsiString(*Config->ApiKey);
        SampleSlot=TEXT("horizon-friend-action-")+FMD5::HashAnsiString(*(Project+TEXT("|")+User));
        // This artificial project/player slot belongs solely to this isolated native run.
        if (UGameplayStatics::DoesSaveGameExist(SampleSlot,0)) UGameplayStatics::DeleteGameInSlot(SampleSlot,0);
    }
    void Cleanup()
    {
        Auth->SignOut(); Http->Disconnect();
        if (UGameplayStatics::DoesSaveGameExist(SampleSlot,0)) UGameplayStatics::DeleteGameInSlot(SampleSlot,0);
        UWorld* World=Instance->GetWorld(); Instance->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); Instance->RemoveFromRoot();
        auto* Config=GetMutableDefault<UHorizonConfig>(); Config->Hosts=OriginalHosts; Config->ApiKey=OriginalKey; Config->MaxRetryAttempts=OriginalRetries; Config->RetryDelaySeconds=OriginalDelay;
    }
    void Next() { ++Stage; bPending=false; }
    FOnFriendsComplete ReadCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonFriendsNative> Weak=AsShared();
        return FOnFriendsComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                const bool Expected=Sent==2 || Sent==7 || Sent==18 || Sent==21 || Sent==24 || Sent==25 || Sent==28 || Sent==30 || Sent==33 || Sent==36 || Sent==40;
                Self->Test->TestEqual(FString::Printf(TEXT("Friends read stage %d"),Sent),Ok,Expected);
                Self->Test->TestEqual(TEXT("Exact read error"),Code,Expected?FString():Sent==34?FString(TEXT("INVALID_RESPONSE")):FString(TEXT("STALE_RESPONSE")));
                if (Sent==2) Self->Test->TestTrue(TEXT("Known last login is timestamp, never online status"),S.Friends.Num()==1 && S.Friends[0].bHasLastActiveAt && !S.Friends[0].LastActiveAt.IsEmpty());
                if (Sent==7) Self->Test->TestTrue(TEXT("Replacement request exposed only by explicit GET"),S.Incoming.Num()==2 && S.Incoming[0].RequestId==Self->R2);
                if (Sent==20 || Sent==23 || Sent==27 || Sent==29) Self->Test->TestFalse(TEXT("Invalidated response cannot refill cache"),Self->Friends->GetCurrentFriends().bPresent);
                if (Sent==19 || Sent==22) Self->Test->TestEqual(TEXT("Revision and time floors survive stale reads"),Self->Friends->GetCurrentFriends().Revision,int64(11));
                if (Sent==25) Self->Test->TestTrue(TEXT("Downgrade retains all 21 rows above limit20"),S.Friends.Num()==21 && S.Limits.MaxFriends==20);
                if (Sent==34) Self->Test->TestEqual(TEXT("Fractional raw revision cannot overwrite cache"),Self->Friends->GetCurrentFriends().Revision,int64(17));
                Self->Next();
            }
        });
    }
    FOnFriendsComplete ActionCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonFriendsNative> Weak=AsShared();
        return FOnFriendsComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                FString Error;
                if (Sent==6 || Sent==35) Error=TEXT("REVISION_CONFLICT");
                else if (Sent==8) Error=TEXT("FRIEND_REQUEST_NOT_FOUND");
                else if (Sent==15) Error=TEXT("INBOX_FULL");
                else if (Sent==17) Error=TEXT("RELATIONSHIP_NOT_ALLOWED");
                Self->Test->TestEqual(FString::Printf(TEXT("Explicit Friends action stage %d"),Sent),Ok,Error.IsEmpty());
                Self->Test->TestEqual(TEXT("Feature failure is terminal and unchanged"),Code,Error);
                if (Sent==6) Self->Test->TestEqual(TEXT("Lost response never invents fresh revision"),Self->Friends->GetCurrentFriends().Revision,int64(2));
                if (Sent==8) Self->Test->TestTrue(TEXT("Old original request ID cannot accept replacement"),Self->Friends->GetCurrentFriends().Incoming.Num()==2 && Self->Friends->GetCurrentFriends().Incoming[0].RequestId==Self->R2);
                if (Sent==9)
                {
                    const auto* Peer=S.Friends.FindByPredicate([Self](const FHorizonFriend& V){ return V.UserId==Self->B; });
                    Self->Test->TestTrue(TEXT("Unknown last login stays explicitly null"),Peer && !Peer->bHasLastActiveAt && Peer->LastActiveAt.IsEmpty());
                }
                if (Sent==14) Self->Test->TestTrue(TEXT("Unblock restores no friendship or request"),S.Friends.Num()==1 && S.Outgoing.IsEmpty() && S.Incoming.IsEmpty() && S.Blocked.Num()==1);
                if (Sent==15) Self->Test->TestEqual(TEXT("Inbox full preserves snapshot"),Self->Friends->GetCurrentFriends().Revision,int64(11));
                if (Sent==17) Self->Test->TestFalse(TEXT("Authorization loss clears cached social data"),Self->Friends->GetCurrentFriends().bPresent);
                if (Sent==26) Self->Test->TestEqual(TEXT("Removal remains possible after downgrade"),S.Friends.Num(),20);
                if (Sent==35) Self->Test->TestEqual(TEXT("Safe integer sent exactly without rounded replacement"),Self->Friends->GetCurrentFriends().Revision,int64(17));
                Self->Next();
            }
        });
    }
    FOnFriendLookupComplete LookupCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonFriendsNative> Weak=AsShared();
        return FOnFriendLookupComplete::CreateLambda([Weak,Sent](bool Ok,const FHorizonFriendLookup& L,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                Self->Test->TestEqual(TEXT("Exact public player lookup"),Ok,Sent==3 || Sent==4);
                Self->Test->TestEqual(TEXT("Lookup error keeps no identity payload"),Code,Sent==16?FString(TEXT("PLAYER_NOT_FOUND")):Sent==31?FString(TEXT("STALE_RESPONSE")):FString());
                if (Ok) Self->Test->TestTrue(TEXT("Unknown public profile is safe placeholder"),L.bPresent && L.UserId==Self->B && L.Display.Name.IsEmpty() && L.Display.Profile.Badges.IsEmpty());
                else Self->Test->TestFalse(TEXT("Failed lookup exposes no public identity"),L.bPresent);
                if (Sent==31) Self->Test->TestFalse(TEXT("Delayed lookup after signout cannot preserve snapshot"),Self->Friends->GetCurrentFriends().bPresent);
                Self->Next();
            }
        });
    }
    void LocalReject(const TFunction<void(FOnFriendsComplete)>& Invoke,const FString& Expected)
    {
        const auto Rejected=MakeShared<bool>(false);
        Invoke(FOnFriendsComplete::CreateLambda([Rejected,Expected](bool Ok,const FHorizonFriendsSnapshot&,const FString& Code,const FString&) { *Rejected=!Ok && Code==Expected; }));
        Test->TestTrue(TEXT("Invalid action rejected before HTTP"),*Rejected);
    }
    bool Update()
    {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Friends timeout stage %d"),Stage)); Cleanup(); return true; }
        if (bPending && (Stage==41 || Stage==43 || Stage==45))
        {
            const FString Suffix=Stage==41?TEXT(":45Z"):Stage==43?TEXT(":46Z"):TEXT(":47Z");
            if (Friends->GetCurrentFriends().ServerTime.EndsWith(Suffix))
            {
                Test->TestEqual(TEXT("Refresh bound only to the pending intent present when it began"),ActorA->bRefreshed,Stage!=41);
                Next();
            }
            return false;
        }
        if (bPending && (Stage==42 || Stage==44))
        {
            if (!ActorB->bSending)
            {
                auto* P=Cast<UHorizonFriendPendingAction>(UGameplayStatics::LoadGameFromSlot(SampleSlot,0));
                Test->TestTrue(TEXT("Uncertain original action remains persisted across actor instances"),P && P->ExpectedRevision==20 && P->SendSerial==(Stage==42?1:2)); Next();
            }
            return false;
        }
        if (bPending) return false;
        bPending=true; const TWeakPtr<FHorizonFriendsNative> Weak=AsShared();
        if (Stage==0) Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Isolated fixture connected"),Ok); Self->Next(); } }));
        else if (Stage==1 || Stage==32 || Stage==39) Auth->SignInAnonymous(TEXT("fixture-anonymous-1009"),FOnAuthComplete::CreateLambda([Weak](bool Ok) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Existing identity restored"),Ok); Self->Next(); } }));
        else if (Stage==3 || Stage==4 || Stage==16 || Stage==31)
        { Friends->FindPlayer(User,Stage==4?FHorizonFriendLookup::FriendCodeFor(B):Stage==16?E:B,LookupCallback()); if (Stage==31) Auth->SignOut(); }
        else if (Stage==5 || Stage==15) Friends->SendRequest(User,Stage==5?F:E,Stage==5?1:11,ActionCallback());
        else if (Stage==6 || Stage==8 || Stage==9) Friends->AcceptRequest(User,B,Stage==6?2:5,Stage==9?R2:R1,ActionCallback());
        else if (Stage==10) Friends->DeclineRequest(User,E,6,R4,ActionCallback());
        else if (Stage==11) Friends->CancelRequest(User,C,7,R3,ActionCallback());
        else if (Stage==12 || Stage==26 || Stage==35) Friends->RemoveFriend(User,Stage==26?D:B,Stage==12?8:Stage==26?13:9007199254740991LL,ActionCallback());
        else if (Stage==13 || Stage==17) Friends->BlockPlayer(User,Stage==13?F:E,Stage==13?9:11,ActionCallback());
        else if (Stage==14) Friends->UnblockPlayer(User,F,10,ActionCallback());
        else if (Stage==20) { Friends->ClearCache(); Friends->GetFriends(User,ReadCallback()); }
        else if (Stage==23) { Friends->GetFriends(User,ReadCallback()); Friends->ClearCache(); }
        else if (Stage==27) { Friends->GetFriends(User,ReadCallback()); Http->SetSessionToken(TEXT("other")); Http->SetSessionToken(TEXT("fixture-session-1009")); }
        else if (Stage==29)
        { Friends->GetFriends(User,ReadCallback()); auto* Config=GetMutableDefault<UHorizonConfig>(); Config->ApiKey=TEXT("other"); Http->Initialize(); Config->ApiKey=TEXT("fixture-key-1009"); Http->Initialize(); }
        else if (Stage==36)
        {
            Friends->GetFriends(User,ReadCallback()); const auto Rejected=MakeShared<bool>(false);
            Friends->FindPlayer(User,B,FOnFriendLookupComplete::CreateLambda([Rejected](bool Ok,const FHorizonFriendLookup&,const FString& Code,const FString&) { *Rejected=!Ok && Code==TEXT("REQUEST_IN_PROGRESS"); }));
            Test->TestTrue(TEXT("Overlapping read/lookup bounded locally"),*Rejected);
        }
        else if (Stage==37)
        {
            Friends->GetFriends(User,FOnFriendsComplete::CreateLambda([Weak](bool Ok,const FHorizonFriendsSnapshot&,const FString& Code,const FString&)
            {
                if (auto Self=Weak.Pin())
                {
                    Self->Test->TestTrue(TEXT("Cleared old ticket rejected"),!Ok && Code==TEXT("STALE_RESPONSE"));
                    if (!Self->bNewDone) Self->LocalReject([Self](FOnFriendsComplete Callback) { Self->Friends->GetFriends(Self->User,Callback); },TEXT("REQUEST_IN_PROGRESS"));
                    else Self->Test->TestEqual(TEXT("Old callback cannot erase newer accepted state"),Self->Friends->GetCurrentFriends().Revision,int64(20));
                    if (++Self->RaceCallbacks==2) Self->Next();
                }
            }));
            Friends->ClearCache();
            Friends->GetFriends(User,FOnFriendsComplete::CreateLambda([Weak](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString&)
            {
                if (auto Self=Weak.Pin())
                { Self->bNewDone=true; Self->Test->TestTrue(TEXT("New generation retains own active ticket"),Ok && Code.IsEmpty() && S.Revision==20); if (++Self->RaceCallbacks==2) Self->Next(); }
            }));
        }
        else if (Stage<38 || Stage==40) Friends->GetFriends(User,ReadCallback());
        else if (Stage==41 || Stage==43 || Stage==45)
        {
            if (Stage==43)
            {
                Test->TestFalse(TEXT("Earlier refresh cannot acknowledge a later second-actor intent"),ActorA->AcknowledgePendingAfterRefresh());
                Test->TestTrue(TEXT("Second actor original remains available for retry"),UGameplayStatics::DoesSaveGameExist(SampleSlot,0));
            }
            ActorA->LoadFriends();
        }
        else if (Stage==42) ActorB->StartAction(TEXT("unblock"),TEXT("00000000-0000-4000-8000-000000007009"),20,TEXT(""));
        else if (Stage==44)
        {
            ActorB->RetrySavedAction();
            Test->TestFalse(TEXT("Refresh of older send serial cannot acknowledge a later retry"),ActorA->AcknowledgePendingAfterRefresh());
            Test->TestTrue(TEXT("Original save survives while second actor retries"),UGameplayStatics::DoesSaveGameExist(SampleSlot,0));
        }
        else if (Stage==46)
        {
            Test->TestTrue(TEXT("Explicit acknowledgement of exact refreshed intent allowed"),ActorA->AcknowledgePendingAfterRefresh());
            Test->TestFalse(TEXT("Acknowledgement removes only that matched original"),UGameplayStatics::DoesSaveGameExist(SampleSlot,0));
            Test->TestFalse(TEXT("Acknowledgement cannot be reused"),ActorA->AcknowledgePendingAfterRefresh());
            Cleanup(); return true;
        }
        else if (Stage==38)
        {
            for (const FString& Query:{FString(TEXT("Public name")),FHorizonFriendLookup::FriendCodeFor(B)+TEXT("="),FString(TEXT("H-AAAAAAAAQACAAAAAAAAAAx"))})
            {
                const auto Rejected=MakeShared<bool>(false);
                Friends->FindPlayer(User,Query,FOnFriendLookupComplete::CreateLambda([Rejected](bool Ok,const FHorizonFriendLookup&,const FString& Code,const FString&) { *Rejected=!Ok && Code==TEXT("INVALID_REQUEST"); }));
                Test->TestTrue(TEXT("Only exact UUID/canonical H-code reaches HTTP"),*Rejected);
            }
            LocalReject([this](FOnFriendsComplete Callback) { Friends->SendRequest(User,User,20,Callback); },TEXT("INVALID_REQUEST"));
            LocalReject([this](FOnFriendsComplete Callback) { Friends->SendRequest(User,TEXT("name"),20,Callback); },TEXT("INVALID_REQUEST"));
            LocalReject([this](FOnFriendsComplete Callback) { Friends->SendRequest(User,B,-1,Callback); },TEXT("INVALID_REQUEST"));
            LocalReject([this](FOnFriendsComplete Callback) { Friends->SendRequest(User,B,9007199254740992LL,Callback); },TEXT("INVALID_REQUEST"));
            LocalReject([this](FOnFriendsComplete Callback) { Friends->AcceptRequest(User,B,20,TEXT(""),Callback); },TEXT("INVALID_REQUEST"));
            LocalReject([this](FOnFriendsComplete Callback) { Friends->GetFriends(B,Callback); },TEXT("SESSION_FORBIDDEN"));
            Auth->SignOut(); LocalReject([this](FOnFriendsComplete Callback) { Friends->GetFriends(User,Callback); },TEXT("SESSION_REQUIRED"));
            Test->TestFalse(TEXT("Signout clears social snapshot"),Friends->GetCurrentFriends().bPresent);
            Next();
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonFriends,TSharedPtr<FHorizonFriendsNative>,State);
bool FWaitForHorizonFriends::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonFriendsTransportTest,"Horizon.Friends.NativeTransport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonFriendsTransportTest::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_FRIENDS_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set isolated HORIZON_FRIENDS_FIXTURE_URL for native proof")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only artificial loopback accepted")); return false; }
    TestNotNull(TEXT("Subsystem Friends property"),UHorizonSubsystem::StaticClass()->FindPropertyByName(TEXT("Friends")));
    for (const TCHAR* Name:{TEXT("RequestFriends"),TEXT("FindFriendPlayer"),TEXT("AcceptFriendRequest"),TEXT("UnblockFriendPlayer")}) TestNotNull(TEXT("Blueprint wrapper exists"),UHorizonFriendsManager::StaticClass()->FindFunctionByName(Name));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonFriends(MakeShared<FHorizonFriendsNative>(this,Host))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonFriendsModelTest,"Horizon.Friends.StrictModels",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonFriendsModelTest::RunTest(const FString&)
{
    const FString User=TEXT("00000000-0000-4000-8000-000000001009"),Peer=TEXT("00000000-0000-4000-8000-000000002009");
    const FString Template=TEXT("{\"userId\":\"00000000-0000-4000-8000-000000001009\",\"revision\":%s,\"friendCode\":\"H-AAAAAAAAQACAAAAAAAAQCQ\",\"friends\":[{\"userId\":\"00000000-0000-4000-8000-000000002009\",\"since\":\"2026-10-16T00:00:01Z\",\"lastActiveAt\":null,\"display\":{\"name\":null,\"profile\":{\"avatarId\":null,\"frameId\":null,\"badges\":[]}}}],\"incoming\":[],\"outgoing\":[],\"blocked\":[],\"limits\":{\"maxFriends\":20,\"maxIncomingRequests\":20,\"maxOutgoingRequests\":20,\"maxBlocked\":100},\"serverTime\":\"2026-10-16T00:00:02Z\"}");
    auto Parse=[](const FString& Text,FHorizonFriendsSnapshot& Out) { TSharedPtr<FJsonObject> Json; return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings) && FHorizonFriendsSnapshot::TryFromJson(Json,Out); };
    TestEqual(TEXT("Canonical H-code uses original UUID network bytes"),FHorizonFriendLookup::FriendCodeFor(User),FString(TEXT("H-AAAAAAAAQACAAAAAAAAQCQ")));
    for (const FString& Number:{FString(TEXT("9007199254740991")),FString(TEXT("9007199254740992")),FString(TEXT("9007199254740991.1")),FString(TEXT("1e3")),FString(TEXT("-1"))})
    { FHorizonFriendsSnapshot S; const bool Expected=Number==TEXT("9007199254740991"); TestEqual(TEXT("Original safe revision lexeme"),Parse(Template.Replace(TEXT("%s"),*Number),S),Expected); if (Expected) TestEqual(TEXT("Safe maximum kept exactly"),S.Revision,int64(9007199254740991LL)); }
    const FString Valid=Template.Replace(TEXT("%s"),TEXT("1")); FHorizonFriendsSnapshot S;
    TestFalse(TEXT("Missing friends array rejected"),Parse(Valid.Replace(TEXT("\"friends\":"),TEXT("\"missingFriends\":")),S));
    TestTrue(TEXT("Public unknown profile parsed safely"),Parse(Valid,S));
    TestTrue(TEXT("Null last login remains unknown, never online"),S.Friends.Num()==1 && !S.Friends[0].bHasLastActiveAt && S.Friends[0].LastActiveAt.IsEmpty() && S.Friends[0].Display.Name.IsEmpty());
    for (const FString& Bad:{Valid.Replace(TEXT("\"lastActiveAt\":null"),TEXT("\"lastActiveAt\":\"\"")),Valid.Replace(TEXT("\"name\":null"),TEXT("\"name\":true")),Valid.Replace(TEXT("\"maxFriends\":20"),TEXT("\"maxFriends\":\"20\"")),Valid.Replace(TEXT("AAAAAAAAQACAAAAAAAAQCQ"),TEXT("AAAAAAAAQACAAAAAAAAQCR"))}) TestFalse(TEXT("Malformed date/type/noncanonical code rejected"),Parse(Bad,S));
    TestTrue(TEXT("Exact UUID and canonical code accepted"),FHorizonFriendLookup::IsPlayerQuery(Peer) && FHorizonFriendLookup::IsPlayerQuery(FHorizonFriendLookup::FriendCodeFor(Peer)));
    TestFalse(TEXT("Noncanonical unused padding bits rejected"),FHorizonFriendLookup::IsPlayerQuery(TEXT("H-AAAAAAAAQACAAAAAAAAQCR")));
    auto* Realtime=NewObject<UHorizonRealtimeManager>(); TestTrue(TEXT("Own-player friend invalidations supported"),Realtime->Subscribe(TEXT("friends:00000000-0000-4000-8000-000000001009"))); Realtime->Close();
    return true;
}
#endif
