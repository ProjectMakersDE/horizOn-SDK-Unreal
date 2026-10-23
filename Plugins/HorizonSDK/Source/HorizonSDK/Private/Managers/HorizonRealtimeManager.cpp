// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonRealtimeManager.h"
#include "Managers/HorizonAuthManager.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonConfig.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Internationalization/Regex.h"

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
void UHorizonRealtimeManager::Close() { Topics.Empty(); DisconnectTransport(); }
double UHorizonRealtimeManager::ReconnectDelay(int32 Count, double Fraction)
{
    return FMath::Clamp(Fraction, 0.0, 1.0) * FMath::Min(30.0, FMath::Pow(2.0, FMath::Min(Count, 5)));
}
void UHorizonRealtimeManager::Tick(float DeltaTime)
{
    if (!HttpClient || !AuthManager) return;
    SyncIdentity();
    if (Topics.IsEmpty() || Token.IsEmpty()) return;
    const double Now = FPlatformTime::Seconds();
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
        FString Url = Self->HttpClient->GetActiveHost(); Url.RemoveFromEnd(TEXT("/"));
        Url.ReplaceInline(TEXT("https://"), TEXT("wss://")); Url.ReplaceInline(TEXT("http://"), TEXT("ws://"));
        Url += TEXT("/api/v1/app/realtime/socket");
        if (!Self->GatewayUrl.IsEmpty()) Url = Self->GatewayUrl;
        if ((!Url.StartsWith(TEXT("ws://")) && !Url.StartsWith(TEXT("wss://"))) || Url.Contains(TEXT("?")) || Url.Contains(TEXT("@")))
        { Self->Failed(TEXT("INVALID_GATEWAY_URL")); return; }
        Self->Socket = FWebSocketsModule::Get().CreateWebSocket(Url);
        Self->Socket->SetTextMessageMemoryLimit(8192);
        Self->Socket->OnConnected().AddUObject(Self, &UHorizonRealtimeManager::Connected);
        Self->Socket->OnMessage().AddUObject(Self, &UHorizonRealtimeManager::Message);
        Self->Socket->OnMessageSent().AddUObject(Self, &UHorizonRealtimeManager::SentMessage);
        Self->Socket->OnConnectionError().AddUObject(Self, &UHorizonRealtimeManager::Failed);
        Self->Socket->OnClosed().AddUObject(Self, &UHorizonRealtimeManager::Closed);
        Self->Socket->Connect();
    }));
}
void UHorizonRealtimeManager::Connected()
{
    auto Frame = MakeShared<FJsonObject>(); Frame->SetStringField(TEXT("type"), TEXT("authenticate"));
    Frame->SetNumberField(TEXT("version"), 1); Frame->SetStringField(TEXT("apiKey"), HttpClient->GetApiKey());
    Frame->SetStringField(TEXT("sessionToken"), Token); Frame->SetStringField(TEXT("clientId"), ClientId);
    Send(Frame);
}
void UHorizonRealtimeManager::Message(const FString& Text)
{
    if (!HttpClient || !AuthManager || SyncIdentity()) return;
    LastFrameAt = FPlatformTime::Seconds();
    if (Text.Len() > 8192) { Failed(TEXT("INVALID_FRAME")); return; }
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
    }
    else if (Type == TEXT("unsubscribed") && CorrelationId == PendingRequestId && Topic == Pending) { Sent.Remove(Topic); Pending.Empty(); }
    else if (Type == TEXT("subscribed") && CorrelationId == PendingRequestId && Topic == Pending)
    { const bool Accepted=!bPendingCanceled&&Topics.Contains(Topic); if(Accepted)Sent.Add(Topic); Pending.Empty(); if(Accepted)OnSubscribed.Broadcast(Topic); }
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
    else if (Type == TEXT("error")) { Frame->TryGetStringField(TEXT("code"), LastErrorCode);
        if (LastErrorCode == TEXT("SUBSCRIPTION_FORBIDDEN") && CorrelationId == PendingRequestId) { Topics.Remove(Topic); Sent.Remove(Topic); Pending.Empty(); }
        if(!Topic.IsEmpty())OnSubscriptionError.Broadcast(Topic,LastErrorCode);
        OnConnectionError.Broadcast(LastErrorCode); }
}
void UHorizonRealtimeManager::Closed(int32 Status, const FString& Reason, bool bClean) { Failed(TEXT("CONNECTION_CLOSED")); }
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
    Send(Frame);
}
void UHorizonRealtimeManager::DisconnectTransport()
{
    Generation++; bIsReady = false; bConnecting = false; Sent.Empty(); Pending.Empty();
    PendingSends = 0;
    if (Socket.IsValid())
    {
        Socket->OnConnected().RemoveAll(this); Socket->OnMessage().RemoveAll(this);
        Socket->OnMessageSent().RemoveAll(this);
        Socket->OnConnectionError().RemoveAll(this); Socket->OnClosed().RemoveAll(this);
        Socket->Close(); Socket.Reset();
    }
}
bool UHorizonRealtimeManager::SyncIdentity()
{
    if (!HttpClient || !AuthManager) return false;
    bool bChanged = false;
    const uint64 CurrentContext=HttpClient->GetRequestContextEpoch();
    if(CurrentContext!=HttpContext){DisconnectTransport();HttpContext=CurrentContext;bUnsupported=false;Attempt=0;RetryAt=0;bChanged=true;}
    const FString CurrentUser = AuthManager->IsSignedIn() ? AuthManager->GetCurrentUser().UserId : TEXT("");
    if (CurrentUser != UserId) { if (!UserId.IsEmpty()) Close(); UserId = CurrentUser; bChanged = true; }
    if (Token != HttpClient->GetSessionToken())
    { DisconnectTransport(); Token = HttpClient->GetSessionToken(); bUnsupported = false; Attempt = 0; bChanged = true; }
    return bChanged;
}
void UHorizonRealtimeManager::BeginDestroy() { Close(); HttpClient = nullptr; Super::BeginDestroy(); }
