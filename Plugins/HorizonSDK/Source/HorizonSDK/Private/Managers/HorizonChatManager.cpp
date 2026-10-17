#include "Managers/HorizonChatManager.h"
#include "Managers/HorizonRealtimeManager.h"
#include "Models/HorizonChatWire.h"
#include "Storage/HorizonChatHistory.h"
#include "GenericPlatform/GenericPlatformHttp.h"
namespace HorizonChatRequest
{
const FString Root=TEXT("/api/v1/app/chat");
FString Topic(const FString& C){return TEXT("chat:")+C;}
TSharedPtr<FJsonObject> Body(const FString& U){auto B=MakeShared<FJsonObject>();B->SetStringField(TEXT("userId"),U);return B;}
FString Query(const FString& U){return TEXT("?userId=")+FGenericPlatformHttp::UrlEncode(U);}
template<typename D> bool Failed(const FHorizonNetworkResponse& R,const D& Done){if(R.bSuccess)return false;Done.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage);return true;}
template<typename D> void Bad(const D& Done,const FString& Code=TEXT("INVALID_REQUEST")){Done.ExecuteIfBound(false,{},Code,Code);}
bool Denied(const FString& C){return C==TEXT("SESSION_REQUIRED")||C==TEXT("SESSION_FORBIDDEN")||C==TEXT("CHAT_NOT_AVAILABLE")||C==TEXT("CHANNEL_NOT_FOUND")||C==TEXT("CHAT_BANNED");}
bool Same(const FHorizonChatMessage& A,const FHorizonChatMessage& B){return A.Id==B.Id&&A.ChannelId==B.ChannelId&&A.AuthorId==B.AuthorId&&HorizonChatWire::Exact(A.Text,B.Text)&&A.CreatedAt==B.CreatedAt&&A.ExpiresAt==B.ExpiresAt;}
}
void UHorizonChatManager::Initialize(UHorizonHttpClient* H,UHorizonAuthManager* A,UHorizonRealtimeManager* R)
{
    if(Realtime){Realtime->OnSubscribed.RemoveDynamic(this,&UHorizonChatManager::OnSubscribed);Realtime->OnResyncRequired.RemoveDynamic(this,&UHorizonChatManager::OnTopic);Realtime->OnEvent.RemoveDynamic(this,&UHorizonChatManager::OnRealtimeEvent);Realtime->OnConnectionError.RemoveDynamic(this,&UHorizonChatManager::OnRealtimeError);Realtime->OnSubscriptionError.RemoveDynamic(this,&UHorizonChatManager::OnSubscriptionError);}HttpClient=H;AuthManager=A;Realtime=R;
    if(R){R->OnSubscribed.AddUniqueDynamic(this,&UHorizonChatManager::OnSubscribed);R->OnResyncRequired.AddUniqueDynamic(this,&UHorizonChatManager::OnTopic);R->OnEvent.AddUniqueDynamic(this,&UHorizonChatManager::OnRealtimeEvent);R->OnConnectionError.AddUniqueDynamic(this,&UHorizonChatManager::OnRealtimeError);R->OnSubscriptionError.AddUniqueDynamic(this,&UHorizonChatManager::OnSubscriptionError);}ClearCache();
}
FHorizonNetworkResponse UHorizonChatManager::Error(const FString& Code){FHorizonNetworkResponse R;R.ServerErrorCode=Code;R.ErrorMessage=Code;return R;}
void UHorizonChatManager::SyncIdentity()
{
    const FString HI=HttpClient&&AuthManager?HttpClient->GetApiKey()+TEXT("|")+AuthManager->GetCurrentUser().UserId:FString();
    if(HI!=HistoryIdentity){HistoryIdentity=HI;History.Reset();if(HttpClient&&AuthManager)History=MakeShared<FHorizonChatHistory>(HttpClient->GetApiKey(),AuthManager->GetCurrentUser().UserId);}
    const FString I=HttpClient&&AuthManager?HttpClient->GetApiKey()+TEXT("|")+AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();const uint64 C=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if(C==Context&&I==Identity)return;Identity=I;Context=C;++Generation;Limits={};ResponseMetadata={};
    TMap<FString,FChannelState> Old=MoveTemp(Channels);Channels.Empty();
    for(const auto& Row:Old){if(Realtime)Realtime->Unsubscribe(HorizonChatRequest::Topic(Row.Key));for(const auto& D:Row.Value.Waiters)D.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Session or host changed"));OnFeedChanged.Broadcast(Row.Key);}
}
FString UHorizonChatManager::Validate(const FString& U)
{
    SyncIdentity();if(!HttpClient||!AuthManager||!AuthManager->IsSignedIn()||HttpClient->GetSessionToken().IsEmpty()||AuthManager->GetCurrentUser().AccessToken!=HttpClient->GetSessionToken())return TEXT("SESSION_REQUIRED");if(U!=AuthManager->GetCurrentUser().UserId)return TEXT("SESSION_FORBIDDEN");if(!HorizonChatWire::Id(U))return TEXT("INVALID_REQUEST");return TEXT("");
}
void UHorizonChatManager::ClearCache()
{
    SyncIdentity();++Generation;Limits={};ResponseMetadata={};TMap<FString,FChannelState> Old=MoveTemp(Channels);Channels.Empty();for(const auto& Row:Old){if(Realtime)Realtime->Unsubscribe(HorizonChatRequest::Topic(Row.Key));for(const auto& D:Row.Value.Waiters)D.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Cache cleared"));OnFeedChanged.Broadcast(Row.Key);}
}
void UHorizonChatManager::Call(const FString& U,const FString& Route,const TSharedPtr<FJsonObject>& B,bool Read,FOnHttpResponse Done)
{
    const FString Failure=Validate(U);
    if(!Failure.IsEmpty()){Done.ExecuteIfBound(Error(Failure));return;}
    const uint64 G=Generation,C=Context;const FString BoundIdentity=Identity;
    TWeakObjectPtr<UHorizonChatManager> Weak(this);
    auto Callback=FOnHttpResponse::CreateLambda([Weak,G,C,BoundIdentity,Read,Route,Done](const FHorizonNetworkResponse& Response)
    {
        auto* S=Weak.Get();if(!S)return;S->SyncIdentity();
        // Clearing the feed fences reads, while an accepted write still belongs to its captured identity.
        if(S->Context!=C||S->Identity!=BoundIdentity||(Read&&S->Generation!=G)){Done.ExecuteIfBound(Error(TEXT("STALE_RESPONSE")));return;}
        FHorizonChatResponseMetadata Metadata;
        Metadata.StatusCode=Response.StatusCode;Metadata.ErrorCode=Response.GetErrorCodeString();
        Metadata.ErrorMessage=Response.ErrorMessage;Metadata.Headers=Response.Headers;
        if(!Response.bSuccess)
        {
            const FString Code=Response.GetErrorCodeString();TArray<FOnChatSyncedComplete> Pending;
            if(Code==TEXT("SESSION_REQUIRED")||Code==TEXT("SESSION_FORBIDDEN")||Code==TEXT("CHAT_NOT_AVAILABLE")||Code==TEXT("CHAT_BANNED"))
            {
                for(auto& Row:S->Channels){Pending.Append(MoveTemp(Row.Value.Waiters));Row.Value.Waiters.Empty();}
                S->ClearCache();
            }
            else if(Code==TEXT("CHANNEL_NOT_FOUND"))
            {
                const FString Prefix=HorizonChatRequest::Root+TEXT("/channels/");
                if(Route.StartsWith(Prefix))
                {
                    FString Id=Route.Mid(Prefix.Len());int32 Slash;if(Id.FindChar('/',Slash))Id=Id.Left(Slash);
                    if(auto* Channel=S->Channels.Find(Id)){Pending=MoveTemp(Channel->Waiters);Channel->Waiters.Empty();}
                    if(HorizonChatWire::Id(Id))S->UnwatchChannel(Id);
                }
            }
            S->ResponseMetadata=Metadata;
            for(const auto& D:Pending)D.ExecuteIfBound(false,{},Code,Response.ErrorMessage);
            Done.ExecuteIfBound(Response);return;
        }
        S->ResponseMetadata=Metadata;
        FHorizonNetworkResponse R=Response;
        if(!HorizonChatWire::Parse(R.RawJsonBody,R.JsonData)){Done.ExecuteIfBound(Error(TEXT("INVALID_RESPONSE")));return;}
        Done.ExecuteIfBound(R);
    });
    if(!B)HttpClient->Get(Route,true,Callback);
    else if(Read)HttpClient->PostJsonRead(B.ToSharedRef(),Route,Callback);
    else HttpClient->JsonOnce(B.ToSharedRef(),Route,false,true,Callback);
}
void UHorizonChatManager::GetLimits(const FString& U,FOnChatLimitsViewComplete D)
{
    Call(U,HorizonChatRequest::Root+TEXT("/limits")+HorizonChatRequest::Query(U),nullptr,true,FOnHttpResponse::CreateWeakLambda(this,[this,D](const FHorizonNetworkResponse& R){if(HorizonChatRequest::Failed(R,D))return;FHorizonChatLimitsView V;if(!FHorizonChatLimitsView::TryFromJson(R.JsonData,V)){HorizonChatRequest::Bad(D,TEXT("INVALID_RESPONSE"));return;}if(Limits.bPresent&&V.ServerTime<Limits.ServerTime){HorizonChatRequest::Bad(D,TEXT("STALE_RESPONSE"));return;}Limits=V;D.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
FHorizonChatResponseMetadata UHorizonChatManager::GetLastResponseMetadata(){SyncIdentity();return ResponseMetadata;}
FHorizonChatLimitsView UHorizonChatManager::GetCurrentLimits(){SyncIdentity();return Limits;}
void UHorizonChatManager::OpenChannel(const FString& U,const FHorizonChatOpenInput& I,FOnChatOpenedComplete D)
{
    if((I.Kind==TEXT("direct")&&(!HorizonChatWire::Id(I.PeerId)||I.PeerId==U||!I.GroupId.IsEmpty()))||(I.Kind==TEXT("group")&&(!HorizonChatWire::Id(I.GroupId)||!I.PeerId.IsEmpty()))||(I.Kind==TEXT("project")&&(!I.GroupId.IsEmpty()||!I.PeerId.IsEmpty()))||(I.Kind!=TEXT("direct")&&I.Kind!=TEXT("group")&&I.Kind!=TEXT("project"))){HorizonChatRequest::Bad(D);return;}
    auto B=HorizonChatRequest::Body(U);B->SetStringField(TEXT("kind"),I.Kind);if(!I.PeerId.IsEmpty())B->SetStringField(TEXT("peerId"),I.PeerId);if(!I.GroupId.IsEmpty())B->SetStringField(TEXT("groupId"),I.GroupId);
    Call(U,HorizonChatRequest::Root+TEXT("/channels/open"),B,false,FOnHttpResponse::CreateWeakLambda(this,[this,U,I,D](const FHorizonNetworkResponse& R){if(HorizonChatRequest::Failed(R,D))return;FHorizonChatOpened V;if(!FHorizonChatOpened::TryFromJson(R.JsonData,V)||V.Channel.Kind!=I.Kind||(I.Kind==TEXT("group")&&V.Channel.GroupId!=I.GroupId)||(I.Kind==TEXT("direct")&&(!V.Channel.PeerIds.Contains(U)||!V.Channel.PeerIds.Contains(I.PeerId)))){HorizonChatRequest::Bad(D,TEXT("INVALID_RESPONSE"));return;}D.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
bool UHorizonChatManager::WatchChannel(const FString& U,const FString& C)
{
    if(!Validate(U).IsEmpty()||!HorizonChatWire::Id(C)||!Realtime||(Channels.Num()>=32&&!Channels.Contains(C)))return false;
    if(Channels.Contains(C))return Realtime->Subscribe(HorizonChatRequest::Topic(C));if(!Realtime->Subscribe(HorizonChatRequest::Topic(C)))return false;auto& S=Channels.Add(C);S.RealtimeGeneration=Realtime->GetConnectionGeneration();S.Serial=++NextSerial;S.Dirty=true;
    if(Realtime->IsSubscribed(HorizonChatRequest::Topic(C)))StartSync(C);return true;
}
void UHorizonChatManager::UnwatchChannel(const FString& C)
{
    FChannelState S;if(!Channels.RemoveAndCopyValue(C,S))return;if(Realtime)Realtime->Unsubscribe(HorizonChatRequest::Topic(C));for(const auto& D:S.Waiters)D.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Channel selection changed"));OnFeedChanged.Broadcast(C);
}
bool UHorizonChatManager::IsChannelSubscribed(const FString& C){SyncIdentity();return Channels.Contains(C)&&Realtime&&Realtime->IsSubscribed(HorizonChatRequest::Topic(C));}
FHorizonChatFeed UHorizonChatManager::GetFeed(const FString& C)
{
    SyncIdentity();auto* S=Channels.Find(C);if(!S||!S->Visible||!S->Complete||!Realtime||!Realtime->IsSubscribed(HorizonChatRequest::Topic(C))||Realtime->GetConnectionGeneration()!=S->RealtimeGeneration)return {};
    FDateTime Server;FDateTime::ParseIso8601(*S->Feed.ServerTime,Server);const FDateTime Now=FMath::Max(FDateTime::UtcNow(),Server+FTimespan::FromSeconds(FMath::Max(0.0,FPlatformTime::Seconds()-S->SynchronizedAt)));
    FDateTime Expiry;FDateTime::ParseIso8601(*S->Feed.Channel.ExpiresAt,Expiry);if(Expiry<=Now)return {};FHorizonChatFeed F=S->Feed;F.Messages.RemoveAll([&](const FHorizonChatMessage& M){FDateTime E;FDateTime::ParseIso8601(*M.ExpiresAt,E);return E<=Now;});return F;
}
void UHorizonChatManager::Hide(const FString& C,bool Forget)
{
    auto* S=Channels.Find(C);if(!S)return;S->Visible=false;S->Dirty=true;if(Forget){++S->Authority;S->Complete=false;S->Feed={};}OnFeedChanged.Broadcast(C);
}
void UHorizonChatManager::SyncChannel(const FString& U,const FString& C,FOnChatSyncedComplete D)
{
    const FString Failure=Validate(U);if(!Failure.IsEmpty()){HorizonChatRequest::Bad(D,Failure);return;}if(!HorizonChatWire::Id(C)){HorizonChatRequest::Bad(D);return;}if(!IsChannelSubscribed(C))
    {
        const FString Captured=History?History->Capture(C):FString();auto B=HorizonChatRequest::Body(U);B->SetField(TEXT("epoch"),MakeShared<FJsonValueNull>());B->SetArrayField(TEXT("knownMessageIds"),{});
        Call(U,HorizonChatRequest::Root+TEXT("/channels/")+C+TEXT("/sync"),B,true,FOnHttpResponse::CreateWeakLambda(this,[this,C,Captured,D](const FHorizonNetworkResponse& R){if(HorizonChatRequest::Failed(R,D))return;FHorizonChatSynced V;if(!FHorizonChatSynced::TryFromJson(R.JsonData,V)||V.Channel.Id!=C||V.Messages.Num()!=V.VisibleMessageIds.Num()){HorizonChatRequest::Bad(D,TEXT("INVALID_RESPONSE"));return;}const uint64 BeforeHistory=Generation;Archive(C,V.Messages,Captured);SyncIdentity();if(Generation!=BeforeHistory){HorizonChatRequest::Bad(D,TEXT("STALE_RESPONSE"));return;}D.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));return;
    }
    auto* S=Channels.Find(C);if(S->Waiters.Num()>=64){HorizonChatRequest::Bad(D,TEXT("REQUEST_IN_PROGRESS"));return;}S->Waiters.Add(D);if(!S->Busy)StartSync(C);
}
void UHorizonChatManager::FinishSync(const FString& C,bool Ok,const FHorizonChatSynced& V,const FString& Code,const FString& Message)
{
    auto* S=Channels.Find(C);if(!S)return;TArray<FOnChatSyncedComplete> D=MoveTemp(S->Waiters);S->Waiters.Empty();for(const auto& W:D)W.ExecuteIfBound(Ok,V,Code,Message);
}
void UHorizonChatManager::StartSync(const FString& C)
{
    auto* S=Channels.Find(C);if(!S||S->Busy||!IsChannelSubscribed(C))return;S=Channels.Find(C);if(!S)return;
    S->Busy=true;S->Dirty=false;const uint64 Serial=S->Serial,G=Generation,R=Realtime->GetConnectionGeneration(),A=S->Authority;S->RealtimeGeneration=R;
    auto B=HorizonChatRequest::Body(AuthManager->GetCurrentUser().UserId);TArray<TSharedPtr<FJsonValue>> Ids;
    if(S->Complete){B->SetStringField(TEXT("epoch"),S->Feed.Channel.Epoch);for(const auto& M:S->Feed.Messages)Ids.Add(MakeShared<FJsonValueString>(M.Id));}else B->SetField(TEXT("epoch"),MakeShared<FJsonValueNull>());B->SetArrayField(TEXT("knownMessageIds"),Ids);
    const FHorizonChatFeed Old=S->Complete?S->Feed:FHorizonChatFeed();const FString Captured=History?History->Capture(C):FString();
    Call(AuthManager->GetCurrentUser().UserId,HorizonChatRequest::Root+TEXT("/channels/")+C+TEXT("/sync"),B,true,FOnHttpResponse::CreateWeakLambda(this,[this,C,Serial,G,R,A,Old,Captured](const FHorizonNetworkResponse& Response)
    {
        auto* S=Channels.Find(C);if(!S||S->Serial!=Serial||G!=Generation)return;S->Busy=false;
        if(S->Authority!=A){if(IsChannelSubscribed(C))StartSync(C);else FinishSync(C,false,{},TEXT("STALE_RESPONSE"),TEXT("Channel authority changed"));return;}
        if(!Realtime||!Realtime->IsSubscribed(HorizonChatRequest::Topic(C))||Realtime->GetConnectionGeneration()!=R){Hide(C,true);FinishSync(C,false,{},TEXT("STALE_RESPONSE"),TEXT("Subscription changed"));return;}
        FHorizonChatSynced V;FString Code=Response.bSuccess?TEXT(""):Response.GetErrorCodeString();if(Code.IsEmpty()&&(!FHorizonChatSynced::TryFromJson(Response.JsonData,V)||V.Channel.Id!=C))Code=TEXT("INVALID_RESPONSE");
        if(Code.IsEmpty()&&((!S->ServerFloor.IsEmpty()&&V.ServerTime<S->ServerFloor)||(Old.bPresent&&V.Channel.Epoch==Old.Channel.Epoch&&V.Channel.Revision<Old.Channel.Revision)))Code=TEXT("STALE_RESPONSE");
        FHorizonChatFeed New;New.Channel=V.Channel;New.Moderation=V.Moderation;New.ServerTime=V.ServerTime;
        if(Code.IsEmpty())for(const auto& Id:V.VisibleMessageIds)
        {
            const auto* Incoming=V.Messages.FindByPredicate([&](const FHorizonChatMessage& M){return M.Id==Id;});const auto* Cached=Old.bPresent&&Old.Channel.Epoch==V.Channel.Epoch?Old.Messages.FindByPredicate([&](const FHorizonChatMessage& M){return M.Id==Id;}):nullptr;
            if((!Incoming&&!Cached)||(Incoming&&Cached&&!HorizonChatRequest::Same(*Incoming,*Cached))){Code=TEXT("INVALID_RESPONSE");break;}const auto& M=Incoming?*Incoming:*Cached;
            if(M.ChannelId!=C||(!New.Messages.IsEmpty()&&(M.CreatedAt<New.Messages.Last().CreatedAt||(M.CreatedAt==New.Messages.Last().CreatedAt&&M.Id<=New.Messages.Last().Id)))){Code=TEXT("INVALID_RESPONSE");break;}New.Messages.Add(M);
        }
        if(!Code.IsEmpty())
        {
            Hide(C,true);FinishSync(C,false,{},Code,Response.ErrorMessage.IsEmpty()?Code:Response.ErrorMessage);if(HorizonChatRequest::Denied(Code))UnwatchChannel(C);return;
        }
        New.bPresent=true;S->Feed=MoveTemp(New);S->Complete=true;S->ServerFloor=V.ServerTime;S->SynchronizedAt=FPlatformTime::Seconds();
        if(S->Dirty){S->Visible=false;StartSync(C);return;}S->Visible=true;Archive(C,S->Feed.Messages,Captured);SyncIdentity();auto* Current=Channels.Find(C);if(G!=Generation||!Current||Current->Serial!=Serial||Current->Authority!=A)return;OnFeedChanged.Broadcast(C);SyncIdentity();Current=Channels.Find(C);if(G!=Generation||!Current||Current->Serial!=Serial||Current->Authority!=A)return;FinishSync(C,true,V,TEXT(""),TEXT(""));
    }));
}
void UHorizonChatManager::OnSubscribed(const FString& Topic){if(!Topic.StartsWith(TEXT("chat:")))return;const FString C=Topic.Mid(5);if(!Channels.Contains(C))return;Hide(C,true);/* The protocol sends SUBSCRIBED resync after its authorized ACK. */}
void UHorizonChatManager::OnTopic(const FString& Topic){if(!Topic.StartsWith(TEXT("chat:")))return;const FString C=Topic.Mid(5);if(!Channels.Contains(C))return;const bool Authorized=Realtime&&Realtime->IsSubscribed(Topic);Hide(C,!Authorized);if(Authorized)StartSync(C);}
void UHorizonChatManager::OnRealtimeEvent(const FString& Json){TSharedPtr<FJsonObject> J;FString Topic,Type,Version;if(!HorizonChatWire::Parse(Json,J)||!J->TryGetStringField(TEXT("type"),Type)||Type!=TEXT("event")||!J->TryGetStringField(TEXT("topic"),Topic))return;auto V=J->TryGetField(TEXT("version"));if(V&&V->Type==EJson::Number&&V->TryGetString(Version)&&Version==TEXT("1"))OnTopic(Topic);}
void UHorizonChatManager::OnSubscriptionError(const FString& Topic,const FString& Code){if(Topic.StartsWith(TEXT("chat:"))&&Code==TEXT("SUBSCRIPTION_FORBIDDEN"))UnwatchChannel(Topic.Mid(5));}
void UHorizonChatManager::OnRealtimeError(const FString& Code){if(Code==TEXT("SUBSCRIPTION_FORBIDDEN")||(Realtime&&Realtime->bIsReady&&Code!=TEXT("SESSION_REVOKED")&&Code!=TEXT("NOT_SUPPORTED")))return;TArray<FString> Ids;Channels.GetKeys(Ids);for(const auto& C:Ids){Hide(C,true);if(Code==TEXT("SUBSCRIPTION_FORBIDDEN")||Code==TEXT("SESSION_REVOKED")||Code==TEXT("NOT_SUPPORTED"))UnwatchChannel(C);}}
void UHorizonChatManager::Tick(float Delta)
{
    SyncIdentity();TArray<FString> Ids;Channels.GetKeys(Ids);for(const auto& C:Ids){auto* S=Channels.Find(C);if(S&&(S->Visible||S->Complete)&&(!Realtime||!Realtime->IsSubscribed(HorizonChatRequest::Topic(C))||S->RealtimeGeneration!=Realtime->GetConnectionGeneration()))Hide(C,true);}
}
void UHorizonChatManager::SendMessage(const FString& U,const FString& C,const FString& Id,const FString& At,const FString& Text,FOnChatSentComplete D)
{
    if(!HorizonChatWire::Id(C)||!HorizonChatWire::Id(Id)||!HorizonChatWire::Time(At)||!HorizonChatWire::Text(Text)){HorizonChatRequest::Bad(D);return;}auto B=HorizonChatRequest::Body(U);B->SetStringField(TEXT("operationId"),Id);B->SetStringField(TEXT("issuedAt"),At);B->SetStringField(TEXT("text"),Text);
    SyncIdentity();const FString Captured=History?History->Capture(C):FString();
    Call(U,HorizonChatRequest::Root+TEXT("/channels/")+C+TEXT("/messages"),B,false,FOnHttpResponse::CreateWeakLambda(this,[this,U,C,Id,At,Text,Captured,D](const FHorizonNetworkResponse& R){if(HorizonChatRequest::Failed(R,D))return;FHorizonChatSent V;if(!FHorizonChatSent::TryFromJson(R.JsonData,V)||V.Receipt.OperationId!=Id||V.Receipt.ChannelId!=C||V.Receipt.IssuedAt!=At){HorizonChatRequest::Bad(D,TEXT("INVALID_RESPONSE"));return;}FHorizonChatMessage M;M.Id=V.Receipt.MessageId;M.ChannelId=C;M.AuthorId=U;M.Text=Text;M.CreatedAt=V.Receipt.CreatedAt;M.ExpiresAt=V.Receipt.ExpiresAt;const uint64 WriteContext=Context;const FString WriteIdentity=Identity;Archive(C,{M},Captured);SyncIdentity();if(Context!=WriteContext||Identity!=WriteIdentity){HorizonChatRequest::Bad(D,TEXT("STALE_RESPONSE"));return;}D.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonChatManager::GetOperation(const FString& U,const FString& Id,FOnChatOperationComplete D)
{
    if(!HorizonChatWire::Id(Id)){HorizonChatRequest::Bad(D);return;}Call(U,HorizonChatRequest::Root+TEXT("/operations/")+Id+HorizonChatRequest::Query(U),nullptr,true,FOnHttpResponse::CreateWeakLambda(this,[Id,D](const FHorizonNetworkResponse& R){if(HorizonChatRequest::Failed(R,D))return;FHorizonChatOperation V;if(!FHorizonChatOperation::TryFromJson(R.JsonData,V)||V.Receipt.OperationId!=Id){HorizonChatRequest::Bad(D,TEXT("INVALID_RESPONSE"));return;}D.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonChatManager::ReportMessage(const FString& U,const FString& C,const FString& Id,const FString& Reason,FOnChatReportedComplete D)
{
    if(!HorizonChatWire::Id(C)||!HorizonChatWire::Id(Id)||!HorizonChatWire::Text(Reason,240,960)){HorizonChatRequest::Bad(D);return;}auto B=HorizonChatRequest::Body(U);B->SetStringField(TEXT("reason"),Reason);
    Call(U,HorizonChatRequest::Root+TEXT("/channels/")+C+TEXT("/messages/")+Id+TEXT("/report"),B,false,FOnHttpResponse::CreateWeakLambda(this,[D](const FHorizonNetworkResponse& R){if(HorizonChatRequest::Failed(R,D))return;FHorizonChatReported V;if(!FHorizonChatReported::TryFromJson(R.JsonData,V)){HorizonChatRequest::Bad(D,TEXT("INVALID_RESPONSE"));return;}D.ExecuteIfBound(true,V,TEXT(""),TEXT(""));}));
}
void UHorizonChatManager::BeginDestroy(){ClearCache();Initialize(nullptr,nullptr,nullptr);Super::BeginDestroy();}

FHorizonChatHistoryPage UHorizonChatManager::ReadLocalHistory(const FString& C,const FString& Cursor,int32 Limit)
{
    const FString Failure=Validate(AuthManager?AuthManager->GetCurrentUser().UserId:FString());FHorizonChatHistoryPage P;if(!Failure.IsEmpty()||!History){P.ErrorCode=Failure.IsEmpty()?TEXT("SESSION_REQUIRED"):Failure;return P;}P=History->Read(C,Cursor,Limit);if(!P.bSuccess)OnLocalHistoryFailed.Broadcast(C,P.ErrorCode);return P;
}
bool UHorizonChatManager::ClearLocalHistory(const FString& C)
{
    if(!HorizonChatWire::Id(C))return false;const FString Failure=Validate(AuthManager?AuthManager->GetCurrentUser().UserId:FString());if(!Failure.IsEmpty()||!History)return false;FString Code;if(!History->Clear(C,Code)){OnLocalHistoryFailed.Broadcast(C,Code);return false;}OnLocalHistoryChanged.Broadcast(C);return true;
}
bool UHorizonChatManager::ClearAllLocalHistory()
{
    const FString Failure=Validate(AuthManager?AuthManager->GetCurrentUser().UserId:FString());if(!Failure.IsEmpty()||!History)return false;FString Code;if(!History->Clear(TEXT(""),Code)){OnLocalHistoryFailed.Broadcast(TEXT(""),Code);return false;}OnLocalHistoryChanged.Broadcast(TEXT(""));return true;
}
void UHorizonChatManager::Archive(const FString& C,TArray<FHorizonChatMessage> Rows,const FString& Captured)
{
    if(!History)return;auto Target=History;bool Any=false;for(const auto& M:Rows){SyncIdentity();if(Target!=History)return;bool Changed=false;FString Code;if(!Target->Append(C,M,Captured,Changed,Code)){if(Code!=TEXT("LOCAL_HISTORY_STALE"))OnLocalHistoryFailed.Broadcast(C,Code);}Any|=Changed;}if(Any)OnLocalHistoryChanged.Broadcast(C);
}
FString UHorizonChatManager::CaptureLocalHistory(const FString& C){SyncIdentity();return History?History->Capture(C):FString();}
void UHorizonChatManager::ArchiveConfirmedSend(const FString& U,const FString& Text,const FHorizonChatReceipt& R,const FString& Captured)
{
    if(!Validate(U).IsEmpty()||!History||!HorizonChatWire::Id(R.MessageId)||!HorizonChatWire::Id(R.ChannelId))return;FHorizonChatMessage M;M.Id=R.MessageId;M.ChannelId=R.ChannelId;M.AuthorId=U;M.Text=Text;M.CreatedAt=R.CreatedAt;M.ExpiresAt=R.ExpiresAt;Archive(R.ChannelId,{M},Captured);
}

void UHorizonChatManager::RequestChatLimits(const FString& UserId){GetLimits(UserId,FOnChatLimitsViewComplete::CreateWeakLambda(this,[this](bool B,const FHorizonChatLimitsView& V,const FString& C,const FString& M){OnLimitsViewResult.Broadcast(B,V,C,M);}));}

void UHorizonChatManager::OpenChatChannel(const FString& UserId,const FHorizonChatOpenInput& Input){OpenChannel(UserId,Input,FOnChatOpenedComplete::CreateWeakLambda(this,[this](bool B,const FHorizonChatOpened& V,const FString& C,const FString& M){OnOpenedResult.Broadcast(B,V,C,M);}));}

void UHorizonChatManager::RequestChatSync(const FString& UserId,const FString& ChannelId){SyncChannel(UserId,ChannelId,FOnChatSyncedComplete::CreateWeakLambda(this,[this](bool B,const FHorizonChatSynced& V,const FString& C,const FString& M){OnSyncedResult.Broadcast(B,V,C,M);}));}

void UHorizonChatManager::SendChatMessage(const FString& UserId,const FString& ChannelId,const FString& OperationId,const FString& IssuedAt,const FString& Text){SendMessage(UserId,ChannelId,OperationId,IssuedAt,Text,FOnChatSentComplete::CreateWeakLambda(this,[this](bool B,const FHorizonChatSent& V,const FString& C,const FString& M){OnSentResult.Broadcast(B,V,C,M);}));}

void UHorizonChatManager::RequestChatOperation(const FString& UserId,const FString& OperationId){GetOperation(UserId,OperationId,FOnChatOperationComplete::CreateWeakLambda(this,[this](bool B,const FHorizonChatOperation& V,const FString& C,const FString& M){OnOperationResult.Broadcast(B,V,C,M);}));}

void UHorizonChatManager::ReportChatMessage(const FString& UserId,const FString& ChannelId,const FString& MessageId,const FString& Reason){ReportMessage(UserId,ChannelId,MessageId,Reason,FOnChatReportedComplete::CreateWeakLambda(this,[this](bool B,const FHorizonChatReported& V,const FString& C,const FString& M){OnReportedResult.Broadcast(B,V,C,M);}));}
