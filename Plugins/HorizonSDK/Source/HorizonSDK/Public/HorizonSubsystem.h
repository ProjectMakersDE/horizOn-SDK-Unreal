// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HorizonTypes.h"
#include "HorizonSubsystem.generated.h"

class UHorizonRealtimeManager;
class UHorizonInboxManager;
class UHorizonAchievementsManager;
class UHorizonFriendsManager;
class UHorizonGroupsManager;
class UHorizonItemsManager;
class UHorizonMarketplaceManager;
class UHorizonSharedObjectsManager;
class UHorizonMatchesManager;
class UHorizonChatManager;
class UHorizonPlayerValuesManager;
class UHorizonHttpClient;
class UHorizonAuthManager;
class UHorizonCloudSaveManager;
class UHorizonLeaderboardManager;
class UHorizonRemoteConfigManager;
class UHorizonLocalizationManager;
class UHorizonNewsManager;
class UHorizonGiftCodeManager;
class UHorizonFeedbackManager;
class UHorizonUserLogManager;
class UHorizonCrashManager;
class UHorizonEmailSendingManager;
class UHorizonPlayerProfileManager;
class UHorizonValidatedActionsManager;
class UHorizonStatisticsManager;

/**
 * Main entry point for the horizOn SDK.
 *
 * Access via UGameInstance::GetSubsystem<UHorizonSubsystem>() or from
 * Blueprints through the "Get HorizonSubsystem" node.
 *
 * The subsystem owns all manager objects (Auth, CloudSave, Leaderboard, etc.)
 * and the shared HTTP client.  Call ConnectToServer() to begin a session.
 */
UCLASS()
class HORIZONSDK_API UHorizonSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Subsystem lifecycle ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// --- Manager accessors ---

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonAuthManager* Auth;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonCloudSaveManager* CloudSave;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonLeaderboardManager* Leaderboard;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonRemoteConfigManager* RemoteConfig;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonLocalizationManager* Localization;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonNewsManager* News;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonGiftCodeManager* GiftCodes;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonFeedbackManager* Feedback;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonUserLogManager* UserLogs;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonCrashManager* Crashes = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonEmailSendingManager* EmailSending;

	/** Avatar, frame, badges and cosmetic unlocks of the signed-in player (TASK-881). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonPlayerProfileManager* PlayerProfile = nullptr;

	/** Server-checked runs: tickets with a seed, validated submit with rule codes (TASK-883). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonValidatedActionsManager* ValidatedActions = nullptr;

	/** Explicit atomic session statistics; no automatic sign-in fetch. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonStatisticsManager* Statistics = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonRealtimeManager* Realtime = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonPlayerValuesManager* PlayerValues = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonInboxManager* Inbox = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonAchievementsManager* Achievements = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonFriendsManager* Friends = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Managers")
    UHorizonGroupsManager* Groups = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="horizOn|Managers")
    UHorizonItemsManager* Items = nullptr;
    UPROPERTY(BlueprintReadOnly,Category="horizOn") UHorizonMarketplaceManager* Marketplace=nullptr;
    UPROPERTY(BlueprintReadOnly,Category="horizOn") UHorizonSharedObjectsManager* SharedObjects=nullptr;
    UPROPERTY(BlueprintReadOnly,Category="horizOn") UHorizonMatchesManager* Matches=nullptr;
    UPROPERTY(BlueprintReadOnly,Category="horizOn") UHorizonChatManager* Chat=nullptr;

	// --- Connection ---

	/** Connect to the horizOn backend (ping-based host selection). */
	UFUNCTION(BlueprintCallable, Category = "horizOn")
	void ConnectToServer();

	/** Disconnect from the backend and clear session state. */
	UFUNCTION(BlueprintCallable, Category = "horizOn")
	void Disconnect();

	/** Returns true if currently connected to the backend. */
	UFUNCTION(BlueprintPure, Category = "horizOn")
	bool IsConnected() const;

	/** Returns the detailed connection status. */
	UFUNCTION(BlueprintPure, Category = "horizOn")
	EHorizonConnectionStatus GetConnectionStatus() const;

	/** Internal -- get the shared HTTP client. */
	UHorizonHttpClient* GetHttpClient() const { return HttpClient; }

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "horizOn|Events")
	FOnHorizonConnected OnConnected;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|Events")
	FOnHorizonConnectionFailed OnConnectionFailed;

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;
};
