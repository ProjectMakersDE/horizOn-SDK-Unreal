// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonValidatedActions.h"
#include "HorizonAsync_ValidatedActions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValidatedRunStartedAsyncSuccess, const FHorizonValidatedRun&, Run);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValidatedSubmitAsyncSuccess, const FHorizonValidatedSubmitResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValidatedStateAsyncSuccess, const FHorizonPlayerState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValidatedEvidenceAsyncSuccess, const FHorizonEvidenceUploadResult&, Result);
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

	/**
	 * Start a run. LeaderboardKey binds the ticket to a board; leave it empty for an unbound ticket.
	 * Sends the manager's DefaultRunContext (nothing when it is empty).
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Start Validated Run"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_StartRun* StartRun(const UObject* WorldContextObject, const FString& LeaderboardKey);

	/**
	 * Start a run with a run start context (versions, content digest, initial state). Context
	 * replaces the manager's DefaultRunContext; an empty Context sends none. Failure codes include
	 * INVALID_CONTENT_DIGEST (local), INITIAL_STATE_TOO_LARGE and INITIAL_STATE_INVALID_ENCODING.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Start Validated Run With Context"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_StartRun* StartRunWithContext(const UObject* WorldContextObject, const FString& LeaderboardKey,
		const FHorizonRunContext& Context);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString LeaderboardKeyStr;
	FHorizonRunContext RunContext;
	bool bHasRunContext = false;

	void HandleResult(bool bSuccess, const FHorizonValidatedRun& Run, const FString& ErrorCode, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node: Submit the current validated run with its input log.
 * The SDK hashes the log (SHA-256) and sends the hash with the run's ticket. When the result
 * requests evidence (Result.Evidence.bRequired) the SDK uploads the log by itself (unless
 * bAutoUploadEvidence is off); the outcome arrives on OnEvidenceUploaded / OnEvidenceUploadFailed.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_SubmitValidated : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnValidatedSubmitAsyncSuccess OnSuccess;

	/** ErrorCode is the rejection code (for example DURATION_TOO_SHORT, TICKET_EXPIRED), PLAYER_BANNED (the run stays), or NO_ACTIVE_RUN, SESSION_REQUIRED, ... */
	UPROPERTY(BlueprintAssignable)
	FOnValidatedActionsAsyncFailure OnFailure;

	/**
	 * Submit the current run. Score is ignored by the server for a run without board.
	 * Stage and LeaderboardKey may stay empty (empty key uses the ticket's board).
	 * Earned: server-owned values the run earned (positive) or spent (negative); only keys the
	 * rules of the API key define (else UNKNOWN_VALUE_KEY). On success Result.State holds the values after the run.
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
 * The SDK does not know the bytes: when Result.Evidence.bRequired is true, call
 * "Upload Validated Run Evidence" with the log before Result.Evidence.UploadBefore.
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

// ============================================================

/**
 * Async Blueprint node: Read the signed-in player's server-owned values (currency, loot).
 * The state also becomes the cached state of the ValidatedActions manager.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_GetValidatedState : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnValidatedStateAsyncSuccess OnSuccess;

	/** ErrorCode is SESSION_REQUIRED, SESSION_FORBIDDEN, PLAYER_NOT_FOUND, NOT_SUPPORTED, CONNECTION_FAILED, ... */
	UPROPERTY(BlueprintAssignable)
	FOnValidatedActionsAsyncFailure OnFailure;

	/** Load the player state (every value defined in the rules, sorted by key). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Get Validated Player State"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_GetValidatedState* GetValidatedState(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;

	void HandleResult(bool bSuccess, const FHorizonPlayerState& State, const FString& ErrorCode, const FString& ErrorMessage);
};

// ============================================================

/**
 * Async Blueprint node: Upload the input log of an accepted run as evidence (Part 3).
 * Use it after "Submit Validated Run With Hash" when Result.Evidence.bRequired is true, or
 * when the automatic upload is off. "Submit Validated Run" uploads by itself.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_UploadEvidence : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnValidatedEvidenceAsyncSuccess OnSuccess;

	/**
	 * ErrorCode is EVIDENCE_HASH_MISMATCH (send the correct bytes again), EVIDENCE_EXPIRED,
	 * EVIDENCE_ALREADY_UPLOADED, EVIDENCE_NOT_REQUESTED, EVIDENCE_TOO_LARGE, EVIDENCE_INVALID_ENCODING,
	 * SESSION_REQUIRED, INVALID_RUN_ID, EMPTY_INPUT_LOG, CONNECTION_FAILED, ... Check
	 * "Is Evidence Upload Retryable" before sending again.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnValidatedActionsAsyncFailure OnFailure;

	/** Upload InputLog (the exact bytes of the submitted hash) for RunId (Result.Evidence.RunId). */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Upload Validated Run Evidence"), Category = "horizOn|ValidatedActions")
	static UHorizonAsync_UploadEvidence* UploadEvidence(const UObject* WorldContextObject, const FString& RunId, const TArray<uint8>& InputLog);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString RunIdStr;
	TArray<uint8> InputLogBytes;

	void HandleResult(bool bSuccess, const FHorizonEvidenceUploadResult& Result, const FString& ErrorCode, const FString& ErrorMessage);
};
