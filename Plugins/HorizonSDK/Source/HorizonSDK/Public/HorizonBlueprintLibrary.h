// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Models/HorizonUserData.h"
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
};
