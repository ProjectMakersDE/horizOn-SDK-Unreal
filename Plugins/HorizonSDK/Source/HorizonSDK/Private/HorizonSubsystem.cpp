// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "HorizonSubsystem.h"
#include "HorizonSDKModule.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"

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

	// Other managers will be created here once implemented (Task 16)
	CloudSave    = nullptr;
	Leaderboard  = nullptr;
	RemoteConfig = nullptr;
	News         = nullptr;
	GiftCodes    = nullptr;
	Feedback     = nullptr;
	UserLogs     = nullptr;

	UE_LOG(LogHorizonSDK, Log, TEXT("horizOn SDK initialized."));
}

void UHorizonSubsystem::Deinitialize()
{
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
