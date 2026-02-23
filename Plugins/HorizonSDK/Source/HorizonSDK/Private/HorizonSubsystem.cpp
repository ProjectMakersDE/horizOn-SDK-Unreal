// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "HorizonSubsystem.h"
#include "HorizonSDKModule.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonCloudSaveManager.h"
#include "Managers/HorizonLeaderboardManager.h"
#include "Managers/HorizonRemoteConfigManager.h"
#include "Managers/HorizonNewsManager.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Managers/HorizonFeedbackManager.h"
#include "Managers/HorizonUserLogManager.h"
#include "Managers/HorizonCrashManager.h"

// ============================================================
// Subsystem lifecycle
// ============================================================

void UHorizonSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create the shared HTTP client
	HttpClient = NewObject<UHorizonHttpClient>(this);
	HttpClient->Initialize();

	// Create the Auth manager
	Auth = NewObject<UHorizonAuthManager>(this);
	Auth->Initialize(HttpClient);

	// Create all managers
	CloudSave = NewObject<UHorizonCloudSaveManager>(this);
	CloudSave->Initialize(HttpClient, Auth);

	Leaderboard = NewObject<UHorizonLeaderboardManager>(this);
	Leaderboard->Initialize(HttpClient, Auth);

	RemoteConfig = NewObject<UHorizonRemoteConfigManager>(this);
	RemoteConfig->Initialize(HttpClient);

	News = NewObject<UHorizonNewsManager>(this);
	News->Initialize(HttpClient);

	GiftCodes = NewObject<UHorizonGiftCodeManager>(this);
	GiftCodes->Initialize(HttpClient, Auth);

	Feedback = NewObject<UHorizonFeedbackManager>(this);
	Feedback->Initialize(HttpClient, Auth);

	UserLogs = NewObject<UHorizonUserLogManager>(this);
	UserLogs->Initialize(HttpClient, Auth);

	Crashes = NewObject<UHorizonCrashManager>(this);
	Crashes->Initialize(HttpClient, Auth);

	// Wire auto-breadcrumb handlers
	Auth->OnUserSignedIn.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedIn);
	Auth->OnUserSignedOut.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedOut);
	OnConnected.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnected);
	OnConnectionFailed.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnectionFailed);

	UE_LOG(LogHorizonSDK, Log, TEXT("horizOn SDK initialized."));
}

void UHorizonSubsystem::Deinitialize()
{
	if (Crashes)
	{
		Crashes->StopCapture();
	}

	if (IsConnected())
	{
		Disconnect();
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("horizOn SDK shut down."));

	Super::Deinitialize();
}

// ============================================================
// Connection
// ============================================================

void UHorizonSubsystem::ConnectToServer()
{
	if (!HttpClient)
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("ConnectToServer -- HttpClient is null. Was Initialize() called?"));
		OnConnectionFailed.Broadcast(TEXT("HttpClient not initialized."));
		return;
	}

	TWeakObjectPtr<UHorizonSubsystem> WeakSelf(this);

	HttpClient->ConnectToServer(FOnRequestComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const FString& ErrorMessage)
		{
			UHorizonSubsystem* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			if (bSuccess)
			{
				UE_LOG(LogHorizonSDK, Log, TEXT("horizOn connected to server."));
				Self->OnConnected.Broadcast();
			}
			else
			{
				UE_LOG(LogHorizonSDK, Error, TEXT("horizOn connection failed: %s"), *ErrorMessage);
				Self->OnConnectionFailed.Broadcast(ErrorMessage);
			}
		}
	));
}

void UHorizonSubsystem::Disconnect()
{
	if (HttpClient)
	{
		HttpClient->Disconnect();
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("horizOn disconnected."));
}

bool UHorizonSubsystem::IsConnected() const
{
	return HttpClient && HttpClient->IsConnected();
}

EHorizonConnectionStatus UHorizonSubsystem::GetConnectionStatus() const
{
	if (!HttpClient)
	{
		return EHorizonConnectionStatus::Disconnected;
	}
	return HttpClient->GetConnectionStatus();
}
