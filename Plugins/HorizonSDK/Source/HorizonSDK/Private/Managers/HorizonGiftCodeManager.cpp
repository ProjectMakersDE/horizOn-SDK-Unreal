// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonGiftCodeManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"

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
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Redeem -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("code"), Code);
	Body->SetStringField(TEXT("userId"), UserId);

	TWeakObjectPtr<UHorizonGiftCodeManager> WeakSelf(this);
	FOnGiftCodeRedeemComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/gift-codes/redeem"), true,
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
