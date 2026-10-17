#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonMarketplaceManager.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Managers/HorizonInboxManager.h"
#include "Examples/HorizonMarketplaceExample.h"
#include "Examples/HorizonMarketplaceQuickstart.h"
#include "Examples/HorizonMarketplaceJournal.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
struct FHorizonMarketplaceNative : TSharedFromThis<FHorizonMarketplaceNative>
{
    FAutomationTestBase* Test;UGameInstance* Instance;UHorizonSubsystem* S;UHorizonMarketplaceExample* Sample=nullptr;
    TArray<FString> OldHosts;FString OldKey,Cursor;int32 OldRetries;float OldDelay;int32 Stage=0,Race=0;bool Pending=false;TArray<TFunction<void()>> Steps;TFunction<bool()> Wait;double Deadline=FPlatformTime::Seconds()+100;
    FHorizonMarketplaceIntent Original,BuyOriginal;FHorizonItemInstance OriginalItem;
    const FString Project=TEXT("10000000-0000-4000-8000-000000001015"),Seller=TEXT("20000000-0000-4000-8000-000000001015"),Buyer=TEXT("30000000-0000-4000-8000-000000001015"),Other=TEXT("30000000-0000-4000-8000-000000001016"),Item=TEXT("40000000-0000-4000-8000-000000001015"),Listing=TEXT("50000000-0000-4000-8000-000000001015"),L2=TEXT("50000000-0000-4000-8000-000000001016"),L3=TEXT("50000000-0000-4000-8000-000000001017"),I3=TEXT("40000000-0000-4000-8000-000000001017"),Unknown=TEXT("60000000-0000-4000-8000-000000009999"),Entry=TEXT("message:70000000-0000-3000-8000-000000001015");
    FHorizonMarketplaceNative(FAutomationTestBase* T,const FString& Host):Test(T)
    {
        auto* C=GetMutableDefault<UHorizonConfig>();OldHosts=C->Hosts;OldKey=C->ApiKey;OldRetries=C->MaxRetryAttempts;OldDelay=C->RetryDelaySeconds;C->Hosts={Host};C->ApiKey=TEXT("market-fixture-key");C->MaxRetryAttempts=2;C->RetryDelaySeconds=.03f;
        Instance=NewObject<UGameInstance>(GEngine);Instance->AddToRoot();Instance->InitializeStandalone();S=Instance->GetSubsystem<UHorizonSubsystem>();
    }
    void Actor(){if(Sample){Sample->RemoveFromRoot();Sample->MarkAsGarbage();}Sample=NewObject<UHorizonMarketplaceExample>();Sample->AddToRoot();Test->TestTrue(TEXT("Existing session explicitly bound"),Sample->Bind(S,Project));}
    FString File(const FString& U){return FPaths::ProjectSavedDir()/TEXT("SaveGames")/(HorizonMarketplaceJournal::Name(Project,U)+TEXT(".market"));}
    void Next(){Stage++;Pending=false;Wait=nullptr;}
    void Done(bool Ok,const FString& Code,const FString& Expected){Test->TestEqual(FString::Printf(TEXT("Marketplace stage %d success"),Stage),Ok,Expected.IsEmpty());Test->TestEqual(TEXT("Original transport/feature code"),Code,Expected);Next();}
    FOnMarketplaceComplete Market(const FString& Expected=TEXT(""),TFunction<void(const FHorizonMarketplaceResponse&)> Check=nullptr)
    {return FOnMarketplaceComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared()),Expected,Check](bool Ok,const FHorizonMarketplaceResponse& V,const FString& Code,const FString&){if(auto T=W.Pin()){if(Ok&&Check)Check(V);T->Done(Ok,Code,Expected);}});}
    FOnInventoryComplete Inventory(bool Empty=false,TFunction<void(const FHorizonInventorySnapshot&)> Check=nullptr)
    {return FOnInventoryComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared()),Empty,Check](bool Ok,const FHorizonInventorySnapshot& V,const FString& C,const FString&){if(auto T=W.Pin()){if(Ok){if(Empty)T->Test->TestTrue(TEXT("No historical item resurrected"),V.Instances.IsEmpty());if(Check)Check(V);}T->Done(Ok,C,TEXT(""));}});}
    FOnInboxClaimComplete Claim(bool Replay,const FString& Expected=TEXT(""))
    {return FOnInboxClaimComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared()),Replay,Expected](bool Ok,const FHorizonInboxClaim& V,const FString& C,const FString&){if(auto T=W.Pin()){if(Ok)T->Test->TestTrue(TEXT("Stable original Inbox escrow reference"),V.bReplayed==Replay&&V.Items.Num()==1&&V.Items[0].InstanceId==T->I3);T->Done(Ok,C,Expected);}});}
    void Login(const FString& Role){S->Auth->SignInAnonymous(TEXT("market-")+Role+TEXT("-anonymous"),FOnAuthComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared())](bool Ok){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Artificial existing identity restored, no signup"),Ok);T->Next();}}));}
    void CheckSaved(const FString& User,bool Confirmed,FHorizonMarketplaceIntent* Out=nullptr)
    {
        HorizonMarketplaceJournal::FLease L(Project,User);Test->TestTrue(TEXT("Original available after released asynchronous owner"),L.Locked&&L.Load());Test->TestEqual(TEXT("Minimal known phase"),!L.Original.ConfirmedListing.IsEmpty(),Confirmed);if(Out)*Out=L.Original.Intent;
        FString Raw;FFileHelper::LoadFileToString(Raw,*File(User));Test->TestFalse(TEXT("No copied financial/item response history"),Raw.Contains(TEXT("itemReference"))||Raw.Contains(TEXT("balance"))||Raw.Contains(TEXT("recordedAt")));
    }
    void Init()
    {
        Steps.Add([this]{S->GetHttpClient()->ConnectToServer(FOnRequestComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared())](bool Ok,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Loopback connection"),Ok);T->Next();}}));});
        Steps.Add([this]{Login(TEXT("seller"));});
        Steps.Add([this]{Actor();S->Marketplace->GetLimits(Other,Market(TEXT("SESSION_FORBIDDEN")));});
        Steps.Add([this]{S->Marketplace->GetListing(Seller,TEXT("bad"),Market(TEXT("INVALID_REQUEST")));});
        Steps.Add([this]{S->Marketplace->GetRules(Seller,Market(TEXT("INVALID_REQUEST")),TEXT("hat"),1);});
        Steps.Add([this]{S->Marketplace->QueryListings(Seller,Market(TEXT("INVALID_REQUEST")),TEXT("hat"),1,200,TEXT(""),51);});
        Steps.Add([this]{S->Marketplace->GetLimits(Seller,Market(TEXT(""),[this](const auto& V){Test->TestEqual(TEXT("Raised operator quota"),V.Limits.RetainedListingsPerProject,12000);}));});
        Steps.Add([this]{S->Marketplace->GetRules(Seller,Market(),TEXT(""),1);});
        Steps.Add([this]{S->Marketplace->GetRules(Seller,Market(),TEXT("hat"),1,7);});
        for(int32 Lower=0;Lower<2;++Lower)Steps.Add([this,Lower]{S->Marketplace->GetMarketplace(Seller,Market(TEXT(""),[this,Lower](const auto& V){Test->TestEqual(TEXT("2051 retained listings above inherited256/2048 caps"),V.State.OpenListings.Num(),2051);Test->TestEqual(TEXT("151 retained terminal references"),V.State.History.Num(),151);Test->TestEqual(TEXT("Current cap does not truncate retained listings"),V.State.Limits.OpenListingsPerPlayer,Lower?1:3000);}));});
        Steps.Add([this]{S->Marketplace->QueryListings(Seller,Market(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Empty bounded query page retains continuation"),V.Listings.IsEmpty()&&!V.NextCursor.IsEmpty());Cursor=V.NextCursor;}),TEXT("hat"),1,200,TEXT(""),1);});
        Steps.Add([this]{S->Marketplace->QueryListings(Seller,Market(),TEXT("hat"),1,200,Cursor,1);});
        Steps.Add([this]{S->Marketplace->GetListing(Seller,Listing,Market(TEXT(""),[this](const auto& V){OriginalItem=V.Listing.Item;}));});
        Steps.Add([this]{S->Items->GetInventory(Seller,Inventory());});
        Steps.Add([this]{S->Marketplace->GetRules(Seller,Market());});
        Steps.Add([this]{Test->TestTrue(TEXT("Original instance and reviewed config/fee"),Sample->ReviewCreate(Item,101,300));Test->TestEqual(TEXT("Reviewed seller net"),Sample->ReviewedSellerNet,int64(98));Sample->ConfirmReviewedTrade();const double Start=FPlatformTime::Seconds();Wait=[this,Start]{const double Elapsed=FPlatformTime::Seconds()-Start;if(Sample&&Elapsed>.04){HorizonMarketplaceJournal::FLease Competing(Project,Seller);Test->TestFalse(TEXT("Second actor cannot enter live HTTP owner"),Competing.Locked);Sample->RemoveFromRoot();Sample->MarkAsGarbage();Sample=nullptr;}return Elapsed>.5;};});
        Steps.Add([this]{CheckSaved(Seller,false,&Original);Test->TestTrue(TEXT("New operation UUID is canonical lowercase"),Original.Operation().Equals(Original.Operation().ToLower(),ESearchCase::CaseSensitive));Actor();Sample->RecoverOriginalOperation();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{CheckSaved(Seller,true);Actor();Sample->RetryPending();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{Test->TestFalse(TEXT("Confirmed recovery GETs cleared only one original"),IFileManager::Get().FileExists(*File(Seller)));Test->TestTrue(TEXT("Current actual inventory remains empty after later sale/discard"),S->Items->GetCurrentInventory().Instances.IsEmpty());S->Marketplace->SubmitPrepared(Original,Market(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Replay current later listing, without historical item"),V.Replayed&&V.Receipt.ListingRevision==1&&V.Listing.Revision==2&&!V.Listing.HasItem);}));});
        Steps.Add([this]{auto Changed=Original;Changed.Json=Changed.Json.Replace(TEXT("\"price\":101"),TEXT("\"price\":102"));S->Marketplace->SubmitPrepared(Changed,Market(TEXT("OPERATION_CONFLICT")));});
        Steps.Add([this]{{auto I=Original;I.Json=I.Json.Replace(*I.Operation(),*Unknown);HorizonMarketplaceJournal::FLease L(Project,Seller);Test->TestTrue(TEXT("Artificial unknown original persisted"),L.Begin(I));}Sample->RecoverOriginalOperation();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{CheckSaved(Seller,false);Test->TestEqual(TEXT("404 does not acknowledge or replace"),Sample->LastErrorCode,FString(TEXT("OPERATION_NOT_FOUND")));IFileManager::Get().Delete(*File(Seller));Login(TEXT("buyer"));});
        Steps.Add([this]{Test->TestFalse(TEXT("Session boundary clears old current cache"),S->Marketplace->GetCurrent(TEXT("state")).Present);UHorizonMarketplaceQuickstart::ReadOneListing(Sample,L2);Wait=[this]{const auto V=S->Marketplace->GetCurrent(TEXT("point"));return V.Present&&V.Listing.Id==L2;};});
        Steps.Add([this]{Test->TestTrue(TEXT("Compiled Quickstart starts durable original buy"),UHorizonMarketplaceQuickstart::BuyReviewedListing(Sample,L2));Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{CheckSaved(Buyer,false,&BuyOriginal);Actor();Sample->RetryPending();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{Test->TestTrue(TEXT("Confirmed buyer inventory GET does not recreate later-discarded purchase"),S->Items->GetCurrentInventory().Instances.IsEmpty());const int64 Now=S->Marketplace->GetServerNow();auto I=UHorizonMarketplaceManager::PrepareBuy(Buyer,Listing,Unknown,(FDateTime::FromUnixTimestamp(Now/1000)+FTimespan::FromMilliseconds(Now%1000)).ToIso8601(),1,101,TEXT("1coin.gold"));S->Marketplace->SubmitPrepared(I,Market(TEXT("LISTING_UNAVAILABLE")));});
        Steps.Add([this]{Login(TEXT("seller"));});
        Steps.Add([this]{S->Marketplace->GetListing(Seller,L3,Market());});
        Steps.Add([this]{Test->TestTrue(TEXT("Owner reviews exact cancellation"),Sample->ReviewCancel(L3));Sample->ConfirmReviewedTrade();Wait=[this]{return !Sample->Busy;};});
        Steps.Add([this]{Test->TestTrue(TEXT("Cancel confirmed and current state refreshed"),Sample->LastErrorCode.IsEmpty());S->Marketplace->RetryReturn(Seller,L3,Market(TEXT(""),[this](const auto& V){Test->TestEqual(TEXT("Inbox capacity recoverable"),V.Delivery,FString(TEXT("pending")));}));});
        Steps.Add([this]{S->Marketplace->RetryReturn(Seller,L3,Market(TEXT(""),[this](const auto& V){Test->TestTrue(TEXT("Original escrow attached to stable Inbox entry"),V.Delivery==TEXT("delivered")&&V.Listing.ReturnEntryId==Entry&&V.Listing.Item.InstanceId==I3);}));});
        Steps.Add([this]{S->Inbox->Claim(Seller,Entry,Claim(false,TEXT("INVENTORY_FULL")));});
        Steps.Add([this]{S->Inbox->Claim(Seller,Entry,Claim(false));});
        Steps.Add([this]{S->Items->GetInventory(Seller,Inventory(false,[this](const auto& V){Test->TestTrue(TEXT("Exact original createdAt and property count"),V.Instances.Num()==1&&V.Instances[0].InstanceId==I3&&V.Instances[0].CreatedAt==OriginalItem.CreatedAt&&V.Instances[0].Properties.Num()==OriginalItem.Properties.Num());if(V.Instances.Num()==1)for(const auto& P:OriginalItem.Properties){const auto* X=V.Instances[0].Properties.FindByPredicate([&](const auto& Q){return Q.Key==P.Key;});Test->TestTrue(TEXT("Original Unicode/NUL/bool/safe integer properties"),X&&X->Type==P.Type&&X->IntegerValue==P.IntegerValue&&X->StringValue==P.StringValue&&X->BooleanValue==P.BooleanValue);}}));});
        Steps.Add([this]{S->Items->DiscardItem(Seller,I3,11,Inventory(true));});
        Steps.Add([this]{S->Inbox->Claim(Seller,Entry,Claim(true));});
        Steps.Add([this]{S->Items->GetInventory(Seller,Inventory(true));});
        Steps.Add([this]{S->Marketplace->RetryReturn(Seller,L3,Market(TEXT(""),[this](const auto& V){Test->TestEqual(TEXT("Terminal returned listing reference"),V.Delivery,FString(TEXT("returned")));}));});
        Steps.Add([this]{Login(TEXT("buyer"));});
        Steps.Add([this]{S->Marketplace->GetListing(Buyer,L3,Market(TEXT("LISTING_NOT_FOUND")));});
        Steps.Add([this]{S->Marketplace->GetLimits(Buyer,Market(TEXT("INVALID_RESPONSE")));});
        Steps.Add([this]{Race=0;S->Marketplace->GetMarketplace(Buyer,FOnMarketplaceComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared())](bool Ok,const auto&,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Old cleared request cannot populate cache"),!Ok&&C==TEXT("STALE_RESPONSE"));if(++T->Race==2)T->Next();}}));S->Marketplace->ClearCache();S->Marketplace->GetMarketplace(Buyer,FOnMarketplaceComplete::CreateLambda([W=TWeakPtr<FHorizonMarketplaceNative>(AsShared())](bool Ok,const auto& V,const FString&,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("New busy ticket owns revision9"),Ok&&V.State.Revision==9);if(++T->Race==2)T->Next();}}));});
        Steps.Add([this]{S->Marketplace->GetMarketplace(Buyer,Market(TEXT("STALE_RESPONSE")));});
        Steps.Add([this]{S->Marketplace->GetLimits(Buyer,Market(TEXT("STALE_RESPONSE")));S->GetHttpClient()->SetSessionToken(TEXT("other-session"));S->GetHttpClient()->SetSessionToken(TEXT("market-buyer-session"));});
    }
    void Cleanup()
    {
        if(Sample){Sample->RemoveFromRoot();Sample->MarkAsGarbage();Sample=nullptr;}for(const auto& U:{Seller,Buyer}){IFileManager::Get().Delete(*File(U));IFileManager::Get().Delete(*(File(U)+TEXT(".next")));}
        S->Auth->SignOut();S->GetHttpClient()->Disconnect();auto* World=Instance->GetWorld();Instance->Shutdown();World->DestroyWorld(false);GEngine->DestroyWorldContext(World);Instance->RemoveFromRoot();auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts=OldHosts;C->ApiKey=OldKey;C->MaxRetryAttempts=OldRetries;C->RetryDelaySeconds=OldDelay;
    }
    bool Update(){if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("Marketplace timeout stage%d"),Stage));Cleanup();return true;}if(Wait){if(Wait())Next();return false;}if(Pending)return false;if(Stage>=Steps.Num()){Cleanup();return true;}Pending=true;Steps[Stage]();return false;}
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonMarketplace,TSharedPtr<FHorizonMarketplaceNative>,State);
bool FWaitForHorizonMarketplace::Update(){return State->Update();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonMarketplaceTransportTest,"Horizon.Marketplace.NativeTransport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonMarketplaceTransportTest::RunTest(const FString&)
{
    const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_MARKETPLACE_FIXTURE_URL"));if(Host.IsEmpty()){AddWarning(TEXT("Set isolated native Marketplace fixture URL"));return true;}if(!Host.StartsWith(TEXT("http://127.0.0.1:"))){AddError(TEXT("Loopback only"));return false;}
    for(const TCHAR* N:{TEXT("ReadLimits"),TEXT("ReadRules"),TEXT("ReadMarketplace"),TEXT("ReadListings"),TEXT("ReadListing"),TEXT("SubmitOriginal"),TEXT("LookupOriginal"),TEXT("RetryReturnDelivery"),TEXT("PrepareCreate"),TEXT("PrepareBuy"),TEXT("PrepareCancel")})TestNotNull(TEXT("Blueprint Marketplace surface"),UHorizonMarketplaceManager::StaticClass()->FindFunctionByName(N));
    auto State=MakeShared<FHorizonMarketplaceNative>(this,Host);State->Init();ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonMarketplace(State));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonMarketplaceModelsTest,"Horizon.Marketplace.StrictModels",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonMarketplaceModelsTest::RunTest(const FString&)
{
    const auto I=UHorizonMarketplaceManager::PrepareCreate(TEXT("20000000-0000-4000-8000-000000001015"),TEXT("60000000-0000-4000-8000-000000001015"),TEXT("2026-10-17T12:00:00.000Z"),TEXT("40000000-0000-4000-8000-000000001015"),9,7,101,300);TestTrue(TEXT("Valid exact original"),I.IsValid());
    for(const TCHAR* N:{TEXT("1e0"),TEXT("1.0"),TEXT("-0"),TEXT("9007199254740992")}){auto Bad=I;Bad.Json=I.Json.Replace(TEXT("\"price\":101"),*(FString(TEXT("\"price\":"))+N));TestFalse(TEXT("Unsafe original tokens rejected"),Bad.IsValid());}
    const FString Alpha=TEXT("abcdefab-cdef-4abc-8def-abcdefabcdef"),AlphaOperation=TEXT("102030ab-cdef-4abc-8def-abcd01234567");
    const auto Lower=UHorizonMarketplaceManager::PrepareCreate(Alpha,AlphaOperation,TEXT("2026-10-17T12:00:00.000Z"),Alpha,9,7,101,300);
    TestTrue(TEXT("Alphabetic lowercase user/operation/instance UUIDs accepted"),Lower.IsValid());
    auto Upper=Lower;Upper.Json=Lower.Json.Replace(*AlphaOperation,*AlphaOperation.ToUpper());TestFalse(TEXT("Uppercase operation UUID rejected before HTTP"),Upper.IsValid());
    TestFalse(TEXT("Uppercase user UUID rejected before preparing intent"),UHorizonMarketplaceManager::PrepareCreate(Alpha.ToUpper(),AlphaOperation,TEXT("2026-10-17T12:00:00.000Z"),Alpha,9,7,101,300).IsValid());
    const FString Point=TEXT(R"MARKET({"listing":{"id":"abcdefab-cdef-4abc-8def-abcdefabcdef","revision":1,"sellerId":"abcdefab-cdef-4abc-8def-abcdefabcdef","buyerId":null,"returnOwnerId":"abcdefab-cdef-4abc-8def-abcdefabcdef","item":{"instanceId":"abcdefab-cdef-4abc-8def-abcdefabcdef","itemKey":"hat","properties":{"power":9007199254740991},"equipped":false,"createdAt":"2026-10-17T12:00:00.000Z"},"itemReference":{"instanceId":"abcdefab-cdef-4abc-8def-abcdefabcdef","itemKey":"hat"},"currencyKey":"1coin.gold","price":101,"feeBps":333,"feeAmount":3,"sellerNet":98,"status":"active","createdAt":"2026-10-17T12:00:00.000Z","expiresAt":"2026-10-18T12:00:00.000Z","terminalAt":null,"returnEntryId":null,"ownerUnavailable":false},"serverTime":"2026-10-17T12:00:00.000Z"})MARKET");FHorizonMarketplaceResponse View;
    TestTrue(TEXT("Alphabetic lowercase listing/reference/original UUIDs accepted"),FHorizonMarketplaceResponse::TryFromJson(Point,TEXT("point"),View));
    const FString UpperOriginal=Point.Replace(TEXT("\"item\":{\"instanceId\":\"abcdefab-cdef-4abc-8def-abcdefabcdef\""),TEXT("\"item\":{\"instanceId\":\"ABCDEFAB-CDEF-4ABC-8DEF-ABCDEFABCDEF\""));
    TestFalse(TEXT("Original escrow UUID is not accepted via case-insensitive FString equality"),FHorizonMarketplaceResponse::TryFromJson(UpperOriginal,TEXT("point"),View));
    auto Date=I;Date.Json=I.Json.Replace(TEXT("2026-10-17"),TEXT("2026-02-30"));TestFalse(TEXT("Impossible dates not normalized"),Date.IsValid());
    return true;
}
#endif
