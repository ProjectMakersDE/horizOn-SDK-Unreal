// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HorizonTypes.h"
#include "HorizonSubsystem.generated.h"

class UHorizonHttpClient;
class UHorizonAuthManager;
class UHorizonCloudSaveManager;
class UHorizonLeaderboardManager;
class UHorizonRemoteConfigManager;
class UHorizonNewsManager;
class UHorizonGiftCodeManager;
class UHorizonFeedbackManager;
class UHorizonUserLogManager;
class UHorizonCrashManager;

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
	UHorizonNewsManager* News;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonGiftCodeManager* GiftCodes;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonFeedbackManager* Feedback;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonUserLogManager* UserLogs;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonCrashManager* Crashes = nullptr;

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
