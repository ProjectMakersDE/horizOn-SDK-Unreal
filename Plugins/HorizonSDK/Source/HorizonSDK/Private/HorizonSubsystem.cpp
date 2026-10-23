// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "HorizonSubsystem.h"
#include "Managers/HorizonRealtimeManager.h"
#include "Managers/HorizonInboxManager.h"
#include "Managers/HorizonAchievementsManager.h"
#include "Managers/HorizonFriendsManager.h"
#include "Managers/HorizonGroupsManager.h"
#include "Managers/HorizonItemsManager.h"
#include "Managers/HorizonMarketplaceManager.h"
#include "Managers/HorizonSharedObjectsManager.h"
#include "Managers/HorizonMatchesManager.h"
#include "Managers/HorizonChatManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "HorizonSDKModule.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonCloudSaveManager.h"
#include "Managers/HorizonLeaderboardManager.h"
#include "Managers/HorizonRemoteConfigManager.h"
#include "Managers/HorizonLocalizationManager.h"
#include "Managers/HorizonNewsManager.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Managers/HorizonFeedbackManager.h"
#include "Managers/HorizonUserLogManager.h"
#include "Managers/HorizonCrashManager.h"
#include "Managers/HorizonEmailSendingManager.h"
#include "Managers/HorizonPlayerProfileManager.h"
#include "Managers/HorizonValidatedActionsManager.h"
#include "Managers/HorizonStatisticsManager.h"

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

	Realtime = NewObject<UHorizonRealtimeManager>(this);
	Realtime->Initialize(HttpClient, Auth);
    Chat=NewObject<UHorizonChatManager>(this);Chat->Initialize(HttpClient,Auth,Realtime);
	PlayerValues = NewObject<UHorizonPlayerValuesManager>(this);
	PlayerValues->Initialize(HttpClient, Auth);
    Inbox = NewObject<UHorizonInboxManager>(this);
    Inbox->Initialize(HttpClient, Auth, PlayerValues);
    Achievements = NewObject<UHorizonAchievementsManager>(this);
    Achievements->Initialize(HttpClient, Auth, PlayerValues);
    Friends = NewObject<UHorizonFriendsManager>(this);
    Friends->Initialize(HttpClient, Auth);
    Groups = NewObject<UHorizonGroupsManager>(this);
    Groups->Initialize(HttpClient, Auth);
    Items = NewObject<UHorizonItemsManager>(this);
    Items->Initialize(HttpClient, Auth);
    Marketplace=NewObject<UHorizonMarketplaceManager>(this);Marketplace->Initialize(HttpClient,Auth);
    SharedObjects=NewObject<UHorizonSharedObjectsManager>(this);SharedObjects->Initialize(HttpClient,Auth);

	// Create all managers
	CloudSave = NewObject<UHorizonCloudSaveManager>(this);
	CloudSave->Initialize(HttpClient, Auth);

	Leaderboard = NewObject<UHorizonLeaderboardManager>(this);
	Leaderboard->Initialize(HttpClient, Auth);

	RemoteConfig = NewObject<UHorizonRemoteConfigManager>(this);
	RemoteConfig->Initialize(HttpClient);

	Localization = NewObject<UHorizonLocalizationManager>(this);
	Localization->Initialize(HttpClient);

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

	EmailSending = NewObject<UHorizonEmailSendingManager>(this);
	EmailSending->Initialize(HttpClient);

	PlayerProfile = NewObject<UHorizonPlayerProfileManager>(this);
	PlayerProfile->Initialize(HttpClient, Auth);

	// A gift code redeem that grants unlocks clears the cached player profile.
	GiftCodes->SetPlayerProfileManager(PlayerProfile);
	GiftCodes->SetPlayerValuesManager(PlayerValues);

	ValidatedActions = NewObject<UHorizonValidatedActionsManager>(this);
	ValidatedActions->Initialize(HttpClient, Auth);
    Matches=NewObject<UHorizonMatchesManager>(this);Matches->Initialize(HttpClient,Auth,ValidatedActions);Matches->SetRealtime(Realtime);

	Statistics = NewObject<UHorizonStatisticsManager>(this);
	Statistics->Initialize(HttpClient, Auth, PlayerValues, Achievements);

	// An accepted validated run with a board clears the leaderboard cache (like SubmitScore).
	ValidatedActions->SetLeaderboardManager(Leaderboard);

	// Wire auto-breadcrumb handlers
	Auth->OnUserSignedIn.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedIn);
	Auth->OnUserSignedOut.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedOut);
	OnConnected.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnected);
	OnConnectionFailed.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnectionFailed);

	UE_LOG(LogHorizonSDK, Log, TEXT("horizOn SDK initialized."));
}

void UHorizonSubsystem::Deinitialize()
{
	if (Realtime) Realtime->Close();
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
