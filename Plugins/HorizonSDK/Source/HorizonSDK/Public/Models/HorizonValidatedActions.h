// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonValidatedActions.generated.h"

class FJsonObject;

/**
 * Validated Actions models (TASK-883). Field names follow the JSON of
 * docs/features/validated-actions/API-ENDPOINTS.md. Parsing is null safe:
 * JSON `null` or a missing field gives an empty string, 0, false or an empty struct.
 *
 * Part 1 fills FHorizonValidatedRun and FHorizonValidatedSubmitResult.
 * Part 2 (TASK-887) fills FHorizonPlayerState: from GET .../state and as
 * FHorizonValidatedSubmitResult::State. FHorizonEvidenceRequest (Part 3, TASK-888)
 * stays empty until then.
 */

/**
 * A started run: the single use ticket and the server seed.
 * Seed the game's deterministic randomness with Seed and record the input log.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonValidatedRun
{
	GENERATED_BODY()

	/** Ticket ID. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString RunId;

	/** Opaque token, sent unchanged with the submit. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString Ticket;

	/** Server seed for the run (0 to 2,147,483,646). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int32 Seed = 0;

	/** Board the ticket is bound to; empty for an unbound ticket. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString LeaderboardKey;

	/** Issue time (ISO 8601 UTC). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString IssuedAt;

	/** Expiry time (ISO 8601 UTC). The server decides; an expired run is still sent. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString ExpiresAt;

	/** Lifetime at issue in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int32 ExpiresInSeconds = 0;

	/** True when the struct holds a ticket. */
	bool IsValid() const { return !Ticket.IsEmpty(); }

	/** Null safe parse of the start run response. */
	static FHorizonValidatedRun FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

/**
 * One earned (positive) or spent (negative) value of a run. Part 1 SDKs already
 * send it; Part 1 servers ignore it, Part 2 servers apply it.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonEarnedValue
{
	GENERATED_BODY()

	/** Value key, pattern ^[a-z0-9][a-z0-9._-]{0,23}$ (for example "gold"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "horizOn|ValidatedActions")
	FString Key;

	/** Amount; negative spends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "horizOn|ValidatedActions")
	int64 Amount = 0;

	FHorizonEarnedValue() = default;
	FHorizonEarnedValue(const FString& InKey, int64 InAmount) : Key(InKey), Amount(InAmount) {}
};

/**
 * One server-owned value of a player (Part 2): a currency or loot counter that only the
 * server writes. All numbers are int64 and at most 9,007,199,254,740,991.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerStateValue
{
	GENERATED_BODY()

	/** Value key as defined in the rules of the API key (for example "gold"). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString Key;

	/** Current balance (0 when never earned). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 Balance = 0;

	/** Positive credit on the current UTC day (FHorizonPlayerState::Day), 0 after midnight UTC. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 EarnedToday = 0;

	/** Daily cap of positive credit, 0 when there is none (the server sends null). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 DailyCap = 0;

	/**
	 * Amount the run sent in `earned` (submit results only, for values the run touched;
	 * 0 otherwise and in GetState).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 Requested = 0;

	/**
	 * Amount actually applied (submit results only, 0 otherwise). Lower than Requested for a
	 * positive amount clamped by the daily cap or maxBalance. A spend is either the full
	 * negative amount or 0 (a concurrent run used the balance first).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 Credited = 0;

	/** True when the run's amount was applied in full. Grant a purchase only when this is true. */
	bool IsFullyCredited() const { return Credited == Requested; }

	static FHorizonPlayerStateValue FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

/**
 * Server-owned values of the signed-in player (Part 2), from GetState or from an accepted
 * submit. Read only for clients: values change only through `earned` of a validated run.
 */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonPlayerState
{
	GENERATED_BODY()

	/** Player the state belongs to (sent by GET .../state only, empty in a submit result). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString UserId;

	/** Current UTC day of EarnedToday (for example "2026-09-29"). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString Day;

	/** Every value defined in the rules, sorted by key. Empty when the rules define no values. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	TArray<FHorizonPlayerStateValue> Values;

	/** True when the server sent no state (a submit with `state: null`, or before the first load). */
	bool IsEmpty() const { return Day.IsEmpty() && Values.Num() == 0; }

	/** Balance of one value, 0 when the key is unknown. */
	int64 GetBalance(const FString& Key) const;

	/** The value with this key, or nullptr when the rules do not define it. */
	const FHorizonPlayerStateValue* FindValue(const FString& Key) const;

	static FHorizonPlayerState FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

/** Evidence request of an accepted run (Part 3). Required is false in Part 1. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonEvidenceRequest
{
	GENERATED_BODY()

	/** True when the input log must be uploaded. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	bool bRequired = false;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString RunId;

	/** Upload deadline (ISO 8601 UTC). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString UploadBefore;

	/** Largest accepted log in bytes. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int32 MaxBytes = 0;

	static FHorizonEvidenceRequest FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

/** Result of an accepted validated run. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonValidatedSubmitResult
{
	GENERATED_BODY()

	/** Always true on success. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	bool bAccepted = false;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString RunId;

	/** Board written, empty for a run without board. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FString LeaderboardKey;

	/** Submitted score (0 for a run without board). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 Score = 0;

	/** The player's row after the write (0 for a run without board). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 BestScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	bool bIsNewHighScore = false;

	/** 1-based rank (0 for a run without board). */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 Rank = 0;

	/** Server-measured run duration in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	int64 DurationSeconds = 0;

	/**
	 * Part 2: server-owned values after the run, touched values with Requested and Credited.
	 * Empty when the rules define no values (the server sends null) and with Part 1 servers.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FHorizonPlayerState State;

	/** Part 3: evidence request. Required is false in Part 1. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|ValidatedActions")
	FHorizonEvidenceRequest Evidence;

	/** True when the run wrote to a leaderboard. */
	bool HasLeaderboard() const { return !LeaderboardKey.IsEmpty(); }

	/** Null safe parse of the submit response. */
	static FHorizonValidatedSubmitResult FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
