// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

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
	FString GetApiKey() const { return ApiKey; }
	/** For feature cache scopes that must distinguish leave-and-return identity changes. */
	uint64 GetRequestContextEpoch() const { return TransportContextEpoch + SessionContextEpoch; }
	EHorizonConnectionStatus GetConnectionStatus() const;

	/** Set the session token used for authenticated requests. */
	void SetSessionToken(const FString& Token);
	void ClearSessionToken();
	FString GetSessionToken() const;

	// -- HTTP convenience methods --
    /** Explicit one-attempt JSON write, preserving length-aware keys and numeric lexemes. No implicit retries. */
    void JsonOnce(const TSharedRef<FJsonObject>& Body,const FString& Endpoint,bool bPut,bool bUseSessionToken,FOnHttpResponse OnComplete);
    /** Feature-validated immutable original UTF-8 JSON, one authenticated POST and no normalization/retry. */
    void PostOriginalJsonOnce(const FString& OriginalJson,const FString& Endpoint,FOnHttpResponse Complete);
    /** Opaque read-only POST; lossless JSON and existing bounded read retries. */
    void PostJsonRead(const TSharedRef<FJsonObject>& Body,const FString& Endpoint,FOnHttpResponse Complete);
    /** Only POST/PUT/DELETE with original safe If-Match and exactly one authenticated attempt. */
    void RevisionJsonOnce(const TSharedPtr<FJsonObject>& Body,const FString& Endpoint,const FString& Verb,int64 OriginalRevision,FOnHttpResponse Complete);
	void Get(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
	/** ExtraHeaders are sent on every attempt, for example If-Match for a conditional cloud save. */
	void PostJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete,
		const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>());
	/** POST a JSON body and negotiate a binary response. */
	void PostJsonForBinary(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
	void PutJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
	void PostBinary(const FString& Endpoint, const TArray<uint8>& Data, bool bUseSessionToken, FOnHttpResponse OnComplete,
		const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>());
	void Delete(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
	void GetBinary(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);

private:
	FString ActiveHost;
	FString SessionToken;
	FString ApiKey;
	/** Monotone identity lifecycle, so A -> B -> A never revives an old request. */
	uint64 TransportContextEpoch = 0;
	uint64 SessionContextEpoch = 0;
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
		FOnHttpResponse OnComplete,
		const FString& AcceptContentType = TEXT(""),
		const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>(),
        bool bAllowRetry = true
	);

	/** Build and apply standard headers to a request. */
	void ApplyHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, const FString& ContentType, bool bUseSessionToken, const FString& AcceptContentType = TEXT(""),
		const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>()) const;

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
		FOnHttpResponse OnComplete,
		const FString& AcceptContentType = TEXT(""),
		const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>()
	);

	/** Ping a single host, measure latency, call back when all pings are done. */
	void PingHost(const FString& Host, int32 AttemptIndex, TSharedRef<TArray<float>> Latencies, TFunction<void()> OnAllPingsDone);

	/** Select the best host from ping results and finalize connection. */
	void SelectBestHost(const TMap<FString, TArray<float>>& AllResults, FOnRequestComplete OnComplete);
};
