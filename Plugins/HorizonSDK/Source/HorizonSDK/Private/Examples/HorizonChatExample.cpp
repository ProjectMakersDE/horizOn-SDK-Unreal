#include "Examples/HorizonChatExample.h"
#include "Examples/HorizonChatExampleWidget.h"
#include "Examples/HorizonChatJournal.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonChatManager.h"
#include "Managers/HorizonAuthManager.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
UHorizonSubsystem* AHorizonChatExample::Session() const{auto* G=GetGameInstance();auto* S=G?G->GetSubsystem<UHorizonSubsystem>():nullptr;return S&&S->Auth&&S->Auth->IsSignedIn()&&S->Chat?S:nullptr;}
void AHorizonChatExample::Open(const FHorizonChatOpenInput& I)
{
    auto* S=Session();if(!S||Busy)return;CloseChannel();const uint64 C=S->GetHttpClient()->GetRequestContextEpoch(),SelectionAtOpen=++Selection;Busy=true;
    S->Chat->OpenChannel(S->Auth->GetCurrentUser().UserId,I,FOnChatOpenedComplete::CreateWeakLambda(this,[this,C,SelectionAtOpen](bool Ok,const FHorizonChatOpened& V,const FString& Code,const FString&){Busy=false;auto* S=Session();if(!S||S->GetHttpClient()->GetRequestContextEpoch()!=C||SelectionAtOpen!=Selection){LastErrorCode=TEXT("STALE_RESPONSE");return;}if(!Ok){LastErrorCode=Code;return;}ChannelId=V.Channel.Id;LastErrorCode=S->Chat->WatchChannel(S->Auth->GetCurrentUser().UserId,ChannelId)?TEXT(""):TEXT("SUBSCRIPTION_UNAVAILABLE");}));
}
void AHorizonChatExample::CloseChannel(){++Selection;auto* S=Session();if(S&&!ChannelId.IsEmpty())S->Chat->UnwatchChannel(ChannelId);ChannelId.Reset();}
FHorizonChatFeed AHorizonChatExample::GetFeed() const{auto* S=Session();return S?S->Chat->GetFeed(ChannelId):FHorizonChatFeed{};}
bool AHorizonChatExample::IsSubscribed() const{auto* S=Session();return S&&S->Chat->IsChannelSubscribed(ChannelId);}
void AHorizonChatExample::SendText(const FString& Text){Prepare(TEXT("send"),Text,TEXT(""));}
void AHorizonChatExample::Report(const FString& Id,const FString& Reason){Prepare(TEXT("report"),Reason,Id);}
void AHorizonChatExample::Prepare(const FString& Action,const FString& Text,const FString& Message)
{
    auto* S=Session();if(!S||Busy||!GetFeed().bPresent||!HorizonChatWire::Text(Text,Action==TEXT("report")?240:500,Action==TEXT("report")?960:2000)){LastErrorCode=TEXT("INVALID_REQUEST");return;}
    if(Action==TEXT("report")&&!GetFeed().Messages.ContainsByPredicate([&](const FHorizonChatMessage& M){return M.Id==Message;})){LastErrorCode=TEXT("MESSAGE_NOT_FOUND");return;}
    const FString P=HorizonChatJournal::Project(S->GetHttpClient()->GetApiKey()),U=S->Auth->GetCurrentUser().UserId,N=HorizonChatJournal::Name(P,U);TStrongObjectPtr<UHorizonChatOriginal> Original;
    {
        HorizonChatJournal::FExclusiveLock Lock(N);if(!Lock.IsValid()){LastErrorCode=TEXT("JOURNAL_BUSY");return;}bool Damaged;auto* Existing=HorizonChatJournal::Load(N,Damaged);if(Damaged||(Existing&&!HorizonChatJournal::Valid(Existing,P,U))){LastErrorCode=TEXT("JOURNAL_DAMAGED");return;}if(Existing&&Existing->State!=TEXT("acknowledged")){LastErrorCode=TEXT("UNRESOLVED_ORIGINAL");return;}
        auto* O=NewObject<UHorizonChatOriginal>();O->Project=P;O->UserId=U;O->Action=Action;O->OperationId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower();O->IssuedAt=FDateTime::UtcNow().ToIso8601();O->ChannelId=ChannelId;O->MessageId=Message;O->TextBase64=HorizonChatJournal::Encode(Text);if(!HorizonChatJournal::Save(N,O)){LastErrorCode=TEXT("JOURNAL_WRITE_FAILED");return;}Original.Reset(O);
    }Transmit(Original.Get());
}
void AHorizonChatExample::Transmit(UHorizonChatOriginal* O)
{
    auto* S=Session();if(!S||Busy||!O)return;const FString P=HorizonChatJournal::Project(S->GetHttpClient()->GetApiKey()),U=S->Auth->GetCurrentUser().UserId,N=HorizonChatJournal::Name(P,U);TStrongObjectPtr<UHorizonChatOriginal> Original;
    {
        HorizonChatJournal::FExclusiveLock Lock(N);if(!Lock.IsValid()){LastErrorCode=TEXT("JOURNAL_BUSY");return;}bool Damaged;auto* Saved=HorizonChatJournal::Load(N,Damaged);if(Damaged||!HorizonChatJournal::Valid(Saved,P,U)||Saved->State!=TEXT("pending")||HorizonChatJournal::OriginalHash(Saved)!=HorizonChatJournal::OriginalHash(O)){LastErrorCode=TEXT("UNRESOLVED_ORIGINAL");return;}++Saved->SendSerial;if(!HorizonChatJournal::Save(N,Saved)){LastErrorCode=TEXT("JOURNAL_WRITE_FAILED");return;}Original.Reset(HorizonChatJournal::Copy(Saved));
    }
    Busy=true;MissingOriginal.Reset();const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();FString Text;if(!HorizonChatJournal::Decode(Original->TextBase64,Text)){Busy=false;LastErrorCode=TEXT("JOURNAL_DAMAGED");return;}
    const auto Complete=[this,N,Original,Context](bool Ok,const FHorizonChatReceipt& R,const FString& ReportId,const FString& Status,const FString& Code)
    {
        Busy=false;auto* S=Session();if(!S||S->GetHttpClient()->GetRequestContextEpoch()!=Context){LastErrorCode=TEXT("STALE_RESPONSE");return;}if(!Ok){LastErrorCode=Code;if(Code==TEXT("OPERATION_EXPIRED")||Code==TEXT("REPORT_EXPIRED")){HorizonChatJournal::FExclusiveLock Lock(N);if(Lock.IsValid()){bool Damaged;auto* P=HorizonChatJournal::Load(N,Damaged);if(!Damaged&&P&&HorizonChatJournal::OriginalHash(P)==HorizonChatJournal::OriginalHash(Original.Get())&&P->State==TEXT("pending")){P->State=TEXT("expired");HorizonChatJournal::Save(N,P);}}}return;}
        LastErrorCode=HorizonChatJournal::Acknowledge(N,Original.Get(),R,ReportId,Status)?TEXT(""):TEXT("JOURNAL_ACK_FAILED");
    };
    if(Original->Action==TEXT("send"))S->Chat->SendMessage(U,Original->ChannelId,Original->OperationId,Original->IssuedAt,Text,FOnChatSentComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonChatSent& V,const FString& C,const FString&){Complete(Ok,V.Receipt,TEXT(""),TEXT(""),C);}));
    else S->Chat->ReportMessage(U,Original->ChannelId,Original->MessageId,Text,FOnChatReportedComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonChatReported& V,const FString& C,const FString&){Complete(Ok,{},V.ReportId,V.Status,C);}));
}
void AHorizonChatExample::ReconcileOriginal()
{
    auto* S=Session();if(!S||Busy)return;const FString P=HorizonChatJournal::Project(S->GetHttpClient()->GetApiKey()),U=S->Auth->GetCurrentUser().UserId,N=HorizonChatJournal::Name(P,U);TStrongObjectPtr<UHorizonChatOriginal> O;
    {HorizonChatJournal::FExclusiveLock Lock(N);if(!Lock.IsValid()){LastErrorCode=TEXT("JOURNAL_BUSY");return;}bool Damaged;auto* Saved=HorizonChatJournal::Load(N,Damaged);if(Damaged||!HorizonChatJournal::Valid(Saved,P,U)){LastErrorCode=TEXT("JOURNAL_DAMAGED");return;}if(Saved->State==TEXT("acknowledged")){LastErrorCode.Reset();return;}O.Reset(HorizonChatJournal::Copy(Saved));}
    if(O->Action==TEXT("report")){LastErrorCode=TEXT("EXPLICIT_ORIGINAL_REPORT_REPLAY_REQUIRED");return;}const FString HistoryGeneration=S->Chat->CaptureLocalHistory(O->ChannelId);Busy=true;MissingOriginal.Reset();const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();
    S->Chat->GetOperation(U,O->OperationId,FOnChatOperationComplete::CreateWeakLambda(this,[this,O,N,Context,HistoryGeneration](bool Ok,const FHorizonChatOperation& V,const FString& Code,const FString&){Busy=false;auto* S=Session();if(!S||S->GetHttpClient()->GetRequestContextEpoch()!=Context){LastErrorCode=TEXT("STALE_RESPONSE");return;}if(!Ok){LastErrorCode=Code;if(Code==TEXT("OPERATION_NOT_FOUND")){MissingOriginal=HorizonChatJournal::OriginalHash(O.Get());MissingContext=Context;}return;}FString OriginalText;if(V.Receipt.ChannelId==O->ChannelId&&V.Receipt.IssuedAt==O->IssuedAt&&HorizonChatJournal::Decode(O->TextBase64,OriginalText))S->Chat->ArchiveConfirmedSend(O->UserId,OriginalText,V.Receipt,HistoryGeneration);LastErrorCode=HorizonChatJournal::Acknowledge(N,O.Get(),V.Receipt)?TEXT(""):TEXT("JOURNAL_ACK_FAILED");}));
}
void AHorizonChatExample::RetryOriginal()
{
    auto* S=Session();if(!S||Busy)return;const FString P=HorizonChatJournal::Project(S->GetHttpClient()->GetApiKey()),U=S->Auth->GetCurrentUser().UserId,N=HorizonChatJournal::Name(P,U);TStrongObjectPtr<UHorizonChatOriginal> O;
    {HorizonChatJournal::FExclusiveLock Lock(N);if(!Lock.IsValid()){LastErrorCode=TEXT("JOURNAL_BUSY");return;}bool Damaged;auto* Saved=HorizonChatJournal::Load(N,Damaged);if(Damaged||!HorizonChatJournal::Valid(Saved,P,U)){LastErrorCode=TEXT("JOURNAL_DAMAGED");return;}if(Saved->State==TEXT("acknowledged")){LastErrorCode.Reset();return;}if(Saved->State==TEXT("expired")){LastErrorCode=TEXT("UNRESOLVED_EXPIRED_ORIGINAL");return;}O.Reset(HorizonChatJournal::Copy(Saved));}
    if(O->Action==TEXT("send")&&(MissingContext!=S->GetHttpClient()->GetRequestContextEpoch()||MissingOriginal!=HorizonChatJournal::OriginalHash(O.Get()))){ReconcileOriginal();return;}Transmit(O.Get());
}
void AHorizonChatExample::ShowUI(){if(Widget)return;Widget=CreateWidget<UHorizonChatExampleWidget>(GetWorld(),UHorizonChatExampleWidget::StaticClass());if(Widget){Widget->Example=this;Widget->AddToViewport();}}
void AHorizonChatExample::EndPlay(const EEndPlayReason::Type Reason){CloseChannel();if(Widget)Widget->RemoveFromParent();Widget=nullptr;Super::EndPlay(Reason);}
