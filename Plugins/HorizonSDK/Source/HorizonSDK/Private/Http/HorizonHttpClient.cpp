// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Http/HorizonHttpClient.h"
#include "Transport/HorizonGiftCodeTransportContract.h"
#include "Http/HorizonLosslessJson.h"
#include "Engine/Engine.h"
#include "HorizonConfig.h"
#include "HorizonSDKModule.h"
#include "Transport/HorizonLeaderboardTransportContract.h"
#include "Transport/HorizonRetryContract.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/App.h"
#include "Containers/Ticker.h"

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

	++TransportContextEpoch;
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
						if (Self->ActiveHost != CapturedHost) ++Self->TransportContextEpoch;
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
	++TransportContextEpoch;
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
	if (SessionToken != Token) ++SessionContextEpoch;
	SessionToken = Token;
}

void UHorizonHttpClient::ClearSessionToken()
{
	SetSessionToken(TEXT(""));
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

void UHorizonHttpClient::PostJsonRead(const TSharedRef<FJsonObject>& Body,const FString& Endpoint,FOnHttpResponse Complete)
{
    FString Json;if(!HorizonLosslessJson::Object(Body,Json)){FHorizonNetworkResponse R;R.ServerErrorCode=TEXT("INVALID_REQUEST");R.ErrorMessage=TEXT("Invalid read JSON");Complete.ExecuteIfBound(R);return;}
    const FTCHARToUTF8 Bytes(*Json,Json.Len());TArray<uint8> Payload;Payload.Append(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    SendRequest(TEXT("POST"),ActiveHost/Endpoint,TEXT("application/json"),Payload,true,0,Complete);
}
void UHorizonHttpClient::RevisionJsonOnce(const TSharedPtr<FJsonObject>& Body,const FString& Endpoint,const FString& Verb,int64 Revision,FOnHttpResponse Complete)
{
    if((Verb!=TEXT("POST")&&Verb!=TEXT("PUT")&&Verb!=TEXT("DELETE"))||Revision<0||Revision>9007199254740991LL||(Verb==TEXT("DELETE")?Body.IsValid():!Body.IsValid())){FHorizonNetworkResponse R;R.ServerErrorCode=TEXT("INVALID_REQUEST");R.ErrorMessage=TEXT("Original revisioned write required");Complete.ExecuteIfBound(R);return;}
    TArray<uint8> Payload;if(Body){FString Json;if(!HorizonLosslessJson::Object(Body.ToSharedRef(),Json)){FHorizonNetworkResponse R;R.ServerErrorCode=TEXT("INVALID_REQUEST");R.ErrorMessage=TEXT("Invalid write JSON");Complete.ExecuteIfBound(R);return;}const FTCHARToUTF8 Bytes(*Json,Json.Len());Payload.Append(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());}
    TMap<FString,FString> Headers;Headers.Add(TEXT("If-Match"),LexToString(Revision));SendRequest(Verb,ActiveHost/Endpoint,TEXT("application/json"),Payload,true,0,Complete,TEXT(""),Headers,false);
}

void UHorizonHttpClient::PostOriginalJsonOnce(const FString& Json,const FString& Endpoint,FOnHttpResponse Complete)
{
    const FTCHARToUTF8 Bytes(*Json,Json.Len());TArray<uint8> Payload;Payload.Append(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    SendRequest(TEXT("POST"),ActiveHost/Endpoint,TEXT("application/json"),Payload,true,0,Complete,TEXT(""),TMap<FString,FString>(),false);
}

void UHorizonHttpClient::JsonOnce(const TSharedRef<FJsonObject>& Body,const FString& Endpoint,bool bPut,bool bUseSessionToken,FOnHttpResponse Complete)
{
    FString Json;if(!HorizonLosslessJson::Object(Body,Json)){FHorizonNetworkResponse R;R.ServerErrorCode=TEXT("INVALID_REQUEST");R.ErrorMessage=TEXT("Invalid JSON write");Complete.ExecuteIfBound(R);return;}
    const FTCHARToUTF8 Bytes(*Json);TArray<uint8> Payload;Payload.Append(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    SendRequest(bPut?TEXT("PUT"):TEXT("POST"),ActiveHost/Endpoint,TEXT("application/json"),Payload,bUseSessionToken,0,Complete,TEXT(""),TMap<FString,FString>(),false);
}

void UHorizonHttpClient::PostJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete,
	const TMap<FString, FString>& ExtraHeaders)
{
	const FString Url = ActiveHost / Endpoint;

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Body, Writer);

	TArray<uint8> Payload;
	FTCHARToUTF8 Converter(*JsonString);
	Payload.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());

	FString Campaign,Count,Revision; double Protocol=0;
    const auto CampaignValue=Body->TryGetField(TEXT("campaignId")),CountValue=Body->TryGetField(TEXT("expectedCount")),RevisionValue=Body->TryGetField(TEXT("configRevision"));
    const bool StringIdentity=CampaignValue && CampaignValue->Type==EJson::String && CountValue && CountValue->Type==EJson::String && RevisionValue && RevisionValue->Type==EJson::String;
    const bool HasId=StringIdentity && Body->TryGetNumberField(TEXT("protocolVersion"),Protocol) && Protocol==2 && Body->TryGetStringField(TEXT("campaignId"),Campaign) &&
        HorizonTransportContract::GiftUuid(TCHAR_TO_UTF8(*Campaign)) && Body->TryGetStringField(TEXT("expectedCount"),Count) && Body->TryGetStringField(TEXT("configRevision"),Revision) &&
        HorizonTransportContract::GiftCount(TCHAR_TO_UTF8(*Count)) && HorizonTransportContract::GiftCount(TCHAR_TO_UTF8(*Revision));
	const bool bAllowRetry = HorizonTransportContract::AllowJsonPostRetry(TCHAR_TO_UTF8(*Endpoint), HasId);
    SendRequest(TEXT("POST"), Url, TEXT("application/json"), Payload, bUseSessionToken, 0, OnComplete, TEXT(""), ExtraHeaders, bAllowRetry);
}

void UHorizonHttpClient::PostJsonForBinary(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Body, Writer);
	TArray<uint8> Payload;
	FTCHARToUTF8 Converter(*JsonString);
	Payload.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());

	FString Campaign,Count,Revision; double Protocol=0;
    const auto CampaignValue=Body->TryGetField(TEXT("campaignId")),CountValue=Body->TryGetField(TEXT("expectedCount")),RevisionValue=Body->TryGetField(TEXT("configRevision"));
    const bool StringIdentity=CampaignValue && CampaignValue->Type==EJson::String && CountValue && CountValue->Type==EJson::String && RevisionValue && RevisionValue->Type==EJson::String;
    const bool HasId=StringIdentity && Body->TryGetNumberField(TEXT("protocolVersion"),Protocol) && Protocol==2 && Body->TryGetStringField(TEXT("campaignId"),Campaign) &&
        HorizonTransportContract::GiftUuid(TCHAR_TO_UTF8(*Campaign)) && Body->TryGetStringField(TEXT("expectedCount"),Count) && Body->TryGetStringField(TEXT("configRevision"),Revision) &&
        HorizonTransportContract::GiftCount(TCHAR_TO_UTF8(*Count)) && HorizonTransportContract::GiftCount(TCHAR_TO_UTF8(*Revision));
	const bool bAllowRetry = HorizonTransportContract::AllowJsonPostRetry(TCHAR_TO_UTF8(*Endpoint), HasId);
	SendRequest(TEXT("POST"), Url, TEXT("application/json"), Payload, bUseSessionToken, 0, OnComplete,
		TEXT("application/octet-stream"), TMap<FString,FString>(), bAllowRetry);
}

void UHorizonHttpClient::PutJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Body, Writer);

	TArray<uint8> Payload;
	FTCHARToUTF8 Converter(*JsonString);
	Payload.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());

	// Same headers, retries and 429 handling as PostJson (shared SendRequest).
	SendRequest(TEXT("PUT"), Url, TEXT("application/json"), Payload, bUseSessionToken, 0, OnComplete);
}

void UHorizonHttpClient::PostBinary(const FString& Endpoint, const TArray<uint8>& Data, bool bUseSessionToken, FOnHttpResponse OnComplete,
	const TMap<FString, FString>& ExtraHeaders)
{
	const FString Url = ActiveHost / Endpoint;
	SendRequest(TEXT("POST"), Url, TEXT("application/octet-stream"), Data, bUseSessionToken, 0, OnComplete, TEXT(""), ExtraHeaders);
}

void UHorizonHttpClient::Delete(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete)
{
	const FString Url = ActiveHost / Endpoint;
	TArray<uint8> EmptyPayload;
	SendRequest(TEXT("DELETE"), Url, TEXT("application/json"), EmptyPayload, bUseSessionToken, 0, OnComplete);
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
	FOnHttpResponse OnComplete,
	const FString& AcceptContentType,
	const TMap<FString, FString>& ExtraHeaders,
    bool bAllowRetry)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(Verb);
	Request->SetURL(Url);
	Request->SetTimeout(ConnectionTimeoutSeconds);

	ApplyHeaders(Request, ContentType, bUseSessionToken, AcceptContentType, ExtraHeaders);

	if (Payload.Num() > 0)
	{
		Request->SetContent(Payload);
	}

	TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);
	FString CapturedVerb = Verb;
	FString CapturedUrl = Url;
	FString CapturedContentType = ContentType;
	FString CapturedAcceptContentType = AcceptContentType;
	TMap<FString, FString> CapturedExtraHeaders = ExtraHeaders;
	TArray<uint8> CapturedPayload = Payload;
	FOnHttpResponse CapturedOnComplete = OnComplete;
	const FString RequestHost = ActiveHost, RequestKey = ApiKey, RequestSession = SessionToken;
	const uint64 RequestGeneration = TransportContextEpoch;
	const uint64 SessionGeneration = SessionContextEpoch;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakSelf, CapturedVerb, CapturedUrl, CapturedContentType, CapturedAcceptContentType, CapturedExtraHeaders, CapturedPayload, bUseSessionToken, RetryCount, CapturedOnComplete, RequestHost, RequestKey, RequestSession, RequestGeneration, SessionGeneration, bAllowRetry]
		(FHttpRequestPtr /*Req*/, FHttpResponsePtr Resp, bool bConnected)
		{
			UHorizonHttpClient* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			if (Self->TransportContextEpoch != RequestGeneration || (bUseSessionToken && Self->SessionContextEpoch != SessionGeneration) || Self->ActiveHost != RequestHost || Self->ApiKey != RequestKey || (bUseSessionToken && Self->SessionToken != RequestSession))
			{
				FHorizonNetworkResponse Changed; Changed.ServerErrorCode = TEXT("SESSION_CHANGED"); Changed.ErrorMessage = TEXT("Request identity changed");
				CapturedOnComplete.ExecuteIfBound(Changed); return;
			}
			FHorizonNetworkResponse Response = Self->ParseResponse(Resp, bConnected);

			if (Response.bSuccess)
			{
				CapturedOnComplete.ExecuteIfBound(Response);
				return;
			}

            // A lost signup response must not create a second server identity.
            if (!bAllowRetry) { CapturedOnComplete.ExecuteIfBound(Response); return; }

			// Handle HTTP 429 (rate limited)
			if (Response.StatusCode == 429)
			{
				double ServerRetryAfter = 1.0;
				if (Resp.IsValid())
				{
					const FString RetryAfterHeader = Resp->GetHeader(TEXT("Retry-After"));
					if (!RetryAfterHeader.IsEmpty())
					{
						ServerRetryAfter = FCString::Atod(*RetryAfterHeader);
					}
				}

				// Feature-specific policies are surfaced immediately, including future unknown codes.
				if (HorizonTransportContract::IsFeatureRateLimitCode(TCHAR_TO_UTF8(*Response.ServerErrorCode)))
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Rate limited (429, %s) on %s %s. Not retried: %s"),
						*Response.ServerErrorCode, *CapturedVerb, *CapturedUrl, *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(Response);
					return;
				}

				if (RetryCount < Self->MaxRetryAttempts)
				{
					const float RetryAfter = HorizonTransportContract::GlobalRateRetryDelay(ServerRetryAfter, FMath::FRand());
					UE_LOG(LogHorizonSDK, Warning, TEXT("Rate limited (429) on %s %s. Retrying after %.1fs (attempt %d/%d)."),
						*CapturedVerb, *CapturedUrl, RetryAfter, RetryCount + 1, Self->MaxRetryAttempts);

					Self->ScheduleRetry(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
						bUseSessionToken, RetryCount, RetryAfter, CapturedOnComplete, CapturedAcceptContentType, CapturedExtraHeaders);
					return;
				}

				// Still rate limited after the last attempt: deliver the 429 with its clear
				// message from ParseResponse (no 5xx retry below, 429 is not retryable there).
				UE_LOG(LogHorizonSDK, Warning, TEXT("Rate limited (429) on %s %s. Giving up after %d retries: %s"),
					*CapturedVerb, *CapturedUrl, Self->MaxRetryAttempts, *Response.ErrorMessage);
			}

			// Handle 5xx / connection failure with retry
			if (Self->ShouldRetry(Response.StatusCode) && RetryCount < Self->MaxRetryAttempts)
			{
				UE_LOG(LogHorizonSDK, Warning, TEXT("Request failed (%d) on %s %s. Retrying in %.1fs (attempt %d/%d)."),
					Response.StatusCode, *CapturedVerb, *CapturedUrl,
					Self->RetryDelaySeconds, RetryCount + 1, Self->MaxRetryAttempts);

				Self->ScheduleRetry(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
					bUseSessionToken, RetryCount, Self->RetryDelaySeconds, CapturedOnComplete, CapturedAcceptContentType, CapturedExtraHeaders);
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
	bool bUseSessionToken,
	const FString& AcceptContentType,
	const TMap<FString, FString>& ExtraHeaders) const
{
	for (const auto& Header : HorizonTransportContract::BuildHeaders(
		TCHAR_TO_UTF8(*ApiKey), TCHAR_TO_UTF8(*SessionToken), bUseSessionToken))
	{
		Request->SetHeader(UTF8_TO_TCHAR(Header.first.c_str()), UTF8_TO_TCHAR(Header.second.c_str()));
	}
	Request->SetHeader(TEXT("Content-Type"), ContentType);

	// Preserve explicit response negotiation on every retry.
	if (!AcceptContentType.IsEmpty())
	{
		Request->SetHeader(TEXT("Accept"), AcceptContentType);
	}
	else if (ContentType == TEXT("application/octet-stream") && Request->GetVerb() == TEXT("GET"))
	{
		Request->SetHeader(TEXT("Accept"), TEXT("application/octet-stream"));
	}

	for (const auto& Header : ExtraHeaders)
	{
		Request->SetHeader(Header.Key, Header.Value);
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
	Response.Headers    = HttpResponse->GetAllHeaders();
	Response.bSuccess   = (Response.StatusCode >= 200 && Response.StatusCode < 300);

	const FString ContentTypeHeader = HttpResponse->GetContentType();

	if (ContentTypeHeader.Contains(TEXT("application/json")))
	{
		const FString BodyString = HttpResponse->GetContentAsString();
        Response.RawJsonBody = BodyString;
		if (!BodyString.IsEmpty())
		{
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyString);
			TSharedPtr<FJsonObject> JsonObject;
			// Keep exact numeric lexemes for features with strict integer contracts.
			if (FJsonSerializer::Deserialize(Reader, JsonObject, FJsonSerializer::EFlags::StoreNumbersAsStrings) && JsonObject.IsValid())
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
		// Machine readable server code (for example "COSMETIC_LOCKED"), only when it is a JSON string.
		if (Response.JsonData.IsValid())
		{
			const TSharedPtr<FJsonValue> CodeValue = Response.JsonData->TryGetField(TEXT("code"));
			if (CodeValue.IsValid() && CodeValue->Type == EJson::String)
			{
				Response.ServerErrorCode = CodeValue->AsString();
			}
		}

		if (Response.JsonData.IsValid() && Response.JsonData->HasField(TEXT("message")))
		{
			Response.ErrorMessage = Response.JsonData->GetStringField(TEXT("message"));
		}
		else if (Response.StatusCode == 429)
		{
			// The server answers 429 with an empty body and a Retry-After header (seconds).
			const double RetryAfterSeconds = FCString::Atod(*HttpResponse->GetHeader(TEXT("Retry-After")));
			Response.ErrorMessage = std::isfinite(RetryAfterSeconds) && RetryAfterSeconds > 0.0
				? FString::Printf(TEXT("Rate limit exceeded (HTTP 429). Try again in %.0f seconds."), std::ceil(RetryAfterSeconds))
				: FString(TEXT("Rate limit exceeded (HTTP 429). Try again later."));
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
		Response.ServerErrorCode.Empty();
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
	FOnHttpResponse OnComplete,
	const FString& AcceptContentType,
	const TMap<FString, FString>& ExtraHeaders)
{
	TWeakObjectPtr<UHorizonHttpClient> WeakSelf(this);

	// Capture everything needed for the deferred call
	FString CapturedVerb = Verb;
	FString CapturedUrl = Url;
	FString CapturedContentType = ContentType;
	FString CapturedAcceptContentType = AcceptContentType;
	TMap<FString, FString> CapturedExtraHeaders = ExtraHeaders;
	TArray<uint8> CapturedPayload = Payload;
	FOnHttpResponse CapturedOnComplete = OnComplete;
	int32 NextRetryCount = RetryCount + 1;
	const FString RequestHost = ActiveHost, RequestKey = ApiKey, RequestSession = SessionToken;
	const uint64 RequestGeneration = TransportContextEpoch;
	const uint64 SessionGeneration = SessionContextEpoch;

	// Network backoff uses monotone wall time, independent of game time dilation/pause.
	const double RetryAt = FPlatformTime::Seconds() + double(DelaySeconds);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[WeakSelf, CapturedVerb, CapturedUrl, CapturedContentType, CapturedAcceptContentType,
		 CapturedExtraHeaders, CapturedPayload, bUseSessionToken, NextRetryCount, CapturedOnComplete,
		 RequestHost, RequestKey, RequestSession, RequestGeneration, SessionGeneration, RetryAt](float) -> bool
		{
			if (FPlatformTime::Seconds() < RetryAt) return true;
			if (auto* Self = WeakSelf.Get())
			{
				if (Self->TransportContextEpoch != RequestGeneration || (bUseSessionToken && Self->SessionContextEpoch != SessionGeneration) || Self->ActiveHost != RequestHost || Self->ApiKey != RequestKey || (bUseSessionToken && Self->SessionToken != RequestSession))
				{
					FHorizonNetworkResponse Changed; Changed.ServerErrorCode = TEXT("SESSION_CHANGED"); Changed.ErrorMessage = TEXT("Request identity changed before retry");
					CapturedOnComplete.ExecuteIfBound(Changed); return false;
				}
				Self->SendRequest(CapturedVerb, CapturedUrl, CapturedContentType, CapturedPayload,
					bUseSessionToken, NextRetryCount, CapturedOnComplete, CapturedAcceptContentType, CapturedExtraHeaders);
			}
			return false;
		}));
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

	if (ActiveHost != BestHost) ++TransportContextEpoch;
	ActiveHost = BestHost;
	ConnectionStatus = EHorizonConnectionStatus::Connected;
	UE_LOG(LogHorizonSDK, Log, TEXT("Connected to %s (avg latency %.1f ms)"), *BestHost, BestAverage);
	OnComplete.ExecuteIfBound(true, TEXT(""));
}
