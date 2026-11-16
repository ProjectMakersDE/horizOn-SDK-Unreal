// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonItemGiftsManager.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Managers/HorizonInboxManager.h"
#include "Examples/HorizonItemGiftsExample.h"
#include "Examples/HorizonItemGiftsQuickstart.h"
#include "Examples/HorizonItemGiftJournal.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
struct FHorizonItemGiftsNative : TSharedFromThis<FHorizonItemGiftsNative>
{
    FAutomationTestBase* Test;UGameInstance* Instance;UHorizonSubsystem* S;UHorizonItemGiftsExample* Sample=nullptr;
    TArray<FString> OldHosts;FString OldKey;int32 OldRetries;float OldDelay;int32 Stage=0,Race=0;bool Pending=false;TArray<TFunction<void()>> Steps;TFunction<bool()> Wait;double Deadline=FPlatformTime::Seconds()+100;
    FHorizonItemGiftIntent Original;
    const FString Project=TEXT("10000000-0000-4000-8000-000000001140"),Sender=TEXT("20000000-0000-4000-8000-000000001140"),Recipient=TEXT("30000000-0000-4000-8000-000000001140"),Other=TEXT("30000000-0000-4000-8000-000000001141"),Item=TEXT("40000000-0000-4000-8000-000000001140"),Gift=TEXT("50000000-0000-4000-8000-000000001140"),Unknown=TEXT("60000000-0000-4000-8000-000000009999"),ReturnEntry=TEXT("message:70000000-0000-4000-8000-000000001141");
    FHorizonItemGiftsNative(FAutomationTestBase* T,const FString& Host):Test(T)
    {
        auto* C=GetMutableDefault<UHorizonConfig>();OldHosts=C->Hosts;OldKey=C->ApiKey;OldRetries=C->MaxRetryAttempts;OldDelay=C->RetryDelaySeconds;C->Hosts={Host};C->ApiKey=TEXT("gift-fixture-key");C->MaxRetryAttempts=2;C->RetryDelaySeconds=.03f;
        Instance=NewObject<UGameInstance>(GEngine);Instance->AddToRoot();Instance->InitializeStandalone();S=Instance->GetSubsystem<UHorizonSubsystem>();
    }
    void Actor(){if(Sample){Sample->RemoveFromRoot();Sample->MarkAsGarbage();}Sample=NewObject<UHorizonItemGiftsExample>();Sample->AddToRoot();Test->TestTrue(TEXT("Existing session explicitly bound"),Sample->Bind(S,Project));}
    FString File(const FString& U){return FPaths::ProjectSavedDir()/TEXT("SaveGames")/(HorizonItemGiftJournal::Name(Project,U)+TEXT(".gift"));}
    void Next(){Stage++;Pending=false;Wait=nullptr;}
    void Done(bool Ok,const FString& Code,const FString& Expected){Test->TestEqual(FString::Printf(TEXT("Item gifts stage %d success"),Stage),Ok,Expected.IsEmpty());Test->TestEqual(TEXT("Original transport/feature code"),Code,Expected);Next();}
    FOnItemGiftComplete Gifts(const FString& Expected=TEXT(""),TFunction<void(const FHorizonItemGiftResponse&)> Check=nullptr)
    {return FOnItemGiftComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared()),Expected,Check](bool Ok,const FHorizonItemGiftResponse& V,const FString& Code,const FString&){if(auto T=W.Pin()){if(Ok&&Check)Check(V);T->Done(Ok,Code,Expected);}});}
    FOnInventoryComplete Inventory(TFunction<void(const FHorizonInventorySnapshot&)> Check=nullptr)
    {return FOnInventoryComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared()),Check](bool Ok,const FHorizonInventorySnapshot& V,const FString& C,const FString&){if(auto T=W.Pin()){if(Ok&&Check)Check(V);T->Done(Ok,C,TEXT(""));}});}
    void Login(const FString& Role){S->Auth->SignInAnonymous(TEXT("gift-")+Role+TEXT("-anonymous"),FOnAuthComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared())](bool Ok){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Artificial existing identity restored, no signup"),Ok);T->Next();}}));}
    void CheckSaved(const FString& User,bool Confirmed,FHorizonItemGiftIntent* Out=nullptr)
    {
        HorizonItemGiftJournal::FLease L(Project,User);Test->TestTrue(TEXT("Original available after released asynchronous owner"),L.Locked&&L.Load());Test->TestEqual(TEXT("Minimal known phase"),!L.Original.ConfirmedGift.IsEmpty(),Confirmed);if(Out)*Out=L.Original.Intent;
        FString Raw;FFileHelper::LoadFileToString(Raw,*File(User));Test->TestFalse(TEXT("No copied fee/item response history"),Raw.Contains(TEXT("itemReference"))||Raw.Contains(TEXT("balance"))||Raw.Contains(TEXT("recordedAt")));
    }
    void Init()
    {
        Steps.Add([this]{S->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared())](bool Ok,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Loopback connection"),Ok);T->Next();}}));});
        Steps.Add([this]{Login(TEXT("sender"));});
        Steps.Add([this]{Actor();S->ItemGifts->GetConfig(Other,Gifts(TEXT("SESSION_FORBIDDEN")));});
        Steps.Add([this]{S->ItemGifts->GetGift(Sender,TEXT("bad"),Gifts(TEXT("INVALID_REQUEST")));});
        Steps.Add([this]{S->ItemGifts->Decline(Sender,Gift,-1,Gifts(TEXT("INVALID_REQUEST")));});
        Steps.Add([this]{S->ItemGifts->GetConfig(Sender,Gifts(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Fixed fee in a Player Values currency"),V.Config.Enabled&&V.Config.Fee==5&&V.Config.CurrencyKey==TEXT("1coin.gold")&&V.Limits.OpenGiftsPerPlayer==50);}));});
        Steps.Add([this]{S->ItemGifts->GetState(Sender,Gifts(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Empty sender state"),V.State.Sent.IsEmpty()&&V.State.Received.IsEmpty()&&V.State.Limits.ReceivedPageSize==50);}));});
        Steps.Add([this]{S->Items->GetInventory(Sender,Inventory());});
        Steps.Add([this]{Test->TestFalse(TEXT("Self gift is never reviewed"),Sample->ReviewSend(Sender,Item));Test->TestTrue(TEXT("Reviewed recipient, original instance and fee"),Sample->ReviewSend(Recipient,Item));Test->TestEqual(TEXT("Reviewed fee"),Sample->ReviewedFee,int64(5));Sample->ConfirmReviewedSend();const double Start=FPlatformTime::Seconds();
            Wait=[this,Start]{const double Elapsed=FPlatformTime::Seconds()-Start;if(Sample&&Elapsed>.04){HorizonItemGiftJournal::FLease Competing(Project,Sender);Test->TestFalse(TEXT("Second actor cannot enter live HTTP owner"),Competing.Locked);Sample->RemoveFromRoot();Sample->MarkAsGarbage();Sample=nullptr;}return Elapsed>.5;};});
        Steps.Add([this]{CheckSaved(Sender,false,&Original);Test->TestTrue(TEXT("New operation UUID is canonical lowercase"),Original.Operation().Equals(Original.Operation().ToLower(),ESearchCase::CaseSensitive));Actor();Sample->RecoverOriginalOperation();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{CheckSaved(Sender,true);Test->TestEqual(TEXT("Failed refresh keeps the confirmed original"),Sample->LastErrorCode,FString(TEXT("STATE_CONFLICT")));Actor();Sample->RetryPending();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{Test->TestFalse(TEXT("Confirmed recovery GETs cleared only one original"),IFileManager::Get().FileExists(*File(Sender)));Test->TestTrue(TEXT("Current inventory no longer holds the escrowed original"),S->Items->GetCurrentInventory().Instances.IsEmpty());
            S->ItemGifts->SubmitPrepared(Original,Gifts(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Replay returns the original receipt and a later gift state"),V.Replayed&&V.Receipt.GiftRevision==2&&V.HasGift&&V.Gift.Revision==3&&V.Gift.Status==TEXT("claimed"));}));});
        Steps.Add([this]{auto Changed=Original;Changed.Json=Changed.Json.Replace(TEXT("\"expectedConfigRevision\":3"),TEXT("\"expectedConfigRevision\":4"));S->ItemGifts->SubmitPrepared(Changed,Gifts(TEXT("OPERATION_CONFLICT")));});
        Steps.Add([this]{{auto I=Original;I.Json=I.Json.Replace(*I.Operation(),*Unknown);HorizonItemGiftJournal::FLease L(Project,Sender);Test->TestTrue(TEXT("Artificial unknown original persisted"),L.Begin(I));}Sample->RecoverOriginalOperation();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{CheckSaved(Sender,false);Test->TestEqual(TEXT("404 does not acknowledge or replace"),Sample->LastErrorCode,FString(TEXT("OPERATION_NOT_FOUND")));IFileManager::Get().Delete(*File(Sender));Login(TEXT("recipient"));});
        Steps.Add([this]{Test->TestFalse(TEXT("Identity switch clears the old current cache"),S->ItemGifts->GetCurrent(TEXT("state")).Present);UHorizonItemGiftsQuickstart::ReadOneGift(Sample,Gift);Wait=[this]{const auto V=S->ItemGifts->GetCurrent(TEXT("point"));return V.Present&&V.Gift.Id==Gift;};});
        Steps.Add([this]{S->Inbox->GetInbox(Recipient,FOnInboxComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared())](bool Ok,const FHorizonInboxSnapshot& V,const FString& C,const FString&){if(auto T=W.Pin()){FHorizonInboxSource Source;
            T->Test->TestTrue(TEXT("Offer entry exposes the itemGift attachment"),Ok&&V.Entries.Num()==1&&V.Entries[0].CanClaim()&&V.Entries[0].Attachment.ItemGift.GiftId==T->Gift&&V.Entries[0].TryGetSource(Source)&&Source.Kind==EHorizonInboxSourceKind::ItemGiftOffer&&Source.UserId==T->Sender);T->Done(Ok,C,TEXT(""));}}));});
        Steps.Add([this]{Test->TestFalse(TEXT("Unknown gift is not declined"),Sample->DeclineGift(Unknown));S->ItemGifts->Decline(Recipient,Gift,2,Gifts(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Declined offer returns to the sender"),!V.Replayed&&V.ReturnDelivery==TEXT("delivered")&&V.Gift.Status==TEXT("return_delivered")&&!V.Gift.DeclinedAt.IsEmpty()&&V.Gift.ReturnEntryId==ReturnEntry);}));});
        Steps.Add([this]{S->ItemGifts->Decline(Recipient,Gift,2,Gifts(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Repeated decline replays"),V.Replayed);}));});
        Steps.Add([this]{Login(TEXT("sender"));});
        Steps.Add([this]{S->Inbox->Claim(Sender,ReturnEntry,FOnInboxClaimComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared())](bool Ok,const FHorizonInboxClaim& V,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Return claimed through the existing Inbox claim"),Ok&&V.Items.Num()==1&&V.Items[0].InstanceId==T->Item);T->Done(Ok,C,TEXT(""));}}));});
        Steps.Add([this]{S->Items->GetInventory(Sender,Inventory([this](const auto& V){Test->TestTrue(TEXT("Original instance back, read from inventory"),V.Instances.Num()==1&&V.Instances[0].InstanceId==Item);}));});
        Steps.Add([this]{S->ItemGifts->GetGift(Sender,Gift,Gifts(TEXT(""),[this](const auto& V){Test->TestEqual(TEXT("Terminal returned gift"),V.Gift.Status,FString(TEXT("returned")));}));});
        Steps.Add([this]{S->ItemGifts->GetConfig(Sender,Gifts(TEXT("INVALID_RESPONSE")));});
        Steps.Add([this]{S->ItemGifts->GetState(Sender,Gifts());});
        Steps.Add([this]{S->ItemGifts->GetState(Sender,Gifts(TEXT("STALE_RESPONSE")));});
        Steps.Add([this]{Race=0;S->ItemGifts->GetState(Sender,FOnItemGiftComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared())](bool Ok,const auto&,const FString&,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("First read owns the request"),Ok);if(++T->Race==2)T->Next();}}));
            S->ItemGifts->GetConfig(Sender,FOnItemGiftComplete::CreateLambda([W=TWeakPtr<FHorizonItemGiftsNative>(AsShared())](bool Ok,const auto&,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Concurrent request refused without HTTP"),!Ok&&C==TEXT("REQUEST_IN_PROGRESS"));if(++T->Race==2)T->Next();}}));});
    }
    void Cleanup()
    {
        if(Sample){Sample->RemoveFromRoot();Sample->MarkAsGarbage();Sample=nullptr;}for(const auto& U:{Sender,Recipient}){IFileManager::Get().Delete(*File(U));IFileManager::Get().Delete(*(File(U)+TEXT(".next")));}
        S->Auth->SignOut();S->GetHttpClient()->Disconnect();auto* World=Instance->GetWorld();Instance->Shutdown();World->DestroyWorld(false);GEngine->DestroyWorldContext(World);Instance->RemoveFromRoot();auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts=OldHosts;C->ApiKey=OldKey;C->MaxRetryAttempts=OldRetries;C->RetryDelaySeconds=OldDelay;
    }
    bool Update(){if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("Item gifts timeout stage%d"),Stage));Cleanup();return true;}if(Wait){if(Wait())Next();return false;}if(Pending)return false;if(Stage>=Steps.Num()){Cleanup();return true;}Pending=true;Steps[Stage]();return false;}
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonItemGifts,TSharedPtr<FHorizonItemGiftsNative>,State);
bool FWaitForHorizonItemGifts::Update(){return State->Update();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonItemGiftsTransportTest,"Horizon.ItemGifts.NativeTransport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonItemGiftsTransportTest::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_ITEM_GIFTS_FIXTURE_URL"));if(Host.IsEmpty()){AddWarning(TEXT("Set isolated native item gifts fixture URL"));return true;}if(!Host.StartsWith(TEXT("http://127.0.0.1:"))){AddError(TEXT("Loopback only"));return false;}
    for(const TCHAR* N:{TEXT("ReadConfig"),TEXT("ReadState"),TEXT("ReadGift"),TEXT("SubmitOriginal"),TEXT("LookupOriginal"),TEXT("DeclineGift"),TEXT("PrepareSend")})TestNotNull(TEXT("Blueprint item gifts surface"),UHorizonItemGiftsManager::StaticClass()->FindFunctionByName(N));
    auto State=MakeShared<FHorizonItemGiftsNative>(this,Host);State->Init();ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonItemGifts(State));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonItemGiftsModelsTest,"Horizon.ItemGifts.StrictModels",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonItemGiftsModelsTest::RunTest(const FString&)
{
    const FString User=TEXT("20000000-0000-4000-8000-000000001140"),Operation=TEXT("60000000-0000-4000-8000-000000001140"),Recipient=TEXT("30000000-0000-4000-8000-000000001140"),Instance=TEXT("40000000-0000-4000-8000-000000001140");
    const auto I=UHorizonItemGiftsManager::PrepareSend(User,Operation,TEXT("2026-11-16T10:00:00.000Z"),Recipient,Instance,9,3);TestTrue(TEXT("Valid exact original with seven fields"),I.IsValid());
    for(const TCHAR* N:{TEXT("1e0"),TEXT("1.0"),TEXT("-0"),TEXT("9007199254740992")}){auto Bad=I;Bad.Json=I.Json.Replace(TEXT("\"expectedInventoryRevision\":9"),*(FString(TEXT("\"expectedInventoryRevision\":"))+N));TestFalse(TEXT("Unsafe original tokens rejected"),Bad.IsValid());}
    TestFalse(TEXT("Self gift rejected before HTTP"),UHorizonItemGiftsManager::PrepareSend(User,Operation,TEXT("2026-11-16T10:00:00.000Z"),User,Instance,9,3).IsValid());
    TestFalse(TEXT("Uppercase recipient UUID rejected"),UHorizonItemGiftsManager::PrepareSend(User,Operation,TEXT("2026-11-16T10:00:00.000Z"),TEXT("ABCDEFAB-CDEF-4ABC-8DEF-ABCDEFABCDEF"),Instance,9,3).IsValid());
    TestFalse(TEXT("Impossible date rejected"),UHorizonItemGiftsManager::PrepareSend(User,Operation,TEXT("2026-02-30T10:00:00.000Z"),Recipient,Instance,9,3).IsValid());
    auto Extra=I;Extra.Json=I.Json.Replace(TEXT("{"),TEXT("{\"price\":1,"));TestFalse(TEXT("Unknown field rejected"),Extra.IsValid());
    const FString GiftJson=TEXT(R"GIFT({"id":"50000000-0000-4000-8000-000000001140","revision":2,"senderId":"20000000-0000-4000-8000-000000001140","recipientId":"30000000-0000-4000-8000-000000001140","returnOwnerId":"20000000-0000-4000-8000-000000001140","itemReference":{"instanceId":"40000000-0000-4000-8000-000000001140","itemKey":"hat"},"currencyKey":"1coin.gold","fee":5,"configRevision":3,"status":"offered","createdAt":"2026-11-16T10:00:00.000Z","expiresAt":"2026-11-23T10:00:00.000Z","terminalAt":null,"declinedAt":null,"offerEntryId":"message:70000000-0000-4000-8000-000000001140","returnEntryId":null,"ownerUnavailable":false})GIFT");
    const FString ReceiptJson=TEXT(R"GIFT({"operationId":"60000000-0000-4000-8000-000000001140","issuedAt":"2026-11-16T10:00:00.000Z","giftId":"50000000-0000-4000-8000-000000001140","giftRevision":2,"recordedAt":"2026-11-16T10:00:00.000Z","recipientId":"30000000-0000-4000-8000-000000001140","itemReference":{"instanceId":"40000000-0000-4000-8000-000000001140","itemKey":"hat"},"currencyKey":"1coin.gold","fee":5,"configRevision":3})GIFT");
    const FString Written=TEXT("{\"receipt\":")+ReceiptJson+TEXT(",\"gift\":")+GiftJson+TEXT(",\"replayed\":false,\"serverTime\":\"2026-11-16T10:00:00.000Z\"}");FHorizonItemGiftResponse V;
    TestTrue(TEXT("New send receipt with its offered gift"),FHorizonItemGiftResponse::TryFromJson(Written,TEXT("written"),V)&&I.Matches(V.Receipt));
    TestTrue(TEXT("Replay after retention without gift"),FHorizonItemGiftResponse::TryFromJson(Written.Replace(*GiftJson,TEXT("null")).Replace(TEXT("\"replayed\":false"),TEXT("\"replayed\":true")),TEXT("written"),V)&&!V.HasGift);
    for(const FString& Bad:{Written.Replace(*GiftJson,TEXT("null")),Written.Replace(TEXT("\"fee\":5,\"configRevision\":3}"),TEXT("\"fee\":6,\"configRevision\":3}")),Written.Replace(TEXT("\"currencyKey\":\"1coin.gold\",\"fee\":5"),TEXT("\"currencyKey\":null,\"fee\":5")),Written.Replace(TEXT("\"status\":\"offered\""),TEXT("\"status\":\"lost\"")),Written.Replace(TEXT("\"replayed\":false"),TEXT("\"replayed\":false,\"replayed\":true"))})
        TestFalse(TEXT("Inconsistent fee, status, duplicate key or missing new gift rejected"),FHorizonItemGiftResponse::TryFromJson(Bad,TEXT("written"),V));
    auto Other=FHorizonItemGiftReceipt(V.Receipt);Other.RecipientId=User;TestFalse(TEXT("Receipt for another recipient does not match the original"),I.Matches(Other));
    const FString Free=GiftJson.Replace(TEXT("\"currencyKey\":\"1coin.gold\",\"fee\":5"),TEXT("\"currencyKey\":null,\"fee\":0"));
    TestTrue(TEXT("Free gift has a null currency"),FHorizonItemGiftResponse::TryFromJson(TEXT("{\"gift\":")+Free+TEXT(",\"serverTime\":\"2026-11-16T10:00:00.000Z\"}"),TEXT("point"),V)&&V.Gift.CurrencyKey.IsEmpty()&&V.Gift.Fee==0);
    const FString Declined=GiftJson.Replace(TEXT("\"status\":\"offered\""),TEXT("\"status\":\"return_pending\"")).Replace(TEXT("\"declinedAt\":null"),TEXT("\"declinedAt\":\"2026-11-16T11:00:00.000Z\""));
    TestTrue(TEXT("Declined offer awaiting return"),FHorizonItemGiftResponse::TryFromJson(TEXT("{\"gift\":")+Declined+TEXT(",\"replayed\":false,\"returnDelivery\":\"pending\",\"serverTime\":\"2026-11-16T11:00:00.000Z\"}"),TEXT("declined"),V)&&V.ReturnDelivery==TEXT("pending"));
    TestFalse(TEXT("Decline result needs declinedAt"),FHorizonItemGiftResponse::TryFromJson(TEXT("{\"gift\":")+GiftJson.Replace(TEXT("\"status\":\"offered\""),TEXT("\"status\":\"return_pending\""))+TEXT(",\"replayed\":false,\"returnDelivery\":\"pending\",\"serverTime\":\"2026-11-16T11:00:00.000Z\"}"),TEXT("declined"),V));
    TestFalse(TEXT("Terminal status needs terminalAt"),FHorizonItemGiftResponse::TryFromJson(TEXT("{\"gift\":")+GiftJson.Replace(TEXT("\"status\":\"offered\""),TEXT("\"status\":\"claimed\""))+TEXT(",\"serverTime\":\"2026-11-16T10:00:00.000Z\"}"),TEXT("point"),V));
    const FString Limits=TEXT("{\"openGiftsPerPlayer\":3,\"retainedGiftsPerProject\":50,\"projectWritesPerMinute\":10,\"historyPerPlayer\":20,\"receivedPageSize\":50,\"playerWriteIntervalMillis\":1000,\"expirySeconds\":604800,\"terminalRetentionSeconds\":604800,\"operationRetentionSeconds\":2592000}");
    const FString StateJson=TEXT("{\"state\":{\"userId\":\"30000000-0000-4000-8000-000000001140\",\"revision\":4,\"sent\":[],\"history\":[],\"received\":[")+GiftJson+TEXT("],\"operationWindow\":{\"serverTime\":\"2026-11-16T10:00:00.000Z\",\"acceptAfter\":\"2026-10-17T10:00:00.000Z\"},\"limits\":")+Limits+TEXT("},\"serverTime\":\"2026-11-16T10:00:00.000Z\"}");
    TestTrue(TEXT("Recipient state lists the open offer"),FHorizonItemGiftResponse::TryFromJson(StateJson,TEXT("state"),V)&&V.State.Received.Num()==1&&V.State.Limits.OpenGiftsPerPlayer==3);
    TestFalse(TEXT("Received list holds only own open offers"),FHorizonItemGiftResponse::TryFromJson(StateJson.Replace(TEXT("\"userId\":\"30000000"),TEXT("\"userId\":\"31000000")),TEXT("state"),V));
    TestFalse(TEXT("Sent list holds only own open gifts"),FHorizonItemGiftResponse::TryFromJson(StateJson.Replace(TEXT("\"sent\":[],"),*(TEXT("\"sent\":[")+GiftJson+TEXT("],"))),TEXT("state"),V));
    const FString Config=TEXT("{\"config\":{\"revision\":3,\"enabled\":true,\"currencyKey\":null,\"fee\":0},\"limits\":")+Limits+TEXT(",\"serverTime\":\"2026-11-16T10:00:00.000Z\"}");
    TestTrue(TEXT("Free config"),FHorizonItemGiftResponse::TryFromJson(Config,TEXT("config"),V)&&V.Config.Enabled&&V.Config.CurrencyKey.IsEmpty());
    TestFalse(TEXT("A fee without currency is refused"),FHorizonItemGiftResponse::TryFromJson(Config.Replace(TEXT("\"fee\":0"),TEXT("\"fee\":5")),TEXT("config"),V));
    return true;
}
#endif
