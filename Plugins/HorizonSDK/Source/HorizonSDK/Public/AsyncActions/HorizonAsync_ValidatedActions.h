// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonValidatedActions.h"
#include "HorizonAsync_ValidatedActions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValidatedRunStartedAsyncSuccess, const FHorizonValidatedRun&, Run);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValidatedSubmitAsyncSuccess, const FHorizonValidatedSubmitResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnValidatedActionsAsyncFailure, const FString&, ErrorCode, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Start a validated run (ticket plus server seed).
 * The run becomes the current run of the ValidatedActions manager.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_StartRun : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnValidatedRunStartedAsyncSuccess OnSuccess;

	/** ErrorCode is the server code (for example RUN_RATE_LIMITED) or SESSION_REQUIRED, NOT_SUPPORTED, CONNECTION_FAILED, ... */
	UPROPERTY(BlueprintAssignable)
	FOnValidatedActionsAsyncFailure OnFailure;

	/** Start a run. LeaderboardKey binds the ticket to a board; leave it empty for an unbound ticket. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Start Validated Run"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_StartRun* StartRun(const UObject* WorldContextObject, const FString& LeaderboardKey);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString LeaderboardKeyStr;

	void HandleResult(bool bSuccess, const FHorizonValidatedRun& Run, const FString& ErrorCode, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node: Submit the current validated run with its input log.
 * The SDK hashes the log (SHA-256) and sends the hash with the run's ticket.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_SubmitValidated : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnValidatedSubmitAsyncSuccess OnSuccess;

	/** ErrorCode is the rejection code (for example DURATION_TOO_SHORT, TICKET_EXPIRED) or NO_ACTIVE_RUN, SESSION_REQUIRED, ... */
	UPROPERTY(BlueprintAssignable)
	FOnValidatedActionsAsyncFailure OnFailure;

	/**
	 * Submit the current run. Score is ignored by the server for a run without board.
	 * Stage and LeaderboardKey may stay empty (empty key uses the ticket's board); Earned is for Part 2 servers.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Submit Validated Run", AutoCreateRefTerm = "Earned"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_SubmitValidated* SubmitValidated(const UObject* WorldContextObject, int64 Score, const TArray<uint8>& InputLog,
		const FString& Stage, const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	int64 ScoreValue = 0;
	TArray<uint8> InputLogBytes;
	FString StageStr;
	FString LeaderboardKeyStr;
	TArray<FHorizonEarnedValue> EarnedValues;

	void HandleResult(bool bSuccess, const FHorizonValidatedSubmitResult& Result, const FString& ErrorCode, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node: Submit the current validated run with a ready SHA-256 hash of its input log.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_SubmitValidatedWithHash : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnValidatedSubmitAsyncSuccess OnSuccess;

	/** ErrorCode is the rejection code or INVALID_INPUT_LOG_HASH, NO_ACTIVE_RUN, SESSION_REQUIRED, ... */
	UPROPERTY(BlueprintAssignable)
	FOnValidatedActionsAsyncFailure OnFailure;

	/** Submit the current run with InputLogHash (64 hex characters). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Submit Validated Run With Hash", AutoCreateRefTerm = "Earned"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_SubmitValidatedWithHash* SubmitValidatedWithHash(const UObject* WorldContextObject, int64 Score, const FString& InputLogHash,
		const FString& Stage, const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	int64 ScoreValue = 0;
	FString InputLogHashStr;
	FString StageStr;
	FString LeaderboardKeyStr;
	TArray<FHorizonEarnedValue> EarnedValues;

	void HandleResult(bool bSuccess, const FHorizonValidatedSubmitResult& Result, const FString& ErrorCode, const FString& ErrorMessage);
};
