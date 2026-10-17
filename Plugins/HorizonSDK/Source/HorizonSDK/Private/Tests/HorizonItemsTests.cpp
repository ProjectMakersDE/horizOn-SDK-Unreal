#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Models/HorizonItemsWire.h"
#include "AsyncActions/HorizonAsync_Items.h"
#include "Examples/HorizonItemsExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "HorizonConfig.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
struct FHorizonItemsNative : TSharedFromThis<FHorizonItemsNative>
{
 FAutomationTestBase* Test;UGameInstance* Instance;UHorizonHttpClient* Http;UHorizonAuthManager* Auth;UHorizonItemsManager* Items;UHorizonPlayerValuesManager* PV;AHorizonItemsExample* ActorA;
 UHorizonAsync_ItemsInventory* AsyncInventory=nullptr;UHorizonAsync_ItemCatalog* AsyncCatalog=nullptr;
 TArray<FString> OriginalHosts;FString OriginalKey,Slot,SavedOperation;int32 OriginalRetries;float OriginalDelay;TArray<TFunction<void()>> Steps;TFunction<bool()> Wait;int32 Stage=0,Race=0;bool Pending=false;double Deadline=FPlatformTime::Seconds()+100;TArray<FHorizonItemPropertyDelta> Deltas;
 const FString User=TEXT("00000000-0000-4000-8000-000000001011"),Item=TEXT("10000000-0000-4000-8000-000000001011"),Operation=TEXT("1792108830000:20000000-0000-4000-8000-000000001011");
 FHorizonItemsNative(FAutomationTestBase* T,const FString& Host):Test(T)
 {
  auto* C=GetMutableDefault<UHorizonConfig>();OriginalHosts=C->Hosts;OriginalKey=C->ApiKey;OriginalRetries=C->MaxRetryAttempts;OriginalDelay=C->RetryDelaySeconds;C->Hosts={Host};C->ApiKey=TEXT("fixture-key-1011");C->MaxRetryAttempts=2;C->RetryDelaySeconds=.03f;
  Instance=NewObject<UGameInstance>(GEngine);Instance->AddToRoot();Instance->InitializeStandalone();auto* SDK=Instance->GetSubsystem<UHorizonSubsystem>();Http=SDK->GetHttpClient();Auth=SDK->Auth;Items=SDK->Items;PV=SDK->PlayerValues;ActorA=Instance->GetWorld()->SpawnActor<AHorizonItemsExample>();
  Slot=TEXT("horizon-item-original-")+UHorizonExampleIdentity::Project(UHorizonExampleIdentity::Project(C->ApiKey)+TEXT("|")+User);if(UGameplayStatics::DoesSaveGameExist(Slot,0))UGameplayStatics::DeleteGameInSlot(Slot,0);FHorizonItemPropertyDelta D;D.Key=TEXT("durability");D.Amount=-1;Deltas={D};
 }
 UHorizonItemOriginal* Load(){return Cast<UHorizonItemOriginal>(UGameplayStatics::LoadGameFromSlot(Slot,0));}
 void Next(){++Stage;Pending=false;Wait=nullptr;}
 void Done(bool Ok,const FString& C,const FString& Expected){Test->TestEqual(FString::Printf(TEXT("Items stage %d success"),Stage),Ok,Expected.IsEmpty());Test->TestEqual(TEXT("Original feature/transport code"),C,Expected);Next();}
 FOnInventoryComplete Inv(const FString& Expected){return FOnInventoryComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared()),Expected](bool Ok,const FHorizonInventorySnapshot& V,const FString& C,const FString&){if(auto T=W.Pin()){if(Ok){T->Test->TestEqual(TEXT("Authorized inventory"),V.UserId,T->User);if(!V.Instances.IsEmpty()){const auto& P=V.Instances[0].Properties;const auto* Label=P.FindByPredicate([](const FHorizonItemProperty& X){return X.Key==TEXT("label");});T->Test->TestTrue(TEXT("Unicode and embedded NUL string is lossless"),Label&&Label->StringValue.Len()==9&&Label->StringValue[4]==0&&Label->StringValue.EndsWith(TEXT("tail")));const auto* N=P.FindByPredicate([](const FHorizonItemProperty& X){return X.Key==TEXT("durability");});T->Test->TestTrue(TEXT("Safe integer exact at boundary"),N&&N->IntegerValue==9007199254740991LL);}}T->Done(Ok,C,Expected);}});}
 FOnItemCatalogComplete Cat(const FString& E){return FOnItemCatalogComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared()),E](bool Ok,const FHorizonItemCatalogPage& V,const FString& C,const FString&){if(auto T=W.Pin()){if(Ok)T->Test->TestTrue(TEXT("Inactive definitions remain readable"),V.bPresent&&V.Definitions.Num()==1&&!V.Definitions[0].bActive);T->Done(Ok,C,E);}});}
 FOnPlayerValuesOffersComplete Offers(){return FOnPlayerValuesOffersComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared())](bool Ok,const FHorizonPlayerValuesOffers& V,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Item-only offer additive rewards"),Ok&&V.Offers.Num()==1&&V.Offers[0].Rewards.IsEmpty()&&V.Offers[0].ItemRewards.Num()==1);T->Done(Ok,C,TEXT(""));}});}
 FOnPlayerValuesComplete Values(const FString& E){return FOnPlayerValuesComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared()),E](bool Ok,const FHorizonPlayerValuesState& V,const FString& C,const FString&){if(auto T=W.Pin()){if(Ok)T->Test->TestTrue(TEXT("Original instance reference retained"),V.Operation.Items.Num()==1&&V.Operation.Items[0].InstanceId==T->Item);T->Done(Ok,C,E);}});}
 FOnInventoryComplete ConfiguredInv(int32 ExpectedInventory,bool Equipped=false){return FOnInventoryComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared()),ExpectedInventory,Equipped](bool Ok,const FHorizonInventorySnapshot& V,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Configured/retained inventory succeeds"),Ok);T->Test->TestEqual(TEXT("Retained2051 inventory exceeds old mask256 and lowered quota"),V.Instances.Num(),2051);T->Test->TestEqual(TEXT("Current configured inventory quota"),V.Limits.Inventory,ExpectedInventory);T->Test->TestEqual(TEXT("Configurable receipt quota exceeds old50"),V.Limits.Receipts,ExpectedInventory==3000?80:10);if(V.Instances.Num()==2051){T->Test->TestEqual(TEXT("Original instance remains independently addressable"),V.Instances[0].InstanceId,T->Item);T->Test->TestEqual(TEXT("Original equip wire effect"),V.Instances[0].bEquipped,Equipped);}T->Done(Ok,C,TEXT(""));}});}
 FOnItemCatalogComplete ConfiguredCat(int32 ExpectedCatalog,const FString& Key){return FOnItemCatalogComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared()),ExpectedCatalog,Key](bool Ok,const FHorizonItemCatalogPage& V,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Configured/retained catalogue succeeds"),Ok);T->Test->TestEqual(TEXT("Retained catalog exceeds old5000 and lowered current cap"),V.Total,6000);T->Test->TestEqual(TEXT("Configured catalogue quota"),V.Limits.Catalog,ExpectedCatalog);T->Test->TestEqual(TEXT("Explicit original page revision"),V.Revision,int64(2));T->Test->TestTrue(TEXT("Requested page preserved"),V.Definitions.Num()==1&&V.Definitions[0].Key==Key);T->Done(Ok,C,TEXT(""));}});}
 void LocalChecks()
 {
  bool No=false;Items->GetInventory(User,FOnInventoryComplete::CreateLambda([&](bool Ok,const FHorizonInventorySnapshot&,const FString& C,const FString&){No=!Ok&&C==TEXT("SESSION_REQUIRED");}));Test->TestTrue(TEXT("No automatic signup"),No);
  auto Reject=[this](TFunction<void(FOnInventoryComplete)> F){bool Bad=false;F(FOnInventoryComplete::CreateLambda([&](bool Ok,const FHorizonInventorySnapshot&,const FString& C,const FString&){Bad=!Ok&&C==TEXT("INVALID_REQUEST");}));Test->TestTrue(TEXT("Invalid original rejected locally"),Bad);};
  Reject([this](FOnInventoryComplete D){Items->DiscardItem(User,Item,-1,D);});Reject([this](FOnInventoryComplete D){Items->SetEquipped(User,Item,9007199254740992LL,true,D);});Reject([this](FOnInventoryComplete D){auto V=Deltas;const auto Copy=V[0];V.Add(Copy);Items->AdjustProperties(User,Item,1,V,D);});
  bool Cursor=false;Items->GetCatalog(User,FOnItemCatalogComplete::CreateLambda([&](bool Ok,const FHorizonItemCatalogPage&,const FString& C,const FString&){Cursor=!Ok&&C==TEXT("INVALID_REQUEST");}),TEXT("sword"));Test->TestTrue(TEXT("Cursor requires original revision"),Cursor);
 }
 void Init()
 {
  Steps.Add([this]{LocalChecks();Next();});
  Steps.Add([this]{Http->ConnectToServer(FOnRequestComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared())](bool Ok,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Loopback connected"),Ok);T->Next();}}));});
  Steps.Add([this]{Auth->SignInAnonymous(TEXT("fixture-anonymous-1011"),FOnAuthComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared())](bool Ok){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Existing identity restored"),Ok);T->Next();}}));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("")));});
  Steps.Add([this]{Items->GetCatalog(User,Cat(TEXT("")));});
  Steps.Add([this]{Items->GetCatalog(User,Cat(TEXT("")),TEXT("sword"),1,1);});
  Steps.Add([this]{Items->AdjustProperties(User,Item,1LL,Deltas,Inv(TEXT("")));});
  Steps.Add([this]{Items->SetEquipped(User,Item,2LL,true,Inv(TEXT("")));});
  Steps.Add([this]{Items->DiscardItem(User,Item,3LL,Inv(TEXT("")));});
  Steps.Add([this]{Items->AdjustProperties(User,Item,9007199254740991LL,Deltas,Inv(TEXT("SERVER_ERROR")));});
  Steps.Add([this]{Items->SetEquipped(User,Item,9007199254740991LL,true,Inv(TEXT("SERVER_ERROR")));});
  Steps.Add([this]{Items->DiscardItem(User,Item,9007199254740991LL,Inv(TEXT("SERVER_ERROR")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("STALE_RESPONSE")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("STALE_RESPONSE")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("INVALID_RESPONSE")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("")));});
  Steps.Add([this]{Items->ClearCache();Next();});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("STALE_RESPONSE")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("STALE_RESPONSE")));Http->SetSessionToken(TEXT("other"));Http->SetSessionToken(TEXT("fixture-session-1011"));Test->TestFalse(TEXT("Session ABA hides cached inventory"),Items->GetCurrentInventory().bPresent);});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("")));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("STALE_RESPONSE")));auto* C=GetMutableDefault<UHorizonConfig>();C->ApiKey=TEXT("other");Http->Initialize();C->ApiKey=TEXT("fixture-key-1011");Http->Initialize();});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("")));});
  Steps.Add([this]{Race=0;Items->GetInventory(User,FOnInventoryComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared())](bool Ok,const FHorizonInventorySnapshot&,const FString& C,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("Old clear generation rejected"),!Ok&&C==TEXT("STALE_RESPONSE"));bool Busy=false;T->Items->GetInventory(T->User,FOnInventoryComplete::CreateLambda([&](bool B,const FHorizonInventorySnapshot&,const FString& E,const FString&){Busy=!B&&E==TEXT("REQUEST_IN_PROGRESS");}));T->Test->TestTrue(TEXT("Old callback preserves new busy ticket"),Busy);if(++T->Race==2)T->Next();}}));Items->ClearCache();Items->GetInventory(User,FOnInventoryComplete::CreateLambda([W=TWeakPtr<FHorizonItemsNative>(AsShared())](bool Ok,const FHorizonInventorySnapshot& V,const FString&,const FString&){if(auto T=W.Pin()){T->Test->TestTrue(TEXT("New generation owns snapshot"),Ok&&V.Revision==3);if(++T->Race==2)T->Next();}}));});
  Steps.Add([this]{Items->GetInventory(User,Inv(TEXT("SESSION_FORBIDDEN")));});
  Steps.Add([this]{Test->TestFalse(TEXT("Authorization loss clears cache"),Items->GetCurrentInventory().bPresent);Next();});
  Steps.Add([this]{PV->GetOffers(User,Offers());});
  Steps.Add([this]{PV->Purchase(User,TEXT("sword_offer"),Operation,Values(TEXT("")));});
  Steps.Add([this]{PV->Purchase(User,TEXT("sword_offer"),Operation,Values(TEXT("INVALID_RESPONSE")));});
  Steps.Add([this]{Http->SetSessionToken(TEXT("other"));Http->SetSessionToken(TEXT("fixture-session-1011"));Test->TestFalse(TEXT("PV ABA clears receipt cache"),PV->GetCurrentState().bPresent);Next();});
  Steps.Add([this]{Items->ClearCache();AsyncInventory=UHorizonAsync_ItemsInventory::GetInventory(Instance);AsyncInventory->AddToRoot();AsyncInventory->Activate();AsyncInventory->Activate();Wait=[this]{return Items->GetCurrentInventory().bPresent;};});
  Steps.Add([this]{AsyncCatalog=UHorizonAsync_ItemCatalog::GetCatalog(Instance);AsyncCatalog->AddToRoot();AsyncCatalog->Activate();AsyncCatalog->Activate();Wait=[this]{return Items->GetCurrentCatalog().bPresent;};});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->StartItemAction(TEXT("equip"),Item,1,{},true);Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();Test->TestTrue(TEXT("Original persisted after lost response"),P&&P->ExpectedRevision==1&&P->SendSerial==1);ActorA->Destroy();ActorA=Instance->GetWorld()->SpawnActor<AHorizonItemsExample>();Next();});
  Steps.Add([this]{ActorA->RetrySavedAction();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();Test->TestTrue(TEXT("Explicit retry retains original revision and payload"),P&&P->ExpectedRevision==1&&P->SendSerial==2&&P->Equipped);Test->TestFalse(TEXT("No acknowledgement without refresh"),ActorA->AcknowledgeItemActionAfterRefresh());Next();});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();Test->TestNotNull(TEXT("Original before explicit acknowledgement"),P);if(P){P->Equipped=false;UGameplayStatics::SaveGameToSlot(P,Slot,0);Test->TestFalse(TEXT("Changed original cannot be acknowledged"),ActorA->AcknowledgeItemActionAfterRefresh());P->Equipped=true;UGameplayStatics::SaveGameToSlot(P,Slot,0);}Test->TestTrue(TEXT("Exact refreshed CAS original can be explicitly reconciled"),ActorA->AcknowledgeItemActionAfterRefresh());Next();});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->PurchaseItemOffer(TEXT("sword_offer"));Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();Test->TestTrue(TEXT("Confirmed purchase persisted before failed inventory read"),P&&P->Confirmed&&P->SendSerial==1);SavedOperation=P?P->OperationId:FString();ActorA->Destroy();ActorA=Instance->GetWorld()->SpawnActor<AHorizonItemsExample>();Next();});
  Steps.Add([this]{ActorA->RetrySavedAction();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{Test->TestFalse(TEXT("Confirmed purchase recovered by GET only, no purchase replacement"),UGameplayStatics::DoesSaveGameExist(Slot,0));Next();});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->PurchaseItemOffer(TEXT("sword_offer"));Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();SavedOperation=P?P->OperationId:FString();Test->TestTrue(TEXT("Unknown purchase original retained"),P&&!P->Confirmed);Next();});
  Steps.Add([this]{ActorA->RetrySavedAction();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();Test->TestTrue(TEXT("Changed price did not replace or send operation"),P&&P->OperationId==SavedOperation&&P->SendSerial==1);Next();});
  Steps.Add([this]{ActorA->RetrySavedAction(true);Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{Test->TestFalse(TEXT("Replayed purchase and inventory refreshed"),UGameplayStatics::DoesSaveGameExist(Slot,0));Next();});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->StartItemAction(TEXT("adjust"),Item,4,Deltas,false);Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->StartItemAction(TEXT("discard"),Item,5,{},false);Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{Test->TestTrue(TEXT("Actual sample adopts authoritative removal"),Items->GetCurrentInventory().bPresent&&Items->GetCurrentInventory().Instances.IsEmpty());Next();});
  Steps.Add([this]{ActorA->Refresh();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->PurchaseItemOffer(TEXT("sword_offer"));Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{ActorA->RetrySavedAction();Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{auto* P=Load();Test->TestTrue(TEXT("Removed offer retained original for explicit receipt recovery"),P&&!P->Confirmed&&P->SendSerial==1);Next();});
  Steps.Add([this]{ActorA->RetrySavedAction(true);Wait=[this]{return !ActorA->Busy;};});
  Steps.Add([this]{Test->TestFalse(TEXT("Removed offer original receipt recovered without new ID or synthetic instance"),UGameplayStatics::DoesSaveGameExist(Slot,0));Next();});
  Steps.Add([this]{auto* A=UHorizonAsync_ItemsInventory::DiscardItem(Instance,Item,6);A->AddToRoot();Http->SetSessionToken(TEXT("other"));Http->SetSessionToken(TEXT("fixture-session-1011"));A->Activate();A->RemoveFromRoot();Next();});
  Steps.Add([this]{Items->GetInventory(User,ConfiguredInv(3000));});
  Steps.Add([this]{Items->GetInventory(User,ConfiguredInv(10));});
  Steps.Add([this]{Items->GetCatalog(User,ConfiguredCat(6000,TEXT("sword")),TEXT(""),1);});
  Steps.Add([this]{Items->GetCatalog(User,ConfiguredCat(20,TEXT("wand")),TEXT("sword"),1,2);});
  Steps.Add([this]{Items->SetEquipped(User,Item,9,true,ConfiguredInv(10,true));});

 }
 void Cleanup(){if(AsyncInventory)AsyncInventory->RemoveFromRoot();if(AsyncCatalog)AsyncCatalog->RemoveFromRoot();Auth->SignOut();Http->Disconnect();if(UGameplayStatics::DoesSaveGameExist(Slot,0))UGameplayStatics::DeleteGameInSlot(Slot,0);auto* W=Instance->GetWorld();Instance->Shutdown();W->DestroyWorld(false);GEngine->DestroyWorldContext(W);Instance->RemoveFromRoot();auto* C=GetMutableDefault<UHorizonConfig>();C->Hosts=OriginalHosts;C->ApiKey=OriginalKey;C->MaxRetryAttempts=OriginalRetries;C->RetryDelaySeconds=OriginalDelay;}
 bool Update(){if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("Items timeout stage %d"),Stage));Cleanup();return true;}if(Wait){if(Wait())Next();return false;}if(Pending)return false;if(Stage>=Steps.Num()){Cleanup();return true;}Pending=true;Steps[Stage]();return false;}
};
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForHorizonItems,TSharedPtr<FHorizonItemsNative>,State);
bool FWaitForHorizonItems::Update(){return State->Update();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonItemsTransportTest,"Horizon.Items.NativeTransport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonItemsTransportTest::RunTest(const FString&)
{
 const FString Host=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_ITEMS_FIXTURE_URL"));if(Host.IsEmpty()){AddWarning(TEXT("Set isolated HORIZON_ITEMS_FIXTURE_URL"));return true;}if(!Host.StartsWith(TEXT("http://127.0.0.1:"))){AddError(TEXT("Loopback only"));return false;}
 for(const TCHAR* N:{TEXT("RequestInventory"),TEXT("RequestCatalog"),TEXT("AdjustItemProperties"),TEXT("EquipItem"),TEXT("DiscardInventoryItem")})TestNotNull(TEXT("Blueprint manager function"),UHorizonItemsManager::StaticClass()->FindFunctionByName(N));
 for(const TCHAR* N:{TEXT("GetInventory"),TEXT("AdjustProperties"),TEXT("SetEquipped"),TEXT("DiscardItem")})TestNotNull(TEXT("Blueprint async function"),UHorizonAsync_ItemsInventory::StaticClass()->FindFunctionByName(N));
 auto S=MakeShared<FHorizonItemsNative>(this,Host);S->Init();ADD_LATENT_AUTOMATION_COMMAND(FWaitForHorizonItems(S));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonItemsModelsTest,"Horizon.Items.StrictModels",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonItemsModelsTest::RunTest(const FString&)
{
 const FString Model=TEXT(R"ITEMS({"userId":"00000000-0000-4000-8000-000000001011","revision":1,"instances":[{"instanceId":"10000000-0000-4000-8000-000000001011","itemKey":"sword","properties":__PROPERTIES__,"equipped":false,"createdAt":"2026-10-16T00:00:30Z"}],"limits":{"catalog":20,"inventory":10,"receipts":10},"serverTime":"2026-10-16T00:00:30Z"})ITEMS");
 auto Parse=[](const FString& Text){TSharedPtr<FJsonObject> J;FHorizonInventorySnapshot V;return HorizonItemsWire::Parse(Text,J)&&FHorizonInventorySnapshot::TryFromJson(J,V);};
 TestTrue(TEXT("Canonical Unicode/NUL property object accepted"),Parse(Model.Replace(TEXT("__PROPERTIES__"),TEXT(R"PROP({"durability":9007199254740991,"label":"雪/😀\u0000tail","bound":true,"low":-9007199254740991})PROP"))));
 for(const FString& Bad:{FString(TEXT("null")),FString(TEXT("[]")),FString(TEXT("{\"x\":null}")),FString(TEXT("{\"x\":1.0}")),FString(TEXT("{\"x\":1e3}")),FString(TEXT("{\"x\":9007199254740992}")),FString(TEXT("{\"x\":9007199254740991.1}")),FString(TEXT("{\"x\":true,\"x\":false}")),FString(TEXT("{\"X\":1}")),FString(TEXT("{\"x\":\"\\ud800\"}"))}){const FString V=Model.Replace(TEXT("__PROPERTIES__"),*Bad);TestFalse(TEXT("Malformed property rejected before rounding or DOM aliasing"),Parse(V));}
 auto Raw=Model.Replace(TEXT("__PROPERTIES__"),TEXT("{\"x\":1}"));TestTrue(TEXT("Exact safe revision allowed"),Parse(Raw.Replace(TEXT("\"revision\":1"),TEXT("\"revision\":9007199254740991"))));TestFalse(TEXT("Unsafe revision rejected"),Parse(Raw.Replace(TEXT("\"revision\":1"),TEXT("\"revision\":9007199254740992"))));
 TestTrue(TEXT("Positive Int32 operator quota is not old tariff max"),Parse(Raw.Replace(TEXT("\"catalog\":20"),TEXT("\"catalog\":2147483647"))));
 const FString Catalog=TEXT(R"CAT({"revision":1,"definitions":[],"nextAfterKey":null,"total":65536,"limits":{"catalog":2147483647,"inventory":2147483647,"receipts":2147483647},"serverTime":"2026-10-16T00:00:30Z"})CAT");
 auto ParseCatalog=[](const FString& Text){TSharedPtr<FJsonObject> J;FHorizonItemCatalogPage V;return HorizonItemsWire::Parse(Text,J)&&FHorizonItemCatalogPage::TryFromJson(J,V);};
 TestTrue(TEXT("Physical UInt16 slot capacity is independent of operator quotas"),ParseCatalog(Catalog));
 TestFalse(TEXT("One past physical slot capacity is invalid"),ParseCatalog(Catalog.Replace(TEXT("65536"),TEXT("65537"))));
 for(const FString& Bad:{FString(TEXT("0")),FString(TEXT("2147483648")),FString(TEXT("1.0")),FString(TEXT("1e3")),FString(TEXT("9007199254740992"))})TestFalse(TEXT("Configured quota remains exact positive Int32"),Parse(Raw.Replace(TEXT("\"catalog\":20"),*(FString(TEXT("\"catalog\":"))+Bad))));
 return true;
}
#endif
