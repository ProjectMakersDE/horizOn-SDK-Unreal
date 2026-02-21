// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonUserLogManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonUserLogManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonUserLogManager initialized."));
}

// ============================================================
// CreateLog
// ============================================================

void UHorizonUserLogManager::CreateLog(EHorizonLogLevel Level, const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("UserLog::CreateLog -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	// Validate and truncate message
	FString TruncatedMessage = Message;
	if (TruncatedMessage.Len() > MaxMessageLength)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("UserLog::CreateLog -- Message exceeds %d chars, truncating."), MaxMessageLength);
		TruncatedMessage = TruncatedMessage.Left(MaxMessageLength);
	}

	// Validate and truncate error code
	FString TruncatedErrorCode = ErrorCode;
	if (TruncatedErrorCode.Len() > MaxErrorCodeLength)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("UserLog::CreateLog -- ErrorCode exceeds %d chars, truncating."), MaxErrorCodeLength);
		TruncatedErrorCode = TruncatedErrorCode.Left(MaxErrorCodeLength);
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString TypeString = LogLevelToString(Level);

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("message"), TruncatedMessage);
	Body->SetStringField(TEXT("type"), TypeString);
	Body->SetStringField(TEXT("userId"), UserId);

	if (!TruncatedErrorCode.IsEmpty())
	{
		Body->SetStringField(TEXT("errorCode"), TruncatedErrorCode);
	}

	TWeakObjectPtr<UHorizonUserLogManager> WeakSelf(this);
	FOnUserLogComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-logs/create"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("UserLog::CreateLog -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
					return;
				}

				FString LogId;
				FString CreatedAt;

				if (Response.JsonData.IsValid())
				{
					LogId = Response.JsonData->GetStringField(TEXT("id"));
					CreatedAt = Response.JsonData->GetStringField(TEXT("createdAt"));
				}

				UE_LOG(LogHorizonSDK, Verbose, TEXT("UserLog::CreateLog -- Log created: id=%s, createdAt=%s"), *LogId, *CreatedAt);
				CapturedOnComplete.ExecuteIfBound(true, LogId, CreatedAt);
			}
		));
}

// ============================================================
// Convenience Methods
// ============================================================

void UHorizonUserLogManager::Info(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode)
{
	CreateLog(EHorizonLogLevel::Info, Message, OnComplete, ErrorCode);
}

void UHorizonUserLogManager::Warn(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode)
{
	CreateLog(EHorizonLogLevel::Warning, Message, OnComplete, ErrorCode);
}

void UHorizonUserLogManager::Error(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode)
{
	CreateLog(EHorizonLogLevel::Error, Message, OnComplete, ErrorCode);
}

// ============================================================
// Helpers
// ============================================================

FString UHorizonUserLogManager::LogLevelToString(EHorizonLogLevel Level)
{
	switch (Level)
	{
	case EHorizonLogLevel::Info:
		return TEXT("INFO");
	case EHorizonLogLevel::Warning:
		return TEXT("WARN");
	case EHorizonLogLevel::Error:
		return TEXT("ERROR");
	default:
		return TEXT("INFO");
	}
}
