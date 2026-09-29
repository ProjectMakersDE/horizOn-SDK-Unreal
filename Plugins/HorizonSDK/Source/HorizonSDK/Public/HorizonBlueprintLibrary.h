// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Models/HorizonUserData.h"
#include "Models/HorizonPlayerProfile.h"
#include "Models/HorizonValidatedActions.h"
#include "HorizonBlueprintLibrary.generated.h"

class UHorizonSubsystem;

/**
 * Static Blueprint helpers for common horizOn SDK queries.
 *
 * These functions resolve the subsystem from a world context so that
 * Blueprint users can call them directly without managing object references.
 */
UCLASS()
class HORIZONSDK_API UHorizonBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get the horizOn subsystem from a world context object. */
	UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
	static UHorizonSubsystem* GetHorizonSubsystem(const UObject* WorldContextObject);

	/** Returns true if the SDK is currently connected to the backend. */
	UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
	static bool IsHorizonConnected(const UObject* WorldContextObject);

	/** Returns true if a user is currently signed in. */
	UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
	static bool IsHorizonSignedIn(const UObject* WorldContextObject);

	/** Returns the currently signed-in user data (empty if not signed in). */
	UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
	static FHorizonUserData GetHorizonCurrentUser(const UObject* WorldContextObject);

	/** Start crash capture and register a session. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonStartCrashCapture(const UObject* WorldContextObject);

	/** Stop crash capture. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonStopCrashCapture(const UObject* WorldContextObject);

	/** Record a breadcrumb with a type and message. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonRecordBreadcrumb(const UObject* WorldContextObject, const FString& Type, const FString& Message);

	/** Set a custom key-value pair for crash reports (max 10). */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonSetCrashCustomKey(const UObject* WorldContextObject, const FString& Key, const FString& Value);

	// --- Player Profile (TASK-881) ---

	/** Last loaded or saved player profile result (empty before the first Get/Set Player Profile and after sign-out). */
	UFUNCTION(BlueprintPure, Category = "horizOn|PlayerProfile", meta = (WorldContext = "WorldContextObject"))
	static FHorizonPlayerProfileResult GetHorizonCurrentPlayerProfile(const UObject* WorldContextObject);

	/** Catalog entries of one type ("avatar", "frame" or "badge") from a player profile result. */
	UFUNCTION(BlueprintPure, Category = "horizOn|PlayerProfile")
	static TArray<FHorizonCosmetic> GetHorizonCosmeticsOfType(const FHorizonPlayerProfileResult& Result, const FString& Type);

	/** True when the catalog contains the cosmetic and the player may select it now. */
	UFUNCTION(BlueprintPure, Category = "horizOn|PlayerProfile")
	static bool IsHorizonCosmeticAvailable(const FHorizonPlayerProfileResult& Result, const FString& CosmeticId);

	/** Cosmetic IDs unlocked by the last successful "Redeem Gift Code" (empty when the code had no grants). */
	UFUNCTION(BlueprintPure, Category = "horizOn|GiftCode", meta = (WorldContext = "WorldContextObject"))
	static TArray<FString> GetHorizonLastGrantedUnlocks(const UObject* WorldContextObject);

	// --- Validated Actions (TASK-883) ---

	/** The current validated run (empty before "Start Validated Run" and after its submit, a discard or sign-out). */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions", meta = (WorldContext = "WorldContextObject"))
	static FHorizonValidatedRun GetHorizonCurrentValidatedRun(const UObject* WorldContextObject);
};
