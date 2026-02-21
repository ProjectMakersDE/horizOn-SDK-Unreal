// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonCloudSaveManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonCloudSaveManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonCloudSaveManager initialized."));
}

// ============================================================
// JSON Save / Load
// ============================================================

void UHorizonCloudSaveManager::Save(const FString& Data, FOnRequestComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Save -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);
	Body->SetStringField(TEXT("saveData"), Data);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/cloud-save/save"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::Save -- Data saved successfully."));
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Save -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonCloudSaveManager::Load(FOnStringComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Load -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT(""));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnStringComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/cloud-save/load"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Load -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
					return;
				}

				bool bFound = false;
				FString SaveData;

				if (Response.JsonData.IsValid())
				{
					bFound = Response.JsonData->GetBoolField(TEXT("found"));
					if (bFound)
					{
						SaveData = Response.JsonData->GetStringField(TEXT("saveData"));
					}
				}

				if (bFound)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::Load -- Data loaded successfully."));
					CapturedOnComplete.ExecuteIfBound(true, SaveData);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::Load -- No save data found."));
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
				}
			}
		));
}

// ============================================================
// Binary Save / Load
// ============================================================

void UHorizonCloudSaveManager::SaveBytes(const TArray<uint8>& Data, FOnRequestComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::SaveBytes -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString Endpoint = FString::Printf(TEXT("api/v1/app/cloud-save/save?userId=%s"), *UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostBinary(Endpoint, Data, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::SaveBytes -- Binary data saved successfully."));
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::SaveBytes -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonCloudSaveManager::LoadBytes(FOnBinaryComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadBytes -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TArray<uint8>());
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString Endpoint = FString::Printf(TEXT("api/v1/app/cloud-save/load?userId=%s"), *UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnBinaryComplete CapturedOnComplete = OnComplete;

	HttpClient->GetBinary(Endpoint, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.StatusCode == 204)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::LoadBytes -- No binary save data found."));
					CapturedOnComplete.ExecuteIfBound(false, TArray<uint8>());
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::LoadBytes -- Binary data loaded (%d bytes)."), Response.BinaryData.Num());
					CapturedOnComplete.ExecuteIfBound(true, Response.BinaryData);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadBytes -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<uint8>());
				}
			}
		));
}
