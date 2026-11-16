// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Tickable.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonRealtimeManager.generated.h"

class UHorizonHttpClient;
class UHorizonAuthManager;
class IWebSocket;
class FJsonObject;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeEvent, const FString&, EventJson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeTopic, const FString&, Topic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRealtimePolling, const FString&, Topic, int32, PollAfterSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRealtimeTopicError,const FString&,Topic,const FString&,Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeError, const FString&, Code);
/** Catch up state for a resubscribed topic (socket v2), null to omit `sync`. */
DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<FJsonObject>, FOnRealtimeBuildSync, const FString&);

/** Optional realtime channel. With socket v2 chat sends and match lobby or turn actions travel as socket frames
 *  while the gateway allows it; otherwise every call stays HTTP. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonRealtimeManager : public UObject, public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Client, UHorizonAuthManager* Auth);
    /** True only after this connection has received the matching authorized topic ACK. */
    UFUNCTION(BlueprintPure,Category="horizOn|Realtime") bool IsSubscribed(const FString& Topic);
    uint64 GetConnectionGeneration();
    UFUNCTION(BlueprintCallable, Category="horizOn|Realtime") bool Subscribe(const FString& Topic);
    UFUNCTION(BlueprintCallable, Category="horizOn|Realtime") void Unsubscribe(const FString& Topic);
    UFUNCTION(BlueprintCallable, Category="horizOn|Realtime") void Close();
    UFUNCTION(BlueprintCallable, Category="horizOn|Realtime") void SetPollAfterSeconds(int32 Seconds);
    /** True while chat sends and match actions may travel over the socket (ready, sendOverSocket, fewer than 4 in flight). */
    UFUNCTION(BlueprintPure, Category="horizOn|Realtime") bool CanSendOverSocket();
    /** Sends a chat.send or match.action frame and completes with the HTTP-equivalent response.
     *  Returns false without sending when the socket cannot carry it: the caller then uses HTTP. */
    bool SendRequest(const TSharedRef<FJsonObject>& Frame, FOnHttpResponse Complete);
    /** Completion of a socket send whose outcome is unknown (closed or 10 s timeout): resolve over HTTP idempotently. */
    static bool IsOutcomeUnknown(const FHorizonNetworkResponse& Response);
    /** Snapshot of the topic whose `subscribed` frame is being broadcast through OnSubscribed, as raw JSON. */
    bool GetSnapshot(const FString& Topic, FString& OutJson) const;
    FOnRealtimeBuildSync BuildSync;
    static constexpr int32 MaxSendsInFlight = 4;
    static constexpr double SendTimeoutSeconds = 10;
    /** Optional full ws/wss URL for a separately exposed Dev gateway. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="horizOn|Realtime") FString GatewayUrl;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Realtime") bool bIsReady = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Realtime") int32 PollAfterSeconds = 120;
    /** Gateway allows chat sends and match actions over this socket (socket v2 `ready.sendOverSocket`). */
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Realtime") bool bSendOverSocket = false;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Realtime") FString LastErrorCode;
    UPROPERTY(BlueprintAssignable, Category="horizOn|Realtime") FOnRealtimeEvent OnEvent;
    UPROPERTY(BlueprintAssignable, Category="horizOn|Realtime") FOnRealtimeTopic OnSubscribed;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Realtime") FOnRealtimeTopicError OnSubscriptionError;
    UPROPERTY(BlueprintAssignable, Category="horizOn|Realtime") FOnRealtimeTopic OnResyncRequired;
    UPROPERTY(BlueprintAssignable, Category="horizOn|Realtime") FOnRealtimePolling OnPollingRequired;
    UPROPERTY(BlueprintAssignable, Category="horizOn|Realtime") FOnRealtimeError OnConnectionError;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return !IsTemplate() && HttpClient != nullptr; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UHorizonRealtimeManager, STATGROUP_Tickables); }
    virtual void BeginDestroy() override;
    static double ReconnectDelay(int32 Attempt, double Fraction);
    static bool CanPoll(const FString& Topic) { return !Topic.StartsWith(TEXT("chat:")); }
private:
#if WITH_DEV_AUTOMATION_TESTS
    friend struct FRealtimeTransportTestState;
    friend struct FRealtimeSocketV2Test;
#endif
    struct FPendingRequest { FOnHttpResponse Complete; double Deadline = 0; };
    TMap<int32, FPendingRequest> Requests;
    /** Resume ticket: memory only, never persisted or logged. */
    FString Ticket, SnapshotTopic, SnapshotJson;
    double TicketAt = 0;
    bool bTicketAuth = false;
    UPROPERTY() UHorizonHttpClient* HttpClient = nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager = nullptr;
    TSharedPtr<IWebSocket> Socket;
    TSet<FString> Topics, Sent;
    FString Pending, ClientId, Token, UserId;
    int32 Generation = 0, Attempt = 0, PendingSends = 0;
    int32 RequestId = 0, PendingRequestId = 0;
    bool bPendingCanceled = false;
    uint64 HttpContext = 0;
    double RetryAt = 0, PollAt = 0, PingAt = 0, ConnectDeadline = 0, LastFrameAt = 0;
    bool bConnecting = false, bUnsupported = false;
    void Open();
    void OpenSocket();
    void Attach(const TSharedRef<IWebSocket>& NewSocket);
    void ExpireRequests(double Now);
    static FHorizonNetworkResponse ResultResponse(const FString& Text);
    bool SyncIdentity();
    void DisconnectTransport();
    void Connected();
    void Message(const FString& Text);
    void Closed(int32 Status, const FString& Reason, bool bClean);
    void Failed(const FString& Code);
    void Send(const TSharedRef<FJsonObject>& Frame);
    void SentMessage(const FString& Text) { PendingSends = FMath::Max(0, PendingSends - 1); }
    void Command(const FString& Type, const FString& Topic = TEXT(""), int32 CorrelationId = 0);
};
