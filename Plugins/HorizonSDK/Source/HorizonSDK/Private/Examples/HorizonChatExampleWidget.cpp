#include "Examples/HorizonChatExampleWidget.h"
#include "Examples/HorizonChatExample.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonChatManager.h"
#include "Managers/HorizonAuthManager.h"
#include "Engine/GameInstance.h"
TSharedRef<SWidget> UHorizonChatExampleWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);if(WidgetTree->RootWidget)return Super::RebuildWidget();
    auto* Panel=WidgetTree->ConstructWidget<UVerticalBox>();WidgetTree->RootWidget=Panel;
    auto Label=[&](const TCHAR* S){auto* L=WidgetTree->ConstructWidget<UTextBlock>();L->SetText(FText::FromString(S));Panel->AddChild(L);return L;};
    Label(TEXT("horizOn Chat — existing player session required"));Kind=WidgetTree->ConstructWidget<UComboBoxString>();for(const auto& K:{TEXT("direct"),TEXT("group"),TEXT("project")})Kind->AddOption(K);Kind->SetSelectedOption(TEXT("project"));Panel->AddChild(Kind);
    auto Input=[&](const TCHAR* Hint){auto* I=WidgetTree->ConstructWidget<UEditableTextBox>();I->SetHintText(FText::FromString(Hint));Panel->AddChild(I);return I;};
    Context=Input(TEXT("Friend player ID for direct / group ID for group"));
    auto Button=[&](const TCHAR* Title){auto* B=WidgetTree->ConstructWidget<UButton>();auto* L=WidgetTree->ConstructWidget<UTextBlock>();L->SetText(FText::FromString(Title));B->AddChild(L);Panel->AddChild(B);return B;};
    Button(TEXT("Open selected channel"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::Open);Status=Label(TEXT("No subscription"));auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Feed=WidgetTree->ConstructWidget<UTextBlock>();Feed->SetAutoWrapText(true);Scroll->AddChild(Feed);Panel->AddChild(Scroll);
    Text=Input(TEXT("Plain message (up to 500 Unicode characters / 2000 UTF-8 bytes)"));Button(TEXT("Send and persist original"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::Send);Button(TEXT("Reconcile saved original"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::Reconcile);Button(TEXT("Explicit retry of saved original"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::Retry);
    MessageId=Input(TEXT("Message ID to report"));Reason=Input(TEXT("Report reason"));Button(TEXT("Report selected message"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::Report);
    HistoryChannel=Input(TEXT("Device history channel UUID (optional, also offline)"));Button(TEXT("Read device history"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::ReadHistory);Button(TEXT("Older device history"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::OlderHistory);Button(TEXT("Clear this device history"))->OnClicked.AddDynamic(this,&UHorizonChatExampleWidget::ClearHistory);HistoryFeed=Label(TEXT("Device copies in local delivery order"));HistoryFeed->SetAutoWrapText(true);
    if(auto* GI=GetGameInstance())if(auto* S=GI->GetSubsystem<UHorizonSubsystem>())S->Chat->OnLocalHistoryFailed.AddUniqueDynamic(this,&UHorizonChatExampleWidget::HistoryFailed);
    return Super::RebuildWidget();
}
void UHorizonChatExampleWidget::NativeTick(const FGeometry& G,float D)
{
    Super::NativeTick(G,D);if(!Example||!Status||!Feed)return;Status->SetText(FText::FromString((Example->IsSubscribed()?TEXT("Subscribed | "):TEXT("Disconnected / awaiting authorization | "))+Example->LastErrorCode+TEXT(" ")+HistoryError));FString S;const auto View=Example->GetFeed();if(View.bPresent)for(const auto& M:View.Messages)S+=M.Id+TEXT(" | ")+M.AuthorId+TEXT("\n")+M.Text+TEXT("\n\n");Feed->SetText(FText::FromString(S));auto* GI=GetGameInstance();auto* SDK=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;if(!SDK||!SDK->Auth->IsSignedIn()||HistoryIdentity!=SDK->GetHttpClient()->GetApiKey()+TEXT("|")+SDK->Auth->GetCurrentUser().UserId){if(HistoryFeed)HistoryFeed->SetText(FText::GetEmpty());HistoryCursor.Reset();}
}
void UHorizonChatExampleWidget::Open(){if(!Example)return;FHorizonChatOpenInput I;I.Kind=Kind->GetSelectedOption();if(I.Kind==TEXT("direct"))I.PeerId=Context->GetText().ToString();if(I.Kind==TEXT("group"))I.GroupId=Context->GetText().ToString();Example->Open(I);}
void UHorizonChatExampleWidget::Send(){if(Example)Example->SendText(Text->GetText().ToString());}
void UHorizonChatExampleWidget::Retry(){if(Example)Example->RetryOriginal();}
void UHorizonChatExampleWidget::Reconcile(){if(Example)Example->ReconcileOriginal();}
void UHorizonChatExampleWidget::Report(){if(Example)Example->Report(MessageId->GetText().ToString(),Reason->GetText().ToString());}
void UHorizonChatExampleWidget::ReadHistory(){HistoryCursor.Reset();OlderHistory();}
void UHorizonChatExampleWidget::OlderHistory(){auto* GI=GetGameInstance();auto* S=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;if(!S||!Example)return;const FString C=HistoryChannel->GetText().IsEmpty()?Example->ChannelId:HistoryChannel->GetText().ToString();auto P=S->Chat->ReadLocalHistory(C,HistoryCursor);if(!P.bSuccess){HistoryError=P.ErrorCode;return;}HistoryIdentity=S->GetHttpClient()->GetApiKey()+TEXT("|")+S->Auth->GetCurrentUser().UserId;HistoryCursor=P.NextCursor;FString HistoryText=TEXT("Device copies in local delivery order\n\n");for(const auto& M:P.Messages)HistoryText+=M.CreatedAt+TEXT(" | ")+M.AuthorId+TEXT("\n")+M.Text+TEXT("\n\n");HistoryFeed->SetText(FText::FromString(HistoryText));}
void UHorizonChatExampleWidget::ClearHistory(){auto* GI=GetGameInstance();auto* S=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;if(S&&Example&&S->Chat->ClearLocalHistory(HistoryChannel->GetText().IsEmpty()?Example->ChannelId:HistoryChannel->GetText().ToString())){HistoryFeed->SetText(FText::FromString(TEXT("Device history cleared.")));HistoryCursor.Reset();}}
void UHorizonChatExampleWidget::HistoryFailed(const FString& C,const FString& Code){HistoryError=TEXT("Device history: ")+Code+TEXT(". Successful sending is not repeated.");}
