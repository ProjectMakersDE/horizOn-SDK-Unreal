// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonGiftCodeManager.h"
#include "HorizonSDKModule.h"
#include "Transport/HorizonGiftCodeTransportContract.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonGiftCodeManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonGiftCodeManager initialized."));
}

// ============================================================
// Redeem
// ============================================================

void UHorizonGiftCodeManager::Redeem(const FString& Code, FOnGiftCodeRedeemComplete OnComplete)
{
	if (!AuthManager || !HttpClient || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Redeem -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT("User is not signed in."));
		return;
	}

	if (Code.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Redeem -- Gift code is required."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT("Gift code is required."));
		return;
	}

	// The server binds redemption to the player's Bearer session (TASK-886):
	// only build the request when a session token exists.
	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const HorizonTransportContract::FGiftCodeRedeemPlan Plan =
		HorizonTransportContract::BuildGiftCodeRedeemPlan(
			TCHAR_TO_UTF8(*UserId),
			TCHAR_TO_UTF8(*HttpClient->GetSessionToken()),
			TCHAR_TO_UTF8(*Code));
	if (!Plan.bShouldSend)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Redeem -- No player session available."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT("User is not signed in."));
		return;
	}

	TSharedPtr<FJsonObject> ParsedBody;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Plan.BodyJson.c_str()));
	if (!FJsonSerializer::Deserialize(Reader, ParsedBody) || !ParsedBody.IsValid())
	{
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT("Failed to build gift code request."));
		return;
	}

	TWeakObjectPtr<UHorizonGiftCodeManager> WeakSelf(this);
	FOnGiftCodeRedeemComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(ParsedBody.ToSharedRef(), TEXT("api/v1/app/gift-codes/redeem"), Plan.bUseSessionToken,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Redeem -- Request failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""), Response.ErrorMessage);
					return;
				}

				bool bServerSuccess = false;
				FString GiftData;
				FString Message;

				if (Response.JsonData.IsValid())
				{
					bServerSuccess = Response.JsonData->GetBoolField(TEXT("success"));
					GiftData = Response.JsonData->GetStringField(TEXT("giftData"));
					Message = Response.JsonData->GetStringField(TEXT("message"));
				}

				if (bServerSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("GiftCode::Redeem -- Code redeemed successfully. Message: %s"), *Message);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Redeem -- Server rejected code. Message: %s"), *Message);
				}

				CapturedOnComplete.ExecuteIfBound(bServerSuccess, GiftData, Message);
			}
		));
}

// ============================================================
// Validate
// ============================================================

void UHorizonGiftCodeManager::Validate(const FString& Code, FOnGiftCodeValidateComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Validate -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, false);
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("code"), Code);
	Body->SetStringField(TEXT("userId"), UserId);

	TWeakObjectPtr<UHorizonGiftCodeManager> WeakSelf(this);
	FOnGiftCodeValidateComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/gift-codes/validate"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Validate -- Request failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, false);
					return;
				}

				bool bValid = false;
				if (Response.JsonData.IsValid())
				{
					bValid = Response.JsonData->GetBoolField(TEXT("valid"));
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("GiftCode::Validate -- Code is %s."), bValid ? TEXT("valid") : TEXT("invalid"));
				CapturedOnComplete.ExecuteIfBound(true, bValid);
			}
		));
}
