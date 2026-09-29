// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonPlayerProfileManager.h"
#include "HorizonSDKModule.h"
#include "Transport/HorizonPlayerProfileTransportContract.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <string>
#include <vector>

// ============================================================
// Initialization
// ============================================================

void UHorizonPlayerProfileManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;

	// The cache belongs to one player: drop it whenever the signed-in player changes.
	if (AuthManager)
	{
		AuthManager->OnUserSignedIn.AddUniqueDynamic(this, &UHorizonPlayerProfileManager::HandleUserSignedIn);
		AuthManager->OnUserSignedOut.AddUniqueDynamic(this, &UHorizonPlayerProfileManager::HandleUserSignedOut);
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonPlayerProfileManager initialized."));
}

// ============================================================
// Get Profile
// ============================================================

void UHorizonPlayerProfileManager::GetProfile(FOnPlayerProfileComplete OnComplete)
{
	if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("PlayerProfile::GetProfile -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(), TEXT("SESSION_REQUIRED"), TEXT("A signed-in player is required."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const HorizonTransportContract::FPlayerProfileRequestPlan Plan =
		HorizonTransportContract::BuildPlayerProfileGetPlan(
			TCHAR_TO_UTF8(*UserId),
			TCHAR_TO_UTF8(*HttpClient->GetSessionToken()));
	if (!Plan.bShouldSend)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("PlayerProfile::GetProfile -- %s"), UTF8_TO_TCHAR(Plan.ErrorMessage.c_str()));
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(),
			UTF8_TO_TCHAR(Plan.ErrorCode.c_str()), UTF8_TO_TCHAR(Plan.ErrorMessage.c_str()));
		return;
	}

	TWeakObjectPtr<UHorizonPlayerProfileManager> WeakSelf(this);
	FOnPlayerProfileComplete CapturedOnComplete = OnComplete;
	const FString Endpoint = UTF8_TO_TCHAR(Plan.Endpoint.c_str());

	HttpClient->Get(Endpoint, Plan.bUseSessionToken,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, UserId](const FHorizonNetworkResponse& Response)
			{
				UHorizonPlayerProfileManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}
				Self->HandleResponse(Response, UserId, TEXT("GetProfile"), CapturedOnComplete);
			}
		));
}

// ============================================================
// Set Profile
// ============================================================

void UHorizonPlayerProfileManager::SetProfile(const FString& AvatarId, const FString& FrameId, const TArray<FString>& Badges, FOnPlayerProfileComplete OnComplete)
{
	if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("PlayerProfile::SetProfile -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(), TEXT("SESSION_REQUIRED"), TEXT("A signed-in player is required."));
		return;
	}

	std::vector<std::string> BadgeIds;
	BadgeIds.reserve(Badges.Num());
	for (const FString& Badge : Badges)
	{
		BadgeIds.emplace_back(TCHAR_TO_UTF8(*Badge));
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const HorizonTransportContract::FPlayerProfileRequestPlan Plan =
		HorizonTransportContract::BuildPlayerProfileSetPlan(
			TCHAR_TO_UTF8(*UserId),
			TCHAR_TO_UTF8(*HttpClient->GetSessionToken()),
			TCHAR_TO_UTF8(*AvatarId),
			TCHAR_TO_UTF8(*FrameId),
			BadgeIds);
	if (!Plan.bShouldSend)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("PlayerProfile::SetProfile -- %s"), UTF8_TO_TCHAR(Plan.ErrorMessage.c_str()));
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(),
			UTF8_TO_TCHAR(Plan.ErrorCode.c_str()), UTF8_TO_TCHAR(Plan.ErrorMessage.c_str()));
		return;
	}

	TSharedPtr<FJsonObject> ParsedBody;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Plan.BodyJson.c_str()));
	if (!FJsonSerializer::Deserialize(Reader, ParsedBody) || !ParsedBody.IsValid())
	{
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(), TEXT("INVALID_REQUEST"), TEXT("Failed to build player profile request."));
		return;
	}

	TWeakObjectPtr<UHorizonPlayerProfileManager> WeakSelf(this);
	FOnPlayerProfileComplete CapturedOnComplete = OnComplete;
	const FString Endpoint = UTF8_TO_TCHAR(Plan.Endpoint.c_str());

	HttpClient->PutJson(ParsedBody.ToSharedRef(), Endpoint, Plan.bUseSessionToken,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, UserId](const FHorizonNetworkResponse& Response)
			{
				UHorizonPlayerProfileManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}
				Self->HandleResponse(Response, UserId, TEXT("SetProfile"), CapturedOnComplete);
			}
		));
}

// ============================================================
// Cache
// ============================================================

void UHorizonPlayerProfileManager::ClearCache()
{
	CurrentProfile = FHorizonPlayerProfileResult();
	bHasCurrentProfile = false;
	UE_LOG(LogHorizonSDK, Verbose, TEXT("Player profile cache cleared."));
}

void UHorizonPlayerProfileManager::HandleUserSignedIn()
{
	ClearCache();
}

void UHorizonPlayerProfileManager::HandleUserSignedOut()
{
	ClearCache();
}

// ============================================================
// Helpers
// ============================================================

void UHorizonPlayerProfileManager::HandleResponse(const FHorizonNetworkResponse& Response, const FString& RequestUserId,
	const TCHAR* Operation, const FOnPlayerProfileComplete& OnComplete)
{
	if (!Response.bSuccess)
	{
		const FString ErrorCode = Response.GetErrorCodeString();
		UE_LOG(LogHorizonSDK, Warning, TEXT("PlayerProfile::%s -- Failed (%s): %s"), Operation, *ErrorCode, *Response.ErrorMessage);
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(), ErrorCode, Response.ErrorMessage);
		return;
	}

	if (!Response.JsonData.IsValid())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("PlayerProfile::%s -- Response has no JSON body."), Operation);
		OnComplete.ExecuteIfBound(false, FHorizonPlayerProfileResult(), TEXT("INVALID_RESPONSE"), TEXT("The server response could not be read."));
		return;
	}

	const FHorizonPlayerProfileResult Result = FHorizonPlayerProfileResult::FromJson(Response.JsonData);

	// Only cache when the same player is still signed in (a sign-out may have happened meanwhile).
	if (AuthManager && AuthManager->IsSignedIn() && AuthManager->GetCurrentUser().UserId == RequestUserId)
	{
		CurrentProfile = Result;
		bHasCurrentProfile = true;
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("PlayerProfile::%s -- Avatar '%s', frame '%s', %d badges, %d unlocks, %d cosmetics."),
		Operation, *Result.Profile.AvatarId, *Result.Profile.FrameId,
		Result.Profile.Badges.Num(), Result.Unlocks.Num(), Result.Cosmetics.Num());
	OnComplete.ExecuteIfBound(true, Result, FString(), FString());
}
