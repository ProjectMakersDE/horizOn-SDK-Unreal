// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonRealtimeManager.h"
#include "Managers/HorizonAuthManager.h"
#include "Http/HorizonHttpClient.h"
#include "Http/HorizonLosslessJson.h"
#include "HorizonConfig.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Internationalization/Regex.h"

namespace HorizonRealtime
{
// Socket v2 subscribed snapshots and results carry whole chat syncs and match states.
constexpr int32 MaxFrameChars = 2 * 1024 * 1024;
constexpr double TicketMaxAgeSeconds = 9 * 60;
const FString OutcomeUnknown = TEXT("REALTIME_OUTCOME_UNKNOWN");
FHorizonNetworkResponse Unknown()
{
    FHorizonNetworkResponse R; R.ErrorCode = EHorizonErrorCode::ConnectionFailed;
    R.ServerErrorCode = OutcomeUnknown; R.ErrorMessage = TEXT("Realtime send outcome unknown"); return R;
}
TSharedPtr<FJsonObject> Lossless(const FString& Text)
{
    TSharedPtr<FJsonObject> Frame;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Frame, FJsonSerializer::EFlags::StoreNumbersAsStrings)) return nullptr;
    return Frame;
}
}

void UHorizonRealtimeManager::Initialize(UHorizonHttpClient* Client, UHorizonAuthManager* Auth)
{
    HttpClient = Client; AuthManager = Auth;
    ClientId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower();
}
uint64 UHorizonRealtimeManager::GetConnectionGeneration(){SyncIdentity();return uint64(Generation);}
bool UHorizonRealtimeManager::IsSubscribed(const FString& Topic){SyncIdentity();return bIsReady&&Sent.Contains(Topic);}
bool UHorizonRealtimeManager::Subscribe(const FString& Topic)
{
    SyncIdentity();
    FRegexMatcher Match(FRegexPattern(TEXT("^(inbox|matches|chat|achievements|friends|groups):[A-Za-z0-9._-]{1,96}$")), Topic);
    if (!Match.FindNext() || (Topics.Num() >= 32 && !Topics.Contains(Topic))) return false;
    Topics.Add(Topic); return true;
}
void UHorizonRealtimeManager::Unsubscribe(const FString& Topic)
{
    if (Topics.Remove(Topic) == 0) return;
    Sent.Remove(Topic);
    if (Pending == Topic) bPendingCanceled = true;
    if (bIsReady) Command(TEXT("unsubscribe"), Topic, ++RequestId);
    if (Topics.IsEmpty()) DisconnectTransport();
}
void UHorizonRealtimeManager::SetPollAfterSeconds(int32 Seconds) { PollAfterSeconds = FMath::Max(1, Seconds); PollAt = FPlatformTime::Seconds() + PollAfterSeconds; }
void UHorizonRealtimeManager::Close() { Topics.Empty(); Ticket.Empty(); DisconnectTransport(); }
bool UHorizonRealtimeManager::CanSendOverSocket()
{
    SyncIdentity();
    return bIsReady && bSendOverSocket && Socket.IsValid() && Socket->IsConnected() && Requests.Num() < MaxSendsInFlight;
}
bool UHorizonRealtimeManager::IsOutcomeUnknown(const FHorizonNetworkResponse& Response) { return Response.ServerErrorCode == HorizonRealtime::OutcomeUnknown; }
bool UHorizonRealtimeManager::GetSnapshot(const FString& Topic, FString& OutJson) const
{
    if (SnapshotTopic.IsEmpty() || SnapshotTopic != Topic) return false;
    OutJson = SnapshotJson; return true;
}
bool UHorizonRealtimeManager::SendRequest(const TSharedRef<FJsonObject>& Frame, FOnHttpResponse Complete)
{
    if (!CanSendOverSocket() || PendingSends >= 64) return false;
    const int32 Id = ++RequestId; Frame->SetNumberField(TEXT("requestId"), Id);
    FString Json; if (!HorizonLosslessJson::Object(Frame, Json)) return false;
    Requests.Add(Id, { MoveTemp(Complete), FPlatformTime::Seconds() + SendTimeoutSeconds });
    PendingSends++; Socket->Send(Json); return true;
}
void UHorizonRealtimeManager::ExpireRequests(double Now)
{
    TArray<int32> Expired;
    for (const auto& Row : Requests) if (Now >= Row.Value.Deadline) Expired.Add(Row.Key);
    for (const int32 Id : Expired) { FPendingRequest Row; if (Requests.RemoveAndCopyValue(Id, Row)) Row.Complete.ExecuteIfBound(HorizonRealtime::Unknown()); }
}
FHorizonNetworkResponse UHorizonRealtimeManager::ResultResponse(const FString& Text)
{
    // Same fields the HTTP path fills, so feature callbacks map both transports identically.
    FHorizonNetworkResponse R; const TSharedPtr<FJsonObject> Frame = HorizonRealtime::Lossless(Text);
    int32 Status = 0; if (Frame) Frame->TryGetNumberField(TEXT("status"), Status);
    if (Status < 100 || Status > 599) { R.ServerErrorCode = TEXT("INVALID_RESPONSE"); R.ErrorMessage = TEXT("Malformed realtime result"); return R; }
    R.StatusCode = Status; R.ErrorCode = FHorizonNetworkResponse::StatusToErrorCode(Status); R.bSuccess = Status >= 200 && Status < 300;
    if (R.bSuccess)
    {
        const TSharedPtr<FJsonObject>* Body = nullptr;
        if (Frame->TryGetObjectField(TEXT("result"), Body) && Body && Body->IsValid() && HorizonLosslessJson::Object(Body->ToSharedRef(), R.RawJsonBody))
        { R.JsonData = *Body; return R; }
        if (Status == 204) return R;
        R.bSuccess = false; R.ServerErrorCode = TEXT("INVALID_RESPONSE"); R.ErrorMessage = TEXT("Malformed realtime result"); return R;
    }
    FString Code; Frame->TryGetStringField(TEXT("code"), Code); R.ServerErrorCode = Code;
    double RetryAfter = 0; Frame->TryGetNumberField(TEXT("retryAfter"), RetryAfter);
    if (FMath::IsFinite(RetryAfter) && RetryAfter > 0) R.Headers.Add(FString::Printf(TEXT("Retry-After: %.0f"), FMath::CeilToDouble(RetryAfter)));
    if (Frame->TryGetStringField(TEXT("message"), R.ErrorMessage)) return R;
    R.ErrorMessage = Status == 429 ? (RetryAfter > 0 ? FString::Printf(TEXT("Rate limit exceeded (HTTP 429). Try again in %.0f seconds."), FMath::CeilToDouble(RetryAfter))
        : FString(TEXT("Rate limit exceeded (HTTP 429). Try again later."))) : FString::Printf(TEXT("HTTP %d"), Status);
    return R;
}
double UHorizonRealtimeManager::ReconnectDelay(int32 Count, double Fraction)
{
    return FMath::Clamp(Fraction, 0.0, 1.0) * FMath::Min(30.0, FMath::Pow(2.0, FMath::Min(Count, 5)));
}
void UHorizonRealtimeManager::Tick(float DeltaTime)
{
    if (!HttpClient || !AuthManager) return;
    SyncIdentity();
    const double Now = FPlatformTime::Seconds();
    ExpireRequests(Now);
    if (Topics.IsEmpty() || Token.IsEmpty()) return;
    if (bIsReady && Now - LastFrameAt > 90) { Failed(TEXT("HEARTBEAT_TIMEOUT")); return; }
    if (bIsReady)
    {
        if (Pending.IsEmpty()) for (const FString& Topic : Topics) if (!Sent.Contains(Topic))
        { Pending = Topic; bPendingCanceled = false; PendingRequestId = ++RequestId;
            Command(TEXT("subscribe"), Topic, PendingRequestId); break; }
        if (Now >= PingAt && Pending.IsEmpty()) { Command(TEXT("ping")); PingAt = Now + 30; }
    }
    else
    {
        if (bConnecting && Now > ConnectDeadline) Failed(TEXT("CONNECTION_TIMEOUT"));
        if (Now >= PollAt)
        {
            PollAt = Now + PollAfterSeconds;
            const TSet<FString> Copy = Topics;
            for (const FString& Topic : Copy)
            { if (SyncIdentity()) break; if (Topics.Contains(Topic) && CanPoll(Topic)) OnPollingRequired.Broadcast(Topic, PollAfterSeconds); }
        }
        if (!bConnecting && !bUnsupported && Now >= RetryAt) Open();
    }
}
void UHorizonRealtimeManager::Open()
{
    bConnecting = true; ConnectDeadline = FPlatformTime::Seconds() + 15;
    // A held ticket resumes at the gateway without the backend session call (gateway rolling updates).
    if (!Ticket.IsEmpty() && FPlatformTime::Seconds() - TicketAt < HorizonRealtime::TicketMaxAgeSeconds) { OpenSocket(); return; }
    const int32 CapturedGeneration = Generation;
    TWeakObjectPtr<UHorizonRealtimeManager> Weak(this);
    HttpClient->PostJson(MakeShared<FJsonObject>(), TEXT("/api/v1/app/realtime/session"), true,
        FOnHttpResponse::CreateLambda([Weak, CapturedGeneration](const FHorizonNetworkResponse& Result)
    {
        auto* Self = Weak.Get();
        if (!Self || Self->SyncIdentity() || Self->Generation != CapturedGeneration) return;
        if (!Result.bSuccess)
        {
            FString Code;
            if (Result.JsonData.IsValid()) Result.JsonData->TryGetStringField(TEXT("code"), Code);
            Self->bUnsupported = Code == TEXT("NOT_SUPPORTED") || Result.StatusCode == 501;
            Self->Failed(Self->bUnsupported ? TEXT("NOT_SUPPORTED") : TEXT("GATEWAY_UNAVAILABLE")); return;
        }
        double Poll = 120;
        if (Result.JsonData.IsValid()) Result.JsonData->TryGetNumberField(TEXT("pollAfterSeconds"), Poll);
        Self->PollAfterSeconds = FMath::Max(1, static_cast<int32>(Poll));
        Self->OpenSocket();
    }));
}
void UHorizonRealtimeManager::OpenSocket()
{
    FString Url = HttpClient->GetActiveHost(); Url.RemoveFromEnd(TEXT("/"));
    Url.ReplaceInline(TEXT("https://"), TEXT("wss://")); Url.ReplaceInline(TEXT("http://"), TEXT("ws://"));
    Url += TEXT("/api/v1/app/realtime/socket");
    if (!GatewayUrl.IsEmpty()) Url = GatewayUrl;
    if ((!Url.StartsWith(TEXT("ws://")) && !Url.StartsWith(TEXT("wss://"))) || Url.Contains(TEXT("?")) || Url.Contains(TEXT("@")))
    { Failed(TEXT("INVALID_GATEWAY_URL")); return; }
    Attach(FWebSocketsModule::Get().CreateWebSocket(Url));
    Socket->Connect();
}
void UHorizonRealtimeManager::Attach(const TSharedRef<IWebSocket>& NewSocket)
{
    Socket = NewSocket;
    Socket->SetTextMessageMemoryLimit(2 * HorizonRealtime::MaxFrameChars);
    Socket->OnConnected().AddUObject(this, &UHorizonRealtimeManager::Connected);
    Socket->OnMessage().AddUObject(this, &UHorizonRealtimeManager::Message);
    Socket->OnMessageSent().AddUObject(this, &UHorizonRealtimeManager::SentMessage);
    Socket->OnConnectionError().AddUObject(this, &UHorizonRealtimeManager::Failed);
    Socket->OnClosed().AddUObject(this, &UHorizonRealtimeManager::Closed);
}
void UHorizonRealtimeManager::Connected()
{
    auto Frame = MakeShared<FJsonObject>(); Frame->SetStringField(TEXT("type"), TEXT("authenticate"));
    Frame->SetNumberField(TEXT("version"), 1); Frame->SetStringField(TEXT("apiKey"), HttpClient->GetApiKey());
    bTicketAuth = !Ticket.IsEmpty() && FPlatformTime::Seconds() - TicketAt < HorizonRealtime::TicketMaxAgeSeconds;
    if (bTicketAuth) Frame->SetStringField(TEXT("ticket"), Ticket); else Frame->SetStringField(TEXT("sessionToken"), Token);
    Frame->SetStringField(TEXT("clientId"), ClientId);
    Send(Frame);
}
void UHorizonRealtimeManager::Message(const FString& Text)
{
    if (!HttpClient || !AuthManager || SyncIdentity()) return;
    LastFrameAt = FPlatformTime::Seconds();
    if (Text.Len() > HorizonRealtime::MaxFrameChars) { Failed(TEXT("INVALID_FRAME")); return; }
    TSharedPtr<FJsonObject> Frame;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Frame) || !Frame.IsValid())
    { Failed(TEXT("INVALID_FRAME")); return; }
    double Version = 0; Frame->TryGetNumberField(TEXT("version"), Version);
    if (Version != 1) { Failed(TEXT("INVALID_FRAME")); return; }
    FString Type, Topic;
    Frame->TryGetStringField(TEXT("type"), Type); Frame->TryGetStringField(TEXT("topic"), Topic);
    int32 CorrelationId = 0; Frame->TryGetNumberField(TEXT("requestId"), CorrelationId);
    if (Type == TEXT("ready"))
    {
        bIsReady = true; bConnecting = false; Attempt = 0; PingAt = FPlatformTime::Seconds() + 30;
        double Poll = 120; Frame->TryGetNumberField(TEXT("pollAfterSeconds"), Poll);
        PollAfterSeconds = FMath::Max(1, static_cast<int32>(Poll));
        FString NewTicket; if (Frame->TryGetStringField(TEXT("ticket"), NewTicket) && !NewTicket.IsEmpty()) { Ticket = NewTicket; TicketAt = LastFrameAt; }
        bSendOverSocket = false; Frame->TryGetBoolField(TEXT("sendOverSocket"), bSendOverSocket);
    }
    else if (Type == TEXT("ticket"))
    { FString NewTicket; if (Frame->TryGetStringField(TEXT("ticket"), NewTicket) && !NewTicket.IsEmpty()) { Ticket = NewTicket; TicketAt = LastFrameAt; } }
    else if (Type == TEXT("result"))
    { FPendingRequest Row; if (CorrelationId > 0 && Requests.RemoveAndCopyValue(CorrelationId, Row)) Row.Complete.ExecuteIfBound(ResultResponse(Text)); }
    else if (Type == TEXT("unsubscribed") && CorrelationId == PendingRequestId && Topic == Pending) { Sent.Remove(Topic); Pending.Empty(); }
    else if (Type == TEXT("subscribed") && CorrelationId == PendingRequestId && Topic == Pending)
    {
        const bool Accepted=!bPendingCanceled&&Topics.Contains(Topic); if(Accepted)Sent.Add(Topic); Pending.Empty();
        if (!Accepted) return;
        // A snapshot replaces the HTTP reload: features read it through GetSnapshot while OnSubscribed runs.
        const TSharedPtr<FJsonObject>* Snapshot = nullptr; const TSharedPtr<FJsonObject> Lossless = Frame->HasField(TEXT("snapshot")) ? HorizonRealtime::Lossless(Text) : nullptr;
        if (Lossless && Lossless->TryGetObjectField(TEXT("snapshot"), Snapshot) && Snapshot && Snapshot->IsValid() && HorizonLosslessJson::Object(Snapshot->ToSharedRef(), SnapshotJson)) SnapshotTopic = Topic;
        OnSubscribed.Broadcast(Topic); SnapshotTopic.Empty(); SnapshotJson.Empty();
    }
    else if (Type == TEXT("event"))
    {
        if (Topics.Contains(Topic)) OnEvent.Broadcast(Text);
        FString EventId; Frame->TryGetStringField(TEXT("eventId"), EventId);
        auto Ack = MakeShared<FJsonObject>(); Ack->SetStringField(TEXT("type"), TEXT("ack"));
        Ack->SetStringField(TEXT("eventId"), EventId); Send(Ack);
    }
    else if (Type == TEXT("resync_required"))
    {
        FString Reason; Frame->TryGetStringField(TEXT("reason"), Reason);
        if (Reason == TEXT("PERMISSIONS_CHANGED")) Sent.Empty();
        if (!Topic.IsEmpty()) { if (Topics.Contains(Topic)) OnResyncRequired.Broadcast(Topic); }
        else { const TSet<FString> Copy = Topics; for (const FString& Subscribed : Copy)
        { if (SyncIdentity()) break; if (Topics.Contains(Subscribed)) OnResyncRequired.Broadcast(Subscribed); } }
        FString EventId; Frame->TryGetStringField(TEXT("eventId"), EventId);
        auto Ack = MakeShared<FJsonObject>(); Ack->SetStringField(TEXT("type"), TEXT("ack"));
        Ack->SetStringField(TEXT("eventId"), EventId); Send(Ack);
    }
    else if (Type == TEXT("error") && !bIsReady && bTicketAuth && Frame->HasTypedField<EJson::String>(TEXT("code")) && Frame->GetStringField(TEXT("code")) == TEXT("SESSION_REQUIRED"))
    {
        // The ticket was refused: drop it and reconnect at once with the session token.
        Ticket.Empty(); DisconnectTransport(); LastErrorCode = TEXT("SESSION_REQUIRED"); RetryAt = FPlatformTime::Seconds();
    }
    else if (Type == TEXT("error") && Frame->HasTypedField<EJson::String>(TEXT("code")) && Frame->GetStringField(TEXT("code")) == TEXT("CONNECTION_LIMIT_REACHED"))
    {
        // No socket for this session for pollAfterSeconds: features stay on HTTP (polling as v1), then retry.
        DisconnectTransport(); LastErrorCode = TEXT("CONNECTION_LIMIT_REACHED"); RetryAt = FPlatformTime::Seconds() + PollAfterSeconds;
        OnConnectionError.Broadcast(LastErrorCode);
    }
    else if (Type == TEXT("error")) { Frame->TryGetStringField(TEXT("code"), LastErrorCode);
        if (LastErrorCode == TEXT("SUBSCRIPTION_FORBIDDEN") && CorrelationId == PendingRequestId) { Topics.Remove(Topic); Sent.Remove(Topic); Pending.Empty(); }
        if(!Topic.IsEmpty())OnSubscriptionError.Broadcast(Topic,LastErrorCode);
        OnConnectionError.Broadcast(LastErrorCode); }
}
void UHorizonRealtimeManager::Closed(int32 Status, const FString& Reason, bool bClean)
{
    if (Status != 1012) { Failed(TEXT("CONNECTION_CLOSED")); return; }
    // Gateway update (SERVICE_RESTART): short random reconnect with the ticket, attempt counter unchanged.
    DisconnectTransport(); LastErrorCode = TEXT("SERVICE_RESTART");
    RetryAt = FPlatformTime::Seconds() + FMath::FRandRange(0.0, 5.0);
    OnConnectionError.Broadcast(LastErrorCode);
}
void UHorizonRealtimeManager::Failed(const FString& Code)
{
    DisconnectTransport(); LastErrorCode = Code;
    RetryAt = FPlatformTime::Seconds() + ReconnectDelay(Attempt++, FMath::FRand());
    OnConnectionError.Broadcast(Code);
}
void UHorizonRealtimeManager::Send(const TSharedRef<FJsonObject>& Frame)
{
    if (!Socket.IsValid() || !Socket->IsConnected()) return;
    if (PendingSends >= 64) { Failed(TEXT("SLOW_CONSUMER")); return; }
    PendingSends++;
    FString Json; FJsonSerializer::Serialize(Frame, TJsonWriterFactory<>::Create(&Json)); Socket->Send(Json);
}
void UHorizonRealtimeManager::Command(const FString& Type, const FString& Topic, int32 CorrelationId)
{
    auto Frame = MakeShared<FJsonObject>(); Frame->SetStringField(TEXT("type"), Type);
    if (!Topic.IsEmpty()) Frame->SetStringField(TEXT("topic"), Topic);
    if (CorrelationId > 0) Frame->SetNumberField(TEXT("requestId"), CorrelationId);
    if (Type == TEXT("subscribe") && Topic.StartsWith(TEXT("chat:")) && BuildSync.IsBound())
    { const TSharedPtr<FJsonObject> Sync = BuildSync.Execute(Topic); if (Sync) Frame->SetObjectField(TEXT("sync"), Sync); }
    Send(Frame);
}
void UHorizonRealtimeManager::DisconnectTransport()
{
    Generation++; bIsReady = false; bConnecting = false; bSendOverSocket = false; Sent.Empty(); Pending.Empty();
    PendingSends = 0; SnapshotTopic.Empty(); SnapshotJson.Empty();
    TMap<int32, FPendingRequest> Unresolved = MoveTemp(Requests); Requests.Empty();
    if (Socket.IsValid())
    {
        Socket->OnConnected().RemoveAll(this); Socket->OnMessage().RemoveAll(this);
        Socket->OnMessageSent().RemoveAll(this);
        Socket->OnConnectionError().RemoveAll(this); Socket->OnClosed().RemoveAll(this);
        Socket->Close(); Socket.Reset();
    }
    // Sends without a result: the feature resolves them over HTTP with the same operation.
    for (const auto& Row : Unresolved) Row.Value.Complete.ExecuteIfBound(HorizonRealtime::Unknown());
}
bool UHorizonRealtimeManager::SyncIdentity()
{
    if (!HttpClient || !AuthManager) return false;
    bool bChanged = false;
    const uint64 CurrentContext=HttpClient->GetRequestContextEpoch();
    if(CurrentContext!=HttpContext){Ticket.Empty();DisconnectTransport();HttpContext=CurrentContext;bUnsupported=false;Attempt=0;RetryAt=0;bChanged=true;}
    const FString CurrentUser = AuthManager->IsSignedIn() ? AuthManager->GetCurrentUser().UserId : TEXT("");
    if (CurrentUser != UserId) { if (!UserId.IsEmpty()) Close(); UserId = CurrentUser; bChanged = true; }
    if (Token != HttpClient->GetSessionToken())
    { Ticket.Empty(); DisconnectTransport(); Token = HttpClient->GetSessionToken(); bUnsupported = false; Attempt = 0; bChanged = true; }
    return bChanged;
}
void UHorizonRealtimeManager::BeginDestroy() { Close(); HttpClient = nullptr; Super::BeginDestroy(); }
