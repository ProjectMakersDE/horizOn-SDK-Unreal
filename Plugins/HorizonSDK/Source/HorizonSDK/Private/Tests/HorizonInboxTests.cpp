// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonInboxManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "HorizonConfig.h"
#include "HorizonBlueprintLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

struct FHorizonInboxNativeState : TSharedFromThis<FHorizonInboxNativeState>
{
    FAutomationTestBase* Test;
    UHorizonHttpClient* Http;
    UHorizonAuthManager* Auth;
    UHorizonPlayerValuesManager* Values;
    UHorizonInboxManager* Inbox;
    TArray<FString> OriginalHosts;
    FString OriginalKey;
    int32 OriginalRetries;
    float OriginalDelay;
    int32 Stage=0, BurstCallbacks=0;
    bool bPending=false;
    double Deadline=FPlatformTime::Seconds()+60;
    const FString User=TEXT("00000000-0000-4000-8000-000000001006");
    const FString Entry=TEXT("broadcast:00000000-0000-4000-8000-000000001006");
    const FString ItemEntry=TEXT("message:00000000-0000-4000-8000-000000001007");
    FHorizonInboxNativeState(FAutomationTestBase* InTest,const FString& Host):Test(InTest)
    {
        auto* Config=GetMutableDefault<UHorizonConfig>();
        OriginalHosts=Config->Hosts; OriginalKey=Config->ApiKey; OriginalRetries=Config->MaxRetryAttempts; OriginalDelay=Config->RetryDelaySeconds;
        Config->Hosts={Host}; Config->ApiKey=TEXT("fixture-key-1006"); Config->MaxRetryAttempts=2; Config->RetryDelaySeconds=.1f;
        Http=NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth=NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        Values=NewObject<UHorizonPlayerValuesManager>(); Values->AddToRoot(); Values->Initialize(Http,Auth);
        Inbox=NewObject<UHorizonInboxManager>(); Inbox->AddToRoot(); Inbox->Initialize(Http,Auth,Values);
    }
    void Cleanup()
    {
        Auth->SignOut(); Http->Disconnect(); Inbox->RemoveFromRoot(); Values->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot();
        auto* Config=GetMutableDefault<UHorizonConfig>(); Config->Hosts=OriginalHosts; Config->ApiKey=OriginalKey; Config->MaxRetryAttempts=OriginalRetries; Config->RetryDelaySeconds=OriginalDelay;
    }
    void Next() { Stage++; bPending=false; }
    FOnInboxComplete ReadCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonInboxNativeState> Weak=AsShared();
        return FOnInboxComplete::CreateLambda([Weak,Sent](bool Success,const FHorizonInboxSnapshot& Snapshot,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                const bool Expected=Sent==4 || Sent==5 || Sent==6 || Sent==8 || Sent==9 || Sent==10 || Sent==17 || Sent==22;
                Self->Test->TestEqual(FString::Printf(TEXT("Inbox result stage %d"),Sent),Success,Expected);
                const FString ExpectedCode=Expected?TEXT(""):Sent==18?TEXT("UNCLAIMED_REWARD"):TEXT("STALE_RESPONSE");
                Self->Test->TestEqual(TEXT("Typed stable error"),Code,ExpectedCode);
                if (Expected) Self->Test->TestEqual(TEXT("Materialized entry retained"),Snapshot.Entries.Num(),Sent==22?552:1);
                if (Sent==22 && Snapshot.Entries.Num()==552)
                {
                    FString Text; FHorizonInboxSource Source;
                    Self->Test->TestTrue(TEXT("Native UTF8 free text"),Snapshot.Entries[0].TryGetFreeText(Text) && FTCHARToUTF8(*Text).Length()==512);
                    Self->Test->TestEqual(TEXT("Configured text bytes"),Snapshot.Limits.FreeTextBytes,512);
                    Self->Test->TestTrue(TEXT("Native item-only return"),Snapshot.Entries[1].CanClaim() && Snapshot.Entries[1].TryGetSource(Source) && Source.Kind==EHorizonInboxSourceKind::MarketReturn);
                }
                if (Sent==5) Self->Test->TestTrue(TEXT("Unchanged wire reply materialized"),Snapshot.bUnchanged);
                if (Sent==6) Self->Test->TestEqual(TEXT("Missing materialization triggers exactly one full reload"),Snapshot.Revision,int64(11));
                if (Sent==7) Self->Test->TestFalse(TEXT("ClearCache retains revision watermark"),Self->Inbox->GetCurrentInbox().bPresent);
                if (Sent==9)
                {
                    Self->BurstCallbacks++;
                    Self->Test->TestEqual(TEXT("Burst callback revision"),Snapshot.Revision,int64(Self->BurstCallbacks==1?13:14));
                    if (Self->BurstCallbacks<3) return;
                }
                if (Sent==18) Self->Test->TestEqual(TEXT("Valuable removal rejection preserves snapshot"),Self->Inbox->GetCurrentInbox().Revision,int64(17));
                if (Sent==19 || Sent==20) Self->Test->TestFalse(TEXT("Late reply cannot refill cleared/signed-out cache"),Self->Inbox->GetCurrentInbox().bPresent);
                Self->Next();
            }
        });
    }
    FOnInboxClaimComplete ClaimCallback()
    {
        const int32 Sent=Stage; const TWeakPtr<FHorizonInboxNativeState> Weak=AsShared();
        return FOnInboxClaimComplete::CreateLambda([Weak,Sent](bool Success,const FHorizonInboxClaim& Claim,const FString& Code,const FString&)
        {
            if (auto Self=Weak.Pin())
            {
                Self->Test->TestEqual(TEXT("Claim epoch result"),Success,Sent!=13);
                Self->Test->TestEqual(TEXT("Claim stable error"),Code,Sent==13?FString(TEXT("STALE_RESPONSE")):FString());
                if (Success)
                {
                    Self->Test->TestEqual(TEXT("Original entry identity"),Claim.EntryId,Sent>=23?Self->ItemEntry:Self->Entry);
                    Self->Test->TestEqual(TEXT("Server replay marker"),Claim.bReplayed,Sent==12 || Sent==24);
                    if (Sent>=23) Self->Test->TestTrue(TEXT("Item references retained on claim and replay"),Claim.Items.Num()==1 && Claim.Items[0].InstanceId==Self->User && Claim.Items[0].ItemKey==TEXT("sword"));
                    const auto State=Self->Values->GetCurrentState();
                    Self->Test->TestEqual(TEXT("Full authoritative balance, never local delta"),State.Values.Num()==1?State.Values[0].Balance:int64(-1),int64(100));
                }
                else
                {
                    Self->Test->TestFalse(TEXT("PV ClearCache fences in-flight grant"),Self->Values->GetCurrentState().bPresent);
                    Self->Test->TestEqual(TEXT("Rejected grant does not advance inbox"),Self->Inbox->GetCurrentInbox().Revision,int64(16));
                }
                Self->Next();
            }
        });
    }
    bool Update()
    {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Inbox fixture timeout stage %d"),Stage)); Cleanup(); return true; }
        if (bPending) return false;
        bPending=true; const TWeakPtr<FHorizonInboxNativeState> Weak=AsShared();
        if (Stage==0) Http->ConnectToServer(FOnRequestComplete::CreateLambda([Weak](bool Ok,const FString&) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Loopback connected"),Ok); Self->Next(); } }));
        else if (Stage==1 || Stage==2) Auth->SignUpAnonymous(Stage==1?TEXT("signup-503"):TEXT("signup-drop"),FOnAuthComplete::CreateLambda([Weak](bool Ok) { if (auto Self=Weak.Pin()) { Self->Test->TestFalse(TEXT("Uncertain signup fails without automatic second identity"),Ok); Self->Next(); } }));
        else if (Stage==3 || Stage==21) Auth->SignInAnonymous(TEXT("fixture-anonymous"),FOnAuthComplete::CreateLambda([Weak](bool Ok) { if (auto Self=Weak.Pin()) { Self->Test->TestTrue(TEXT("Existing identity signed in"),Ok); Self->Next(); } }));
        else if (Stage==5) Inbox->GetInbox(User,ReadCallback(),TEXT("cursor_10_20"));
        else if (Stage==6) { Inbox->ClearCache(); Inbox->GetInbox(User,ReadCallback(),TEXT("cursor_10_20")); }
        else if (Stage==7) { Inbox->ClearCache(); Inbox->GetInbox(User,ReadCallback()); }
        else if (Stage==9) { const auto Callback=ReadCallback(); Inbox->GetInbox(User,Callback); Inbox->GetInbox(User,Callback); Inbox->GetInbox(User,Callback); }
        else if (Stage==10 || Stage==19) { Inbox->MarkRead(User,Entry,ReadCallback()); if (Stage==19) Inbox->ClearCache(); }
        else if (Stage>=11 && Stage<=13) { Inbox->Claim(User,Entry,ClaimCallback()); if (Stage==13) Values->ClearCache(); }
        else if (Stage==18) Inbox->Remove(User,Entry,ReadCallback());
        else if (Stage<=20) { Inbox->GetInbox(User,ReadCallback()); if (Stage==20) Auth->SignOut(); }
        else if (Stage==22) Inbox->GetInbox(User,ReadCallback());
        else if (Stage==23 || Stage==24) Inbox->Claim(User,ItemEntry,ClaimCallback());
        else
        {
            Auth->SignOut();
            bool bRejected=false;
            Inbox->Claim(User,Entry,FOnInboxClaimComplete::CreateLambda([&](bool Ok,const FHorizonInboxClaim&,const FString& Code,const FString&) { bRejected=!Ok && Code==TEXT("SESSION_REQUIRED"); }));
            Test->TestTrue(TEXT("Signed-out claim rejected before HTTP"),bRejected);
            Cleanup(); return true;
        }
        return false;
    }
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonInbox,TSharedPtr<FHorizonInboxNativeState>,State);
bool FWaitForHorizonInbox::Update() { return State->Update(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonInboxTransportTest,"Horizon.Inbox.NativeTransport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonInboxTransportTest::RunTest(const FString& Parameters)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_INBOX_FIXTURE_URL"));
    if (Host.IsEmpty()) { AddWarning(TEXT("Set loopback HORIZON_INBOX_FIXTURE_URL for native transport proof")); return true; }
    if (!Host.StartsWith(TEXT("http://127.0.0.1:"))) { AddError(TEXT("Only artificial loopback fixture accepted")); return false; }
    TestNotNull(TEXT("Blueprint Inbox request"),UHorizonInboxManager::StaticClass()->FindFunctionByName(TEXT("RequestInbox")));
    TestNotNull(TEXT("Blueprint explicit claim"),UHorizonInboxManager::StaticClass()->FindFunctionByName(TEXT("ClaimEntry")));
    TestNotNull(TEXT("Blueprint typed source"),UHorizonBlueprintLibrary::StaticClass()->FindFunctionByName(TEXT("GetInboxSource")));
    TestNotNull(TEXT("Blueprint plain text"),UHorizonBlueprintLibrary::StaticClass()->FindFunctionByName(TEXT("GetInboxFreeText")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonInbox(MakeShared<FHorizonInboxNativeState>(this,Host)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonInboxModelTest,"Horizon.Inbox.StrictModels",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonInboxModelTest::RunTest(const FString& Parameters)
{
    const FString Template=TEXT("{\"userId\":\"00000000-0000-4000-8000-000000001006\",\"cursor\":\"opaque_cursor\",\"revision\":%s,\"broadcastRevision\":1,\"serverTime\":\"2026-10-16T00:00:10Z\",\"pollAfterSeconds\":30,\"unchanged\":false,\"entries\":[{\"id\":\"message:00000000-0000-4000-8000-000000001006\",\"kind\":\"message\",\"type\":\"reward\",\"reference\":\"\",\"textKey\":\"reward\",\"parameters\":{},\"createdAt\":\"2026-10-16T00:00:00Z\",\"expiresAt\":\"2026-10-17T00:00:00Z\",\"overdue\":false,\"read\":false,\"claimed\":false,\"attachment\":{\"values\":[{\"key\":\"gold\",\"amount\":7}]}}],\"counts\":{\"reward\":{\"total\":1,\"unread\":1,\"claimable\":1}},\"limits\":{\"targeted\":50,\"broadcasts\":500}}");
    auto Parse=[&](const FString& Text,FHorizonInboxSnapshot& Snapshot) { TSharedPtr<FJsonObject> Json; return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json,FJsonSerializer::EFlags::StoreNumbersAsStrings) && FHorizonInboxSnapshot::TryFromJson(Json,Snapshot); };
    for (const FString& Lexeme:{FString(TEXT("9007199254740991")),FString(TEXT("9007199254740992")),FString(TEXT("9007199254740991.1")),FString(TEXT("1e3")),FString(TEXT("-1"))})
    {
        FHorizonInboxSnapshot Snapshot; const bool Expected=Lexeme==TEXT("9007199254740991");
        TestEqual(TEXT("Revision requires original safe integer token"),Parse(Template.Replace(TEXT("%s"),*Lexeme),Snapshot),Expected);
        if (Expected) TestEqual(TEXT("Safe integer preserved"),Snapshot.Revision,int64(9007199254740991LL));
    }
    const FString Valid=Template.Replace(TEXT("%s"),TEXT("1"));
    for (const FString& Invalid:{Valid.Replace(TEXT("\"parameters\":{},"),TEXT("")),Valid.Replace(TEXT("\"parameters\":{}"),TEXT("\"parameters\":null")),Valid.Replace(TEXT("\"parameters\":{}"),TEXT("\"parameters\":[]"))})
    { FHorizonInboxSnapshot Snapshot; TestFalse(TEXT("Missing or non-object parameters rejected"),Parse(Invalid,Snapshot)); }
    FHorizonInboxSnapshot LongPolling;
    TestTrue(TEXT("Full positive server int32 polling interval accepted"),Parse(Valid.Replace(TEXT("\"pollAfterSeconds\":30"),TEXT("\"pollAfterSeconds\":2147483647")),LongPolling));
    TestFalse(TEXT("Polling overflow rejected"),Parse(Valid.Replace(TEXT("\"pollAfterSeconds\":30"),TEXT("\"pollAfterSeconds\":2147483648")),LongPolling));
    for (const FString& Invalid:{Valid.Replace(TEXT("\"amount\":7"),TEXT("\"amount\":9007199254740991.1")),Valid.Replace(TEXT("\"total\":1"),TEXT("\"total\":2")),Valid.Replace(TEXT("\"kind\":\"message\""),TEXT("\"kind\":\"broadcast\"")),Valid.Replace(TEXT("\"unchanged\":false"),TEXT("\"unchanged\":true"))})
    { FHorizonInboxSnapshot Snapshot; TestFalse(TEXT("Malformed reward, counts, identity and unchanged body rejected"),Parse(Invalid,Snapshot)); }
    FHorizonInboxSnapshot Configured;
    TestTrue(TEXT("Admin limits above prior defaults"),Parse(Valid.Replace(TEXT("\"targeted\":50,\"broadcasts\":500"),TEXT("\"targeted\":1000,\"broadcasts\":2000,\"freeTexts\":20,\"freeTextBytes\":512,\"freeTextRetentionDays\":30")),Configured));
    TestEqual(TEXT("Configured free text size preserved"),Configured.Limits.FreeTextBytes,512);
    const FString User=TEXT("00000000-0000-4000-8000-000000001006");
    auto One=[&](const FString& Type,const FString& Key,const FString& Parameter,const FString& Attachment)
    {
        return Valid.Replace(TEXT("\"type\":\"reward\""),*(TEXT("\"type\":\"")+Type+TEXT("\"")))
            .Replace(TEXT("\"textKey\":\"reward\""),*(TEXT("\"textKey\":\"")+Key+TEXT("\"")))
            .Replace(TEXT("\"reference\":\"\""),*(TEXT("\"reference\":\"")+User+TEXT("\"")))
            .Replace(TEXT("\"parameters\":{}"),*(TEXT("\"parameters\":")+Parameter))
            .Replace(TEXT("\"attachment\":{\"values\":[{\"key\":\"gold\",\"amount\":7}]}"),*(TEXT("\"attachment\":")+Attachment))
            .Replace(TEXT("\"counts\":{\"reward\":"),*(TEXT("\"counts\":{\"")+Type+TEXT("\":")))
            .Replace(TEXT("\"claimable\":1"),Attachment==TEXT("null")?TEXT("\"claimable\":0"):TEXT("\"claimable\":1"));
    };
    struct FSourceCase { const TCHAR* Type; const TCHAR* Key; const TCHAR* Parameter; EHorizonInboxSourceKind Kind; };
    const FSourceCase Sources[]={
        {TEXT("friend_request"),TEXT("horizon.friends.request"),TEXT("userId"),EHorizonInboxSourceKind::FriendRequest},
        {TEXT("friend_accept"),TEXT("horizon.friends.accept"),TEXT("userId"),EHorizonInboxSourceKind::FriendAccepted},
        {TEXT("group_invite"),TEXT("horizon.groups.invite"),TEXT("groupId"),EHorizonInboxSourceKind::GroupInvitation},
        {TEXT("group_request"),TEXT("horizon.groups.request"),TEXT("groupId"),EHorizonInboxSourceKind::GroupRequest},
        {TEXT("group_accept-invite"),TEXT("horizon.groups.accept-invite"),TEXT("groupId"),EHorizonInboxSourceKind::GroupInvitationAccepted},
        {TEXT("group_accept-request"),TEXT("horizon.groups.accept-request"),TEXT("groupId"),EHorizonInboxSourceKind::GroupRequestAccepted},
        {TEXT("match_turn"),TEXT("horizon.matches.your_turn"),TEXT("matchId"),EHorizonInboxSourceKind::MatchTurn}};
    for (const auto& Case:Sources)
    {
        FString Params=FString::Printf(TEXT("{\"%s\":\"%s\"%s}"),Case.Parameter,*User,Case.Kind==EHorizonInboxSourceKind::MatchTurn?TEXT(",\"revision\":\"9007199254740991\""):TEXT(""));
        FHorizonInboxSnapshot Snapshot; FHorizonInboxSource Source;
        const FString Json=One(Case.Type,Case.Key,Params,TEXT("null"));
        TestTrue(TEXT("Known typed source remains bound"),Parse(Json,Snapshot) && Snapshot.Entries[0].TryGetSource(Source) && Source.Kind==Case.Kind && Source.SourceId==User);
        TestFalse(TEXT("Malformed source ID refused"),Parse(Json.Replace(*(TEXT("\"reference\":\"")+User+TEXT("\"")),TEXT("\"reference\":\"bad\"")),Snapshot));
    }
    const FString Return=TEXT("{\"values\":[],\"itemReturn\":{\"listingId\":\"")+User+TEXT("\",\"instanceId\":\"")+User+TEXT("\",\"itemKey\":\"sword\"}}");
    FHorizonInboxSnapshot Legacy; FHorizonInboxSource LegacySource;
    const FString Acceptance=One(TEXT("group_accept-request"),TEXT("horizon.groups.accept-request"),TEXT("{\"groupId\":\"")+User+TEXT("\"}"),TEXT("null"))
        .Replace(*(TEXT("\"reference\":\"")+User+TEXT("\"")),TEXT("\"reference\":\"00000000-0000-4000-8000-000000001007\""));
    TestTrue(TEXT("Legacy acceptance retains request reference and group navigation"),Parse(Acceptance,Legacy) && Legacy.Entries[0].TryGetSource(LegacySource) && LegacySource.GroupId==User && LegacySource.SourceId!=User);
    const FString Market=One(TEXT("marketplace"),TEXT("marketplace.item_return"),TEXT("{}"),Return);
    FHorizonInboxSnapshot ItemOnly;
    TestTrue(TEXT("Item-only return counts as claimable"),Parse(Market,ItemOnly) && ItemOnly.Entries[0].CanClaim() && ItemOnly.Entries[0].Attachment.Values.IsEmpty());
    for (const FString& Bad:{Market.Replace(TEXT("\"values\":[]"),TEXT("\"values\":[],\"unknownReward\":1")),Market.Replace(TEXT("\"itemKey\":\"sword\""),TEXT("\"itemKey\":\"sword\",\"unknownReward\":1")),Valid.Replace(TEXT("\"amount\":7"),TEXT("\"amount\":7,\"unknownReward\":1"))})
    { FHorizonInboxSnapshot Snapshot; TestFalse(TEXT("Unknown valuable fields refused"),Parse(Bad,Snapshot)); }
    const FString Text=FString::ChrN(256,TCHAR(0x00e4));
    FString FreeText=One(TEXT("free_text"),TEXT("horizon.inbox.free_text"),TEXT("{\"text\":\"")+Text+TEXT("\"}"),TEXT("null")).Replace(TEXT("2026-10-16T00:00:00Z"),TEXT("2030-01-01T00:00:00Z"));
    FHorizonInboxSnapshot Plain; FString Decoded;
    TestTrue(TEXT("512B plain text remains readable across reversed clock dates"),Parse(FreeText,Plain) && Plain.Entries[0].TryGetFreeText(Decoded) && Decoded==Text);
    TSharedPtr<FJsonObject> Base; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Valid),Base,FJsonSerializer::EFlags::StoreNumbersAsStrings);
    FString Row; auto Writer=TJsonWriterFactory<>::Create(&Row); FJsonSerializer::Serialize(Base->GetArrayField(TEXT("entries"))[0]->AsObject().ToSharedRef(),Writer);
    TArray<FString> Rows; for (int32 I=0;I<551;I++) Rows.Add(Row.Replace(*User,*FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens)));
    const int32 Start=Valid.Find(TEXT("\"entries\":"))+10; const int32 End=Valid.Find(TEXT(",\"counts\":"));
    const FString Retained=(Valid.Left(Start)+TEXT("[")+FString::Join(Rows,TEXT(","))+TEXT("]")+Valid.Mid(End)).Replace(TEXT("\"total\":1,\"unread\":1,\"claimable\":1"),TEXT("\"total\":551,\"unread\":551,\"claimable\":551")).Replace(TEXT("\"targeted\":50,\"broadcasts\":500"),TEXT("\"targeted\":1,\"broadcasts\":1"));
    FHorizonInboxSnapshot Old;
    TestTrue(TEXT("Retained551 entries survive lowered limits"),Parse(Retained,Old) && Old.Entries.Num()==551);
    return true;
}
#endif
