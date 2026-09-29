// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonPlayerProfile.generated.h"

class FJsonObject;

/**
 * The visible part of a player profile (TASK-881): what leaderboards show next
 * to name and score. IDs are references into the game's own assets; the server
 * stores no images. Treat IDs the game does not know as "not set".
 *
 * JSON `null` values map to an empty string / empty array.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerProfile
{
	GENERATED_BODY()

	/** Selected avatar, empty when not set. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	FString AvatarId;

	/** Selected frame, empty when not set. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	FString FrameId;

	/** Displayed badges (0 to 3), order kept. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	TArray<FString> Badges;

	bool HasAvatar() const { return !AvatarId.IsEmpty(); }
	bool HasFrame() const { return !FrameId.IsEmpty(); }

	/** Null safe parse of `{"avatarId", "frameId", "badges"}`. */
	static FHorizonPlayerProfile FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

/**
 * One entry of the cosmetics catalog of the API key.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonCosmetic
{
	GENERATED_BODY()

	/** Cosmetic ID, for example "avatar.zombie_07". */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	FString Id;

	/** "avatar", "frame" or "badge". */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	FString Type;

	/** True when the cosmetic needs an unlock. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	bool bLocked = false;

	/** True when the player may select it now (free or unlocked). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	bool bAvailable = false;

	static FHorizonCosmetic FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

/**
 * Result of GetProfile and SetProfile: the player's profile, unlocks and the
 * full catalog of the API key with an availability flag per entry.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerProfileResult
{
	GENERATED_BODY()

	/** The player. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	FString UserId;

	/** Current selection. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	FHorizonPlayerProfile Profile;

	/** Owned locked cosmetics (may contain IDs deleted from the catalog). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	TArray<FString> Unlocks;

	/** Catalog of the API key, sorted by Id. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	TArray<FHorizonCosmetic> Cosmetics;

	/** Badges a player may show at once (3). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	int32 MaxBadges = 3;

	/** Unlocks a player may hold (25). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|PlayerProfile")
	int32 MaxUnlocks = 25;

	/** Catalog entries of one type ("avatar", "frame" or "badge"). */
	TArray<FHorizonCosmetic> GetCosmetics(const FString& Type) const;

	/** True when the catalog contains the ID and the player may select it. */
	bool IsAvailable(const FString& CosmeticId) const;

	/** Null safe parse of the PlayerProfileResponse JSON body. */
	static FHorizonPlayerProfileResult FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
