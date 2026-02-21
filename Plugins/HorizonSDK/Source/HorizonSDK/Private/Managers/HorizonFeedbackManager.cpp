// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonFeedbackManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Misc/EngineVersion.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "RHI.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonFeedbackManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonFeedbackManager initialized."));
}

// ============================================================
// Submit
// ============================================================

void UHorizonFeedbackManager::Submit(const FString& Title, const FString& Category, const FString& Message,
	const FString& Email, bool bIncludeDeviceInfo, FOnRequestComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("title"), Title);
	Body->SetStringField(TEXT("category"), Category);
	Body->SetStringField(TEXT("message"), Message);

	if (!Email.IsEmpty())
	{
		Body->SetStringField(TEXT("email"), Email);
	}

	// Attach userId if signed in
	if (AuthManager && AuthManager->IsSignedIn())
	{
		Body->SetStringField(TEXT("userId"), AuthManager->GetCurrentUser().UserId);
	}

	if (bIncludeDeviceInfo)
	{
		Body->SetStringField(TEXT("deviceInfo"), CollectDeviceInfo());
	}

	TWeakObjectPtr<UHorizonFeedbackManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-feedback/submit"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("Feedback::Submit -- Feedback submitted successfully."));
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("Feedback::Submit -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

// ============================================================
// Convenience Methods
// ============================================================

void UHorizonFeedbackManager::ReportBug(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email)
{
	Submit(Title, TEXT("BUG"), Message, Email, /*bIncludeDeviceInfo=*/ true, OnComplete);
}

void UHorizonFeedbackManager::RequestFeature(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email)
{
	Submit(Title, TEXT("FEATURE"), Message, Email, /*bIncludeDeviceInfo=*/ false, OnComplete);
}

void UHorizonFeedbackManager::SendGeneral(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email)
{
	Submit(Title, TEXT("GENERAL"), Message, Email, /*bIncludeDeviceInfo=*/ false, OnComplete);
}

// ============================================================
// Device Info
// ============================================================

FString UHorizonFeedbackManager::CollectDeviceInfo() const
{
	return FString::Printf(TEXT("UE %s | %s | %s | %s"),
		*FEngineVersion::Current().ToString(),
		*FPlatformMisc::GetOSVersion(),
		*FPlatformMisc::GetDeviceMakeAndModel(),
		GRHIAdapterName.IsEmpty() ? TEXT("Unknown GPU") : *GRHIAdapterName
	);
}
