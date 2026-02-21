// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Http/HorizonHttpClient.h"
#include "HorizonConfig.h"
#include "HorizonSDKModule.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Misc/App.h"
#include "Containers/Ticker.h"
#include "TimerManager.h"
#include "Engine/World.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonHttpClient::Initialize()
{
	const UHorizonConfig* Config = UHorizonConfig::Get();
	if (!Config)
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("HorizonHttpClient::Initialize -- UHorizonConfig not found."));
		return;
	}

	ApiKey                   = Config->ApiKey;
	MaxRetryAttempts         = Config->MaxRetryAttempts;
	RetryDelaySeconds        = Config->RetryDelaySeconds;
	ConnectionTimeoutSeconds = static_cast<float>(Config->ConnectionTimeoutSeconds);

	ConnectionStatus = EHorizonConnectionStatus::Disconnected;

	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonHttpClient initialized (timeout=%.1fs, maxRetries=%d, retryDelay=%.1fs)."),
		ConnectionTimeoutSeconds, MaxRetryAttempts, RetryDelaySeconds);
}

// ============================================================
// Connection
// ============================================================

void UHorizonHttpClient::ConnectToServer(FOnRequestComplete OnComplete)
{
	const UHorizonConfig* Config = UHorizonConfig::Get();
	if (!Config || Config->Hosts.Num() == 0)
	{
		ConnectionStatus = EHorizonConnectionStatus::Failed;
		UE_LOG(LogHorizonSDK, Error, TEXT("ConnectToServer -- No hosts configured."));
		OnComplete.ExecuteIfBound(false, TEXT("No hosts configured in HorizonConfig."));
		return;
	}

	ConnectionStatus = EHorizonConnectionStatus::Connecting;

	const TArray<FString>& Hosts = Config->Hosts;

	if (Hosts.Num() == 1)
	{
		// Single host -- simple health check
		const FString& Host = Hosts[0];
		const FString Url = Host / TEXT("actuator/health");

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetVerb(TEXT("GET"));
		Request->SetURL(Url);
		Request->SetTimeout(ConnectionTimeoutSeconds);
		Request->SetHeader(TEXT("X-API-Key"), ApiKey);

		TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);
		FString CapturedHost = Host;
		FOnRequestComplete CapturedOnComplete = OnComplete;

		Request->OnProcessRequestComplete().BindLambda(
			[WeakSelf, CapturedHost, CapturedOnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Resp, bool bConnected)
			{
				UHorizonHttpClient* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (bConnected && Resp.IsValid())
				{
					const FString Body = Resp->GetContentAsString();
					if (Body.Contains(TEXT("UP")))
					{
						Self->ActiveHost = CapturedHost;
						Self->ConnectionStatus = EHorizonConnectionStatus::Connected;
						UE_LOG(LogHorizonSDK, Log, TEXT("Connected to %s"), *CapturedHost);
						CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
						return;
					}
				}

				Self->ConnectionStatus = EHorizonConnectionStatus::Failed;
				UE_LOG(LogHorizonSDK, Error, TEXT("Health check failed for %s"), *CapturedHost);
				CapturedOnComplete.ExecuteIfBound(false, FString::Printf(TEXT("Health check failed for %s"), *CapturedHost));
			});

		Request->ProcessRequest();
	}
	else
	{
		// Multiple hosts -- ping each 3 times, pick lowest average latency
		TSharedRef<TMap<FString, TArray<float>>> AllResults = MakeShared<TMap<FString, TArray<float>>>();
		TSharedRef<int32> PendingHosts = MakeShared<int32>(Hosts.Num());
		TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);
		FOnRequestComplete CapturedOnComplete = OnComplete;

		for (const FString& Host : Hosts)
		{
			TSharedRef<TArray<float>> Latencies = MakeShared<TArray<float>>();
			Latencies->SetNum(3);

			FString CapturedHost = Host;

			PingHost(Host, 0, Latencies, [WeakSelf, CapturedHost, Latencies, AllResults, PendingHosts, CapturedOnComplete]()
			{
				UHorizonHttpClient* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				AllResults->Add(CapturedHost, *Latencies);

				(*PendingHosts)--;
				if (*PendingHosts == 0)
				{
					Self->SelectBestHost(*AllResults, CapturedOnComplete);
				}
			});
		}
	}
}

void UHorizonHttpClient::Disconnect()
{
	ActiveHost.Empty();
	SessionToken.Empty();
	ConnectionStatus = EHorizonConnectionStatus::Disconnected;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonHttpClient disconnected."));
}

// ============================================================
// State
// ============================================================

bool UHorizonHttpClient::IsConnected() const
{
	return ConnectionStatus == EHorizonConnectionStatus::Connected;
}

FString UHorizonHttpClient::GetActiveHost() const
{
	return ActiveHost;
}

EHorizonConnectionStatus UHorizonHttpClient::GetConnectionStatus() const
{
	return ConnectionStatus;
}

void UHorizonHttpClient::SetSessionToken(const FString& Token)
{
	SessionToken = Token;
}

void UHorizonHttpClient::ClearSessionToken()
{
	SessionToken.Empty();
}

FString UHorizonHttpClient::GetSessionToken() const
{
	return SessionToken;
}

// ============================================================
// HTTP convenience methods
// ============================================================

void UHorizonHttpClient::Get(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;
	TArray<uint8> EmptyPayload;
	SendRequest(TEXT("GET"), Url, TEXT("application/json"), EmptyPayload, bUseSessionToken, 0, OnComplete);
}

void UHorizonHttpClient::PostJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Body, Writer);

	TArray<uint8> Payload;
	FTCHARToUTF8 Converter(*JsonString);
	Payload.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());

	SendRequest(TEXT("POST"), Url, TEXT("application/json"), Payload, bUseSessionToken, 0, OnComplete);
}

void UHorizonHttpClient::PostBinary(const FString& Endpoint, const TArray<uint8>& Data, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;
	SendRequest(TEXT("POST"), Url, TEXT("application/octet-stream"), Data, bUseSessionToken, 0, OnComplete);
}

void UHorizonHttpClient::GetBinary(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;
	TArray<uint8> EmptyPayload;
	SendRequest(TEXT("GET"), Url, TEXT("application/octet-stream"), EmptyPayload, bUseSessionToken, 0, OnComplete);
}

// ============================================================
// Internal request dispatcher
// ============================================================

void UHorizonHttpClient::SendRequest(
	const FString& Verb,
	const FString& Url,
	const FString& ContentType,
	const TArray<uint8>& Payload,
	bool bUseSessionToken,
	int32 RetryCount,
	FOnHttpResponse OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(Verb);
	Request->SetURL(Url);
	Request->SetTimeout(ConnectionTimeoutSeconds);

	ApplyHeaders(Request, ContentType, bUseSessionToken);

	if (Payload.Num() > 0)
	{
		Request->SetContent(Payload);
	}

	TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);
	FString CapturedVerb = Verb;
	FString CapturedUrl = Url;
	FString CapturedContentType = ContentType;
	TArray<uint8> CapturedPayload = Payload;
	FOnHttpResponse CapturedOnComplete = OnComplete;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakSelf, CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload, bUseSessionToken, RetryCount, CapturedOnComplete]
		(FHttpRequestPtr /*Req*/, FHttpResponsePtr Resp, bool bConnected)
		{
			UHorizonHttpClient* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			FHorizonNetworkResponse Response = Self->ParseResponse(Resp, bConnected);

			if (Response.bSuccess)
			{
				CapturedOnComplete.ExecuteIfBound(Response);
				return;
			}

			// Handle HTTP 429 (rate limited)
			if (Response.StatusCode == 429)
			{
				float RetryAfter = 1.0f;
				if (Resp.IsValid())
				{
					const FString RetryAfterHeader = Resp->GetHeader(TEXT("Retry-After"));
					if (!RetryAfterHeader.IsEmpty())
					{
						const float Parsed = FCString::Atof(*RetryAfterHeader);
						if (Parsed > 0.0f)
						{
							RetryAfter = Parsed;
						}
					}
				}

				UE_LOG(LogHorizonSDK, Warning, TEXT("Rate limited (429) on %s %s. Retrying after %.1fs (attempt %d/%d)."),
					*CapturedVerb, *CapturedUrl, RetryAfter, RetryCount + 1, Self->MaxRetryAttempts);

				if (RetryCount < Self->MaxRetryAttempts)
				{
					Self->ScheduleRetry(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
						bUseSessionToken, RetryCount, RetryAfter, CapturedOnComplete);
					return;
				}
			}

			// Handle 5xx / connection failure with retry
			if (Self->ShouldRetry(Response.StatusCode) && RetryCount < Self->MaxRetryAttempts)
			{
				UE_LOG(LogHorizonSDK, Warning, TEXT("Request failed (%d) on %s %s. Retrying in %.1fs (attempt %d/%d)."),
					Response.StatusCode, *CapturedVerb, *CapturedUrl,
					Self->RetryDelaySeconds, RetryCount + 1, Self->MaxRetryAttempts);

				Self->ScheduleRetry(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
					bUseSessionToken, RetryCount, Self->RetryDelaySeconds, CapturedOnComplete);
				return;
			}

			// No more retries -- deliver the error response
			CapturedOnComplete.ExecuteIfBound(Response);
		});

	Request->ProcessRequest();
}

// ============================================================
// Headers
// ============================================================

void UHorizonHttpClient::ApplyHeaders(
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request,
	const FString& ContentType,
	bool bUseSessionToken) const
{
	Request->SetHeader(TEXT("X-API-Key"), ApiKey);
	Request->SetHeader(TEXT("Content-Type"), ContentType);

	// For binary GET requests, also set the Accept header
	if (ContentType == TEXT("application/octet-stream") && Request->GetVerb() == TEXT("GET"))
	{
		Request->SetHeader(TEXT("Accept"), TEXT("application/octet-stream"));
	}

	if (bUseSessionToken && !SessionToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SessionToken));
	}
}

// ============================================================
// Response parsing
// ============================================================

FHorizonNetworkResponse UHorizonHttpClient::ParseResponse(FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully) const
{
	FHorizonNetworkResponse Response;

	if (!bConnectedSuccessfully || !HttpResponse.IsValid())
	{
		Response.bSuccess     = false;
		Response.StatusCode   = 0;
		Response.ErrorCode    = EHorizonErrorCode::ConnectionFailed;
		Response.ErrorMessage = TEXT("Connection failed");
		return Response;
	}

	Response.StatusCode = HttpResponse->GetResponseCode();
	Response.ErrorCode  = FHorizonNetworkResponse::StatusToErrorCode(Response.StatusCode);
	Response.bSuccess   = (Response.StatusCode >= 200 && Response.StatusCode < 300);

	const FString ContentTypeHeader = HttpResponse->GetContentType();

	if (ContentTypeHeader.Contains(TEXT("application/json")))
	{
		const FString BodyString = HttpResponse->GetContentAsString();
		if (!BodyString.IsEmpty())
		{
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyString);
			TSharedPtr<FJsonObject> JsonObject;
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				Response.JsonData = JsonObject;
			}
		}
	}
	else if (ContentTypeHeader.Contains(TEXT("application/octet-stream")))
	{
		Response.BinaryData = HttpResponse->GetContent();
	}

	// Extract error message for non-success responses
	if (!Response.bSuccess)
	{
		if (Response.JsonData.IsValid() && Response.JsonData->HasField(TEXT("message")))
		{
			Response.ErrorMessage = Response.JsonData->GetStringField(TEXT("message"));
		}
		else
		{
			Response.ErrorMessage = FString::Printf(TEXT("HTTP %d"), Response.StatusCode);
		}
	}

	// Special case: 204 No Content is still success (e.g. cloud save load-bytes "not found")
	if (Response.StatusCode == 204)
	{
		Response.bSuccess = true;
		Response.ErrorCode = EHorizonErrorCode::None;
		Response.ErrorMessage.Empty();
	}

	return Response;
}

// ============================================================
// Retry logic
// ============================================================

bool UHorizonHttpClient::ShouldRetry(int32 StatusCode) const
{
	return StatusCode >= 500 || StatusCode == 0;
}

void UHorizonHttpClient::ScheduleRetry(
	const FString& Verb,
	const FString& Url,
	const FString& ContentType,
	const TArray<uint8>& Payload,
	bool bUseSessionToken,
	int32 RetryCount,
	float DelaySeconds,
	FOnHttpResponse OnComplete)
{
	TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);

	// Capture everything needed for the deferred call
	FString CapturedVerb = Verb;
	FString CapturedUrl = Url;
	FString CapturedContentType = ContentType;
	TArray<uint8> CapturedPayload = Payload;
	FOnHttpResponse CapturedOnComplete = OnComplete;
	int32 NextRetryCount = RetryCount + 1;

	// Try to use UWorld timer first (game thread safe with pause support)
	UWorld* World = nullptr;
	if (GetOuter())
	{
		World = GetOuter()->GetWorld();
	}
	if (!World)
	{
		World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
	}

	if (World)
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			[WeakSelf, CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload, bUseSessionToken, NextRetryCount, CapturedOnComplete]()
			{
				UHorizonHttpClient* Self = WeakSelf.Get();
				if (Self)
				{
					Self->SendRequest(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
						bUseSessionToken, NextRetryCount, CapturedOnComplete);
				}
			},
			DelaySeconds,
			false
		);
	}
	else
	{
		// Fallback: FTSTicker (works even without a world, e.g. in editor utilities)
		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[WeakSelf, CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload, bUseSessionToken, NextRetryCount, CapturedOnComplete, DelaySeconds]
				(float DeltaTime) mutable -> bool
				{
					DelaySeconds -= DeltaTime;
					if (DelaySeconds > 0.0f)
					{
						return true; // Keep ticking
					}

					UHorizonHttpClient* Self = WeakSelf.Get();
					if (Self)
					{
						Self->SendRequest(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
							bUseSessionToken, NextRetryCount, CapturedOnComplete);
					}
					return false; // Remove ticker
				}
			)
		);
	}
}

// ============================================================
// Host ping / selection
// ============================================================

void UHorizonHttpClient::PingHost(const FString& Host, int32 AttemptIndex, TSharedRef<TArray<float>> Latencies, TFunction<void()> OnAllPingsDone)
{
	const FString Url = Host / TEXT("actuator/health");
	const double StartTime = FPlatformTime::Seconds();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(Url);
	Request->SetTimeout(ConnectionTimeoutSeconds);
	Request->SetHeader(TEXT("X-API-Key"), ApiKey);

	TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);
	FString CapturedHost = Host;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakSelf, CapturedHost, AttemptIndex, Latencies, OnAllPingsDone, StartTime]
		(FHttpRequestPtr /*Req*/, FHttpResponsePtr Resp, bool bConnected)
		{
			UHorizonHttpClient* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			const double Elapsed = (FPlatformTime::Seconds() - StartTime) * 1000.0; // ms

			if (bConnected && Resp.IsValid() && Resp->GetResponseCode() >= 200 && Resp->GetResponseCode() < 300)
			{
				(*Latencies)[AttemptIndex] = static_cast<float>(Elapsed);
				UE_LOG(LogHorizonSDK, Verbose, TEXT("Ping %s attempt %d: %.1f ms"), *CapturedHost, AttemptIndex + 1, Elapsed);
			}
			else
			{
				// Mark as very high latency so this host is deprioritised
				(*Latencies)[AttemptIndex] = TNumericLimits<float>::Max();
				UE_LOG(LogHorizonSDK, Warning, TEXT("Ping %s attempt %d failed."), *CapturedHost, AttemptIndex + 1);
			}

			if (AttemptIndex < 2)
			{
				Self->PingHost(CapturedHost, AttemptIndex + 1, Latencies, OnAllPingsDone);
			}
			else
			{
				OnAllPingsDone();
			}
		});

	Request->ProcessRequest();
}

void UHorizonHttpClient::SelectBestHost(const TMap<FString, TArray<float>>& AllResults, FOnRequestComplete OnComplete)
{
	FString BestHost;
	float BestAverage = TNumericLimits<float>::Max();

	for (const auto& Pair : AllResults)
	{
		const FString& Host = Pair.Key;
		const TArray<float>& Latencies = Pair.Value;

		float Sum = 0.0f;
		bool bAllFailed = true;
		for (float L : Latencies)
		{
			if (L < TNumericLimits<float>::Max())
			{
				bAllFailed = false;
			}
			Sum += L;
		}

		if (bAllFailed)
		{
			UE_LOG(LogHorizonSDK, Warning, TEXT("Host %s: all ping attempts failed."), *Host);
			continue;
		}

		const float Average = Sum / static_cast<float>(Latencies.Num());
		UE_LOG(LogHorizonSDK, Log, TEXT("Host %s: average latency %.1f ms"), *Host, Average);

		if (Average < BestAverage)
		{
			BestAverage = Average;
			BestHost = Host;
		}
	}

	if (BestHost.IsEmpty())
	{
		ConnectionStatus = EHorizonConnectionStatus::Failed;
		UE_LOG(LogHorizonSDK, Error, TEXT("All configured hosts are unreachable."));
		OnComplete.ExecuteIfBound(false, TEXT("All configured hosts are unreachable."));
		return;
	}

	ActiveHost = BestHost;
	ConnectionStatus = EHorizonConnectionStatus::Connected;
	UE_LOG(LogHorizonSDK, Log, TEXT("Connected to %s (avg latency %.1f ms)"), *BestHost, BestAverage);
	OnComplete.ExecuteIfBound(true, TEXT(""));
}
