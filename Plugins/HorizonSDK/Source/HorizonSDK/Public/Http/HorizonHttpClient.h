// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Models/HorizonNetworkResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HorizonHttpClient.generated.h"

DECLARE_DELEGATE_OneParam(FOnHttpResponse, const FHorizonNetworkResponse& /*Response*/);

/**
 * HTTP client for the horizOn SDK.
 *
 * Wraps FHttpModule with automatic retry logic, HTTP 429 rate-limit handling,
 * and multi-host ping-based selection.  Use Initialize() once after construction,
 * then ConnectToServer() to discover the best backend host.
 */
UCLASS()
class HORIZONSDK_API UHorizonHttpClient : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize the client -- reads settings from UHorizonConfig. */
	void Initialize();

	/** Connect to the best available host via ping-based selection. */
	void ConnectToServer(FOnRequestComplete OnComplete);

	/** Disconnect and clear the active host. */
	void Disconnect();

	// -- State queries --
	bool IsConnected() const;
	FString GetActiveHost() const;
	EHorizonConnectionStatus GetConnectionStatus() const;

	/** Set the session token used for authenticated requests. */
	void SetSessionToken(const FString& Token);
	void ClearSessionToken();
	FString GetSessionToken() const;

	// -- HTTP convenience methods --
	void Get(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
	void PostJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
	void PostBinary(const FString& Endpoint, const TArray<uint8>& Data, bool bUseSessionToken, FOnHttpResponse OnComplete);
	void GetBinary(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);

private:
	FString ActiveHost;
	FString SessionToken;
	FString ApiKey;
	EHorizonConnectionStatus ConnectionStatus = EHorizonConnectionStatus::Disconnected;
	int32 MaxRetryAttempts = 3;
	float RetryDelaySeconds = 1.0f;
	float ConnectionTimeoutSeconds = 10.0f;

	/** Internal request dispatcher with built-in retry logic. */
	void SendRequest(
		const FString& Verb,
		const FString& Url,
		const FString& ContentType,
		const TArray<uint8>& Payload,
		bool bUseSessionToken,
		int32 RetryCount,
		FOnHttpResponse OnComplete
	);

	/** Build and apply standard headers to a request. */
	void ApplyHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, const FString& ContentType, bool bUseSessionToken) const;

	/** Parse an HTTP response into FHorizonNetworkResponse. */
	FHorizonNetworkResponse ParseResponse(FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully) const;

	/** Return true if the given status code warrants an automatic retry. */
	bool ShouldRetry(int32 StatusCode) const;

	/** Schedule a retry after a delay (game-thread safe). */
	void ScheduleRetry(
		const FString& Verb,
		const FString& Url,
		const FString& ContentType,
		const TArray<uint8>& Payload,
		bool bUseSessionToken,
		int32 RetryCount,
		float DelaySeconds,
		FOnHttpResponse OnComplete
	);

	/** Ping a single host, measure latency, call back when all pings are done. */
	void PingHost(const FString& Host, int32 AttemptIndex, TSharedRef<TArray<float>> Latencies, TFunction<void()> OnAllPingsDone);

	/** Select the best host from ping results and finalize connection. */
	void SelectBestHost(const TMap<FString, TArray<float>>& AllResults, FOnRequestComplete OnComplete);
};
