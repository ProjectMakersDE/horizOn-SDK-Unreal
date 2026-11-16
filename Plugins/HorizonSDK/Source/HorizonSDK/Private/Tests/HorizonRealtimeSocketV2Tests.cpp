// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Managers/HorizonRealtimeManager.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonChatManager.h"
#include "Models/HorizonChatWire.h"
#include "Http/HorizonHttpClient.h"
#include "IWebSocket.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/OutputDevice.h"

// Socket v2 (TASK-1139) against a fake socket: no network, no credentials, artificial ticket values only.
namespace HorizonRealtimeV2Test
{
class FFakeSocket : public IWebSocket
{
public:
    TArray<FString> Sent;
    bool bOpen = true;
    virtual void Connect() override {}
    virtual void Close(int32, const FString&) override { bOpen = false; }
    virtual bool IsConnected() override { return bOpen; }
    virtual void Send(const FString& Data) override { Sent.Add(Data); }
    virtual void Send(const void*, SIZE_T, bool) override {}
    virtual void SetTextMessageMemoryLimit(uint64) override {}
    virtual FWebSocketConnectedEvent& OnConnected() override { return ConnectedEvent; }
    virtual FWebSocketConnectionErrorEvent& OnConnectionError() override { return ErrorEvent; }
    virtual FWebSocketClosedEvent& OnClosed() override { return ClosedEvent; }
    virtual FWebSocketMessageEvent& OnMessage() override { return MessageEvent; }
    virtual FWebSocketBinaryMessageEvent& OnBinaryMessage() override { return BinaryEvent; }
    virtual FWebSocketRawMessageEvent& OnRawMessage() override { return RawEvent; }
    virtual FWebSocketMessageSentEvent& OnMessageSent() override { return SentEvent; }
    TSharedPtr<FJsonObject> Last() const
    {
        TSharedPtr<FJsonObject> J; if (Sent.Num()) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Sent.Last()), J); return J;
    }
private:
    FWebSocketConnectedEvent ConnectedEvent; FWebSocketConnectionErrorEvent ErrorEvent; FWebSocketClosedEvent ClosedEvent;
    FWebSocketMessageEvent MessageEvent; FWebSocketBinaryMessageEvent BinaryEvent; FWebSocketRawMessageEvent RawEvent; FWebSocketMessageSentEvent SentEvent;
};
struct FCapture : FOutputDevice
{
    FString Text;
    virtual void Serialize(const TCHAR* V, ELogVerbosity::Type, const FName&) override { Text += V; }
};
const FString TicketA = TEXT("artificial-ticket-a.sig"), TicketB = TEXT("artificial-ticket-b.sig");
const FString Channel = TEXT("10000000-0000-4000-8000-000000001139");
FString Ready(bool bSend, const FString& Ticket = TicketA)
{ return FString::Printf(TEXT("{\"type\":\"ready\",\"version\":1,\"heartbeatSeconds\":30,\"pollAfterSeconds\":120,\"maxSubscriptions\":32,\"ticket\":\"%s\",\"sendOverSocket\":%s}"), *Ticket, bSend ? TEXT("true") : TEXT("false")); }
FString ChannelJson() { return TEXT(R"({"id":"10000000-0000-4000-8000-000000001139","epoch":"20000000-0000-4000-8000-000000001139","kind":"project","peerIds":[],"groupId":null,"revision":3,"expiresAt":"2099-01-01T00:00:00.000Z"})"); }
FString Message(int32 N) { return FString::Printf(TEXT(R"({"id":"30000000-0000-4000-8000-00000000000%d","channelId":"10000000-0000-4000-8000-000000001139","authorId":"00000000-0000-4000-8000-000000001139","text":"m%d","createdAt":"2026-11-16T00:00:0%d.000Z","expiresAt":"2026-11-23T00:00:00.000Z"})"), N, N, N); }
FString MessageId(int32 N) { return FString::Printf(TEXT("30000000-0000-4000-8000-00000000000%d"), N); }
FString Synced(const TArray<int32>& Visible, const TArray<int32>& Rows, const FString& Time)
{
    FString Ids, Messages;
    for (int32 N : Visible) Ids += (Ids.IsEmpty() ? TEXT("\"") : TEXT(",\"")) + MessageId(N) + TEXT("\"");
    for (int32 N : Rows) Messages += (Messages.IsEmpty() ? TEXT("") : TEXT(",")) + Message(N);
    return TEXT("{\"channel\":") + ChannelJson() + TEXT(",\"visibleMessageIds\":[") + Ids + TEXT("],\"messages\":[") + Messages
        + TEXT("],\"moderation\":{\"muteUntil\":null,\"banUntil\":null},\"serverTime\":\"") + Time + TEXT("\"}");
}
}

struct FRealtimeSocketV2Test
{
    FAutomationTestBase* T;
    UHorizonHttpClient* Http = nullptr;
    UHorizonAuthManager* Auth = nullptr;
    UHorizonRealtimeManager* R = nullptr;
    TSharedPtr<HorizonRealtimeV2Test::FFakeSocket> Fake;
    explicit FRealtimeSocketV2Test(FAutomationTestBase* InTest) : T(InTest)
    {
        Http = NewObject<UHorizonHttpClient>(); Http->AddToRoot(); Http->Initialize();
        Auth = NewObject<UHorizonAuthManager>(); Auth->AddToRoot(); Auth->Initialize(Http);
        R = NewObject<UHorizonRealtimeManager>(); R->AddToRoot(); R->Initialize(Http, Auth);
        R->SyncIdentity();
    }
    ~FRealtimeSocketV2Test() { R->Close(); R->RemoveFromRoot(); Auth->RemoveFromRoot(); Http->RemoveFromRoot(); }
    /** Attaches the fake transport as Open would, then authenticates. */
    void Connect() { Fake = MakeShared<HorizonRealtimeV2Test::FFakeSocket>(); R->Attach(Fake.ToSharedRef()); R->Connected(); }
    static TSharedRef<FJsonObject> ChatFrame()
    {
        auto F = MakeShared<FJsonObject>(); F->SetStringField(TEXT("type"), TEXT("chat.send"));
        F->SetStringField(TEXT("channelId"), HorizonRealtimeV2Test::Channel); F->SetStringField(TEXT("operationId"), TEXT("40000000-0000-4000-8000-000000001139"));
        F->SetStringField(TEXT("issuedAt"), TEXT("2026-11-16T00:00:00.000Z")); F->SetStringField(TEXT("text"), TEXT("hi")); return F;
    }
    bool Send(FHorizonNetworkResponse& Out, bool& bDone)
    { return R->SendRequest(ChatFrame(), FOnHttpResponse::CreateLambda([&Out, &bDone](const FHorizonNetworkResponse& X) { Out = X; bDone = true; })); }
    int32 LastRequestId() const { int32 Id = 0; if (auto J = Fake->Last()) J->TryGetNumberField(TEXT("requestId"), Id); return Id; }

    void Transport()
    {
        using namespace HorizonRealtimeV2Test;
        FCapture Log; GLog->AddOutputDevice(&Log);
        FHorizonNetworkResponse Out; bool bDone = false;

        // HTTP fallback while not ready, and while the gateway disallows socket sends.
        T->TestFalse(TEXT("Not ready: caller uses HTTP"), Send(Out, bDone));
        Connect();
        auto First = Fake->Last();
        T->TestTrue(TEXT("First authenticate uses the session token"), First && First->HasField(TEXT("sessionToken")) && !First->HasField(TEXT("ticket")));
        R->Message(Ready(false));
        T->TestTrue(TEXT("Ready"), R->bIsReady);
        T->TestFalse(TEXT("sendOverSocket false: caller uses HTTP"), Send(Out, bDone));

        // Send over the socket when ready, with the contract frame.
        R->DisconnectTransport(); Connect(); R->Message(Ready(true));
        T->TestTrue(TEXT("Ready allows socket sends"), R->CanSendOverSocket());
        T->TestTrue(TEXT("Chat send goes over the socket"), Send(Out, bDone));
        auto Frame = Fake->Last(); int32 Id = LastRequestId();
        T->TestTrue(TEXT("chat.send frame with requestId, no userId"), Frame && Frame->GetStringField(TEXT("type")) == TEXT("chat.send") && Id > 0 && !Frame->HasField(TEXT("userId")));

        // 200 result maps to the HTTP success response, numbers kept exact.
        R->Message(FString::Printf(TEXT("{\"type\":\"result\",\"version\":1,\"requestId\":%d,\"status\":200,\"result\":{\"receipt\":{\"revision\":9007199254740991}}}"), Id));
        T->TestTrue(TEXT("200 result completes as success"), bDone && Out.bSuccess && Out.StatusCode == 200 && Out.JsonData.IsValid());
        T->TestTrue(TEXT("200 result body is the exact HTTP body"), Out.RawJsonBody.Contains(TEXT("9007199254740991")));

        // Error result maps like HTTP: status, code, retryAfter.
        bDone = false; Send(Out, bDone); Id = LastRequestId();
        R->Message(FString::Printf(TEXT("{\"type\":\"result\",\"version\":1,\"requestId\":%d,\"status\":429,\"code\":\"CHAT_RATE_LIMITED\",\"retryAfter\":3}"), Id));
        T->TestTrue(TEXT("429 result is the HTTP rate limit error"), bDone && !Out.bSuccess && Out.StatusCode == 429 && Out.GetErrorCodeString() == TEXT("CHAT_RATE_LIMITED") && Out.ErrorCode == EHorizonErrorCode::RateLimited);
        T->TestEqual(TEXT("retryAfter as Retry-After"), Out.GetHeader(TEXT("Retry-After")), FString(TEXT("3")));

        // Timeout and close resolve as unknown outcome.
        bDone = false; Send(Out, bDone); R->ExpireRequests(FPlatformTime::Seconds() + UHorizonRealtimeManager::SendTimeoutSeconds + 1);
        T->TestTrue(TEXT("10 s timeout is an unknown outcome"), bDone && UHorizonRealtimeManager::IsOutcomeUnknown(Out));
        bDone = false; Send(Out, bDone); R->Closed(1006, TEXT(""), false);
        T->TestTrue(TEXT("Close before the result is an unknown outcome"), bDone && UHorizonRealtimeManager::IsOutcomeUnknown(Out));

        // At most 4 in flight: the fifth goes over HTTP.
        Connect(); R->Message(Ready(true));
        for (int32 N = 0; N < UHorizonRealtimeManager::MaxSendsInFlight; ++N) R->SendRequest(ChatFrame(), FOnHttpResponse::CreateLambda([](const FHorizonNetworkResponse&) {}));
        T->TestFalse(TEXT("Fifth concurrent send falls back to HTTP"), Send(Out, bDone));

        // Ticket on reconnect, a ticket frame replaces the stored one.
        R->Message(FString::Printf(TEXT("{\"type\":\"ticket\",\"version\":1,\"ticket\":\"%s\"}"), *TicketB));
        R->DisconnectTransport(); Connect(); auto Resume = Fake->Last();
        T->TestTrue(TEXT("Reconnect authenticates with the newest ticket only"), Resume && Resume->HasField(TEXT("ticket")) && Resume->GetStringField(TEXT("ticket")) == TicketB && !Resume->HasField(TEXT("sessionToken")));
        R->TicketAt = FPlatformTime::Seconds() - 10 * 60; R->DisconnectTransport(); Connect(); Resume = Fake->Last();
        T->TestTrue(TEXT("Ticket older than 9 minutes is not used"), Resume && Resume->HasField(TEXT("sessionToken")) && !Resume->HasField(TEXT("ticket")));

        // SESSION_REQUIRED after a ticket authenticate: drop it, reconnect at once with the session token.
        R->Ticket = TicketA; R->TicketAt = FPlatformTime::Seconds(); R->DisconnectTransport(); Connect();
        const int32 Attempts = R->Attempt;
        R->Message(TEXT("{\"type\":\"error\",\"version\":1,\"code\":\"SESSION_REQUIRED\"}"));
        T->TestTrue(TEXT("Refused ticket dropped, immediate retry"), R->Ticket.IsEmpty() && !R->bIsReady && R->RetryAt <= FPlatformTime::Seconds() && R->Attempt == Attempts);
        Connect(); Resume = Fake->Last();
        T->TestTrue(TEXT("Fallback authenticates with the session token"), Resume && Resume->HasField(TEXT("sessionToken")) && !Resume->HasField(TEXT("ticket")));

        // 1012 short reconnect keeps the ticket and the attempt counter; other closes back off.
        R->Message(Ready(true)); const int32 Before = R->Attempt; const double Now = FPlatformTime::Seconds();
        R->Closed(1012, TEXT("SERVICE_RESTART"), true);
        T->TestTrue(TEXT("1012 reconnects within 5 s with the ticket"), R->RetryAt >= Now && R->RetryAt <= Now + 5.5 && R->Attempt == Before && R->Ticket == TicketA);
        Connect(); R->Message(Ready(true)); R->Closed(1006, TEXT(""), false);
        T->TestTrue(TEXT("Other closes count an attempt"), R->Attempt == Before + 1);

        // CONNECTION_LIMIT_REACHED: HTTP for pollAfterSeconds.
        R->Attempt = 0; Connect();
        R->Message(TEXT("{\"type\":\"error\",\"version\":1,\"code\":\"CONNECTION_LIMIT_REACHED\"}"));
        T->TestTrue(TEXT("Connection limit stays on HTTP for pollAfterSeconds"), !R->bIsReady && !R->CanSendOverSocket() && R->RetryAt >= FPlatformTime::Seconds() + R->PollAfterSeconds - 1);

        GLog->RemoveOutputDevice(&Log);
        T->TestFalse(TEXT("No ticket in logs"), Log.Text.Contains(TEXT("artificial-ticket")));
        T->TestFalse(TEXT("No api key or session token in logs"), Log.Text.Contains(TEXT("sessionToken")) || Log.Text.Contains(TEXT("apiKey")));
    }

    void Snapshot()
    {
        using namespace HorizonRealtimeV2Test;
        UHorizonChatManager* Chat = NewObject<UHorizonChatManager>(); Chat->AddToRoot(); Chat->Initialize(Http, Auth, R); Chat->History.Reset();
        const FString Topic = TEXT("chat:") + Channel, MatchTopic = TEXT("matches:") + Channel;
        // A feed known before the reconnect: messages 1 and 2.
        TSharedPtr<FJsonObject> J; FHorizonChatSynced Known;
        T->TestTrue(TEXT("Known feed parses"), HorizonChatWire::Parse(Synced({1, 2}, {1, 2}, TEXT("2026-11-16T00:00:05.000Z")), J) && FHorizonChatSynced::TryFromJson(J, Known));
        auto& S = Chat->Channels.Add(Channel); S.Serial = 1;
        S.Known.bPresent = true; S.Known.Channel = Known.Channel; S.Known.Messages = Known.Messages; S.Known.ServerTime = Known.ServerTime;

        Connect(); R->Message(Ready(true)); R->Topics.Add(Topic); R->Topics.Add(MatchTopic);
        // Resubscribe carries sync for chat, none for matches.
        R->Pending = Topic; R->PendingRequestId = ++R->RequestId; R->Command(TEXT("subscribe"), Topic, R->PendingRequestId);
        auto Sub = Fake->Last(); const TSharedPtr<FJsonObject>* Sync = nullptr;
        T->TestTrue(TEXT("Chat resubscribe sends epoch and known ids"), Sub && Sub->TryGetObjectField(TEXT("sync"), Sync) && Sync && (*Sync)->GetStringField(TEXT("epoch")) == Known.Channel.Epoch && (*Sync)->GetArrayField(TEXT("knownMessageIds")).Num() == 2);
        R->Command(TEXT("subscribe"), MatchTopic, R->RequestId + 1);
        T->TestFalse(TEXT("Match subscribe omits sync"), Fake->Last()->HasField(TEXT("sync")));

        // A snapshot with only the missed message 3 is applied without an HTTP sync.
        R->Message(FString::Printf(TEXT("{\"type\":\"subscribed\",\"version\":1,\"topic\":\"%s\",\"requestId\":%d,\"snapshot\":%s}"), *Topic, R->PendingRequestId, *Synced({1, 2, 3}, {3}, TEXT("2026-11-16T00:00:09.000Z"))));
        const auto* After = Chat->Channels.Find(Channel);
        T->TestTrue(TEXT("Snapshot applied as the sync answer"), After && After->Visible && After->Complete && After->Feed.Messages.Num() == 3);
        T->TestTrue(TEXT("No HTTP reload after a snapshot"), After && !After->Busy);
        T->TestTrue(TEXT("Snapshot buffer cleared after the broadcast"), R->SnapshotTopic.IsEmpty() && R->SnapshotJson.IsEmpty());
        Chat->Initialize(nullptr, nullptr, nullptr); Chat->RemoveFromRoot();
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonRealtimeSocketV2Test, "Horizon.Realtime.SocketV2", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHorizonRealtimeSocketV2Test::RunTest(const FString&)
{
    { FRealtimeSocketV2Test State(this); State.Transport(); }
    { FRealtimeSocketV2Test State(this); State.Snapshot(); }
    return true;
}
#endif
