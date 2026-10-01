// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonValidatedActions.h"
#include "HorizonValidatedActionsManager.generated.h"

class UHorizonLeaderboardManager;

/**
 * Completion of StartRun.
 * On failure ErrorCode is the server `code` (for example "RUN_RATE_LIMITED"),
 * "SESSION_REQUIRED" for the local session check, "NOT_SUPPORTED" for a backend without
 * the feature (404 without code), or the HTTP mapping ("RATE_LIMITED", "CONNECTION_FAILED",
 * ...) when the server sent no code. Both strings are empty on success.
 */
DECLARE_DELEGATE_FourParams(FOnValidatedRunStarted, bool /*bSuccess*/, const FHorizonValidatedRun& /*Run*/, const FString& /*ErrorCode*/, const FString& /*ErrorMessage*/);

/**
 * Completion of SubmitValidated / SubmitValidatedWithHash.
 * On failure ErrorCode is the server `code` (rule rejections such as "DURATION_TOO_SHORT",
 * ticket codes such as "TICKET_EXPIRED", "SCORE_LIMIT_REACHED", "PLAYER_BANNED", ...), a local code
 * ("SESSION_REQUIRED", "NO_ACTIVE_RUN", "INVALID_INPUT_LOG_HASH"), "NOT_SUPPORTED", or the
 * HTTP mapping. Both strings are empty on success.
 */
DECLARE_DELEGATE_FourParams(FOnValidatedSubmitComplete, bool /*bSuccess*/, const FHorizonValidatedSubmitResult& /*Result*/, const FString& /*ErrorCode*/, const FString& /*ErrorMessage*/);

/**
 * Completion of GetState (Part 2).
 * On failure ErrorCode is the server `code` ("SESSION_FORBIDDEN", "PLAYER_NOT_FOUND"),
 * "SESSION_REQUIRED" (local check or server), "NOT_SUPPORTED" for a backend without the
 * feature, or the HTTP mapping. Both strings are empty on success.
 */
DECLARE_DELEGATE_FourParams(FOnPlayerStateLoaded, bool /*bSuccess*/, const FHorizonPlayerState& /*State*/, const FString& /*ErrorCode*/, const FString& /*ErrorMessage*/);

/**
 * Completion of UploadEvidence (Part 3).
 * On failure ErrorCode is the server `code` ("EVIDENCE_HASH_MISMATCH", "EVIDENCE_EXPIRED",
 * "EVIDENCE_ALREADY_UPLOADED", "EVIDENCE_NOT_REQUESTED", "EVIDENCE_TOO_LARGE",
 * "EVIDENCE_INVALID_ENCODING", "SESSION_FORBIDDEN", ...), a local code ("SESSION_REQUIRED",
 * "INVALID_RUN_ID", "EMPTY_INPUT_LOG", "EVIDENCE_TOO_LARGE" against evidence.maxBytes), "NOT_SUPPORTED", or the HTTP
 * mapping ("CONNECTION_FAILED", ...). Both strings are empty on success.
 * UHorizonValidatedActionsManager::IsEvidenceUploadRetryable(ErrorCode) tells whether sending
 * the log again can help.
 */
DECLARE_DELEGATE_FourParams(FOnEvidenceUploaded, bool /*bSuccess*/, const FHorizonEvidenceUploadResult& /*Result*/, const FString& /*ErrorCode*/, const FString& /*ErrorMessage*/);

/** Fired after every successful evidence upload (automatic or UploadEvidence). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHorizonEvidenceUploaded, const FString&, RunId, int32, Bytes);

/**
 * Fired after every failed evidence upload (automatic or UploadEvidence), also for local
 * checks. The submit result is not affected: the run was accepted.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHorizonEvidenceUploadFailed, const FString&, RunId, const FString&, ErrorCode, const FString&, ErrorMessage);

/**
 * Fired when the cached player state changes: after GetState, after an accepted run that
 * returned a state, and (with an empty state) on sign-out or when another player signs in.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonPlayerStateChanged, const FHorizonPlayerState&, State);

/**
 * Validated Actions Manager for the horizOn SDK (TASK-883 Part 1, TASK-887 Part 2, TASK-888 Part 3).
 *
 * A run starts with a single use ticket and a server seed (StartRun). The game seeds its
 * deterministic randomness with the seed, records its input log and submits the result
 * with the SHA-256 of that log (SubmitValidated). The server checks the rules of the API
 * key before anything is written and answers with a machine readable `code` on rejection.
 *
 * The manager keeps the started run as the current run. A submit that used up the ticket
 * (accepted, 422 TICKET_* codes, 422 rule and value rejections, 403 SCORE_LIMIT_REACHED)
 * clears it. Checks the server runs before it touches the ticket keep the run, so the game
 * may retry with the same ticket: 404 LEADERBOARD_NOT_FOUND, 422 LEADERBOARD_MISMATCH,
 * 400 SCORE_REQUIRED, 400 PLAYER_NAME_REQUIRED, 403 PLAYER_BANNED (a player banned from the
 * board; the ticket stays usable after an unban, DiscardRun drops it), and also network errors,
 * 401, 429 and 5xx. The run is also cleared on sign-out and when another player signs in.
 *
 * Server-owned state (Part 2): the rules of the API key can define values (currency, loot)
 * that only the server writes. A run earns or spends them through `Earned` of the submit;
 * the accepted result carries the state after the run (Result.State). GetState reads the
 * state at any time. The manager caches the last known state (GetCurrentState()); there is
 * no method that writes it.
 *
 * Evidence (Part 3): the server can ask for the input log of an accepted run
 * (Result.Evidence.bRequired, for example a flagged run or a run in the board's top N). When the
 * run was submitted with the raw bytes (SubmitValidated) and bAutoUploadEvidence is on, the
 * manager uploads the log right away. After SubmitValidatedWithHash the game calls
 * UploadEvidence(Result.Evidence.RunId, Log) itself before Result.Evidence.UploadBefore (24 h).
 * Upload outcomes arrive through OnEvidenceUploaded / OnEvidenceUploadFailed and never change
 * the submit result.
 *
 * Every call needs a signed-in player and sends the player session (Authorization: Bearer).
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonValidatedActionsManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/** Called by UHorizonSubsystem: an accepted run with a board clears the leaderboard cache. */
	void SetLeaderboardManager(UHorizonLeaderboardManager* InLeaderboardManager);

	/**
	 * Start a run: POST /api/v1/app/validated-actions/runs.
	 * On success the run becomes the current run (a previous current run is dropped).
	 * @param LeaderboardKey Board to bind the ticket to; empty for an unbound ticket.
	 * @param OnComplete     Called with (bSuccess, Run, ErrorCode, ErrorMessage).
	 */
	void StartRun(const FString& LeaderboardKey, FOnValidatedRunStarted OnComplete);

	/**
	 * Submit the current run: hashes InputLog (SHA-256) and sends
	 * POST /api/v1/app/validated-actions/submit with the current run's ticket.
	 * @param Score          Score of the run; ignored by the server for a run without board.
	 * @param InputLog       Raw input log bytes of the run (Part 3 uploads the same bytes as evidence).
	 * @param Stage          Stage key for stage rules; empty when none.
	 * @param LeaderboardKey Target board; empty uses the ticket's board.
	 * @param Earned         Earned (positive) or spent (negative) server-owned values, at most 64
	 *                       entries, each key once. Send it only when the rules of the API key define
	 *                       these keys: an unknown key rejects the run (UNKNOWN_VALUE_KEY). Empty when none.
	 * @param OnComplete     Called with (bSuccess, Result, ErrorCode, ErrorMessage). On success
	 *                       Result.State holds the values after the run.
	 */
	void SubmitValidated(int64 Score, const TArray<uint8>& InputLog, const FString& Stage, const FString& LeaderboardKey,
		const TArray<FHorizonEarnedValue>& Earned, FOnValidatedSubmitComplete OnComplete);

	/**
	 * Like SubmitValidated with a ready hash (64 hex characters) instead of the log bytes.
	 * A malformed hash fails locally with INVALID_INPUT_LOG_HASH.
	 */
	void SubmitValidatedWithHash(int64 Score, const FString& InputLogHash, const FString& Stage, const FString& LeaderboardKey,
		const TArray<FHorizonEarnedValue>& Earned, FOnValidatedSubmitComplete OnComplete);

	/**
	 * Read the signed-in player's server-owned values (Part 2):
	 * GET /api/v1/app/validated-actions/state. Every key defined in the rules is listed (balance 0
	 * when never earned), sorted by key. On success the state becomes the cached current state.
	 * @param OnComplete Called with (bSuccess, State, ErrorCode, ErrorMessage).
	 */
	void GetState(FOnPlayerStateLoaded OnComplete);

	/**
	 * Last known server-owned values of the signed-in player: from GetState or from the last
	 * accepted run that returned a state. Empty before the first load and after sign-out.
	 */
	const FHorizonPlayerState& GetCurrentState() const { return CurrentState; }

	/** True when a player state was loaded for the signed-in player. */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	bool HasState() const { return !CurrentState.IsEmpty(); }

	/** Balance of one value from the cached state, 0 when unknown or not loaded. */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	int64 GetBalance(const FString& Key) const { return CurrentState.GetBalance(Key); }

	/** Fired when the cached player state changes (see FOnHorizonPlayerStateChanged). */
	UPROPERTY(BlueprintAssignable, Category = "horizOn|ValidatedActions|Events")
	FOnHorizonPlayerStateChanged OnStateChanged;

	/** SHA-256 of the raw input log as 64 lower case hex characters. */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	static FString ComputeInputLogHash(const TArray<uint8>& InputLog);

	/** The current run (empty when HasActiveRun() is false). */
	const FHorizonValidatedRun& GetCurrentRun() const { return CurrentRun; }

	/** True while a started run waits for its submit. */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	bool HasActiveRun() const { return CurrentRun.IsValid(); }

	/**
	 * Upload the input log of an accepted run as evidence (Part 3):
	 * PUT /api/v1/app/validated-actions/runs/{RunId}/evidence with the log as standard base64.
	 * Only valid after a submit whose result had Evidence.bRequired, within Evidence.UploadBefore.
	 * InputLog must be the exact bytes whose SHA-256 was submitted (else EVIDENCE_HASH_MISMATCH).
	 * The HTTP client retries network errors as for every request; the SDK adds no upload retry
	 * of its own. Send again yourself only when IsEvidenceUploadRetryable(ErrorCode) is true.
	 * When the manager saw the evidence request of RunId, Evidence.MaxBytes is checked locally.
	 * @param RunId      Result.Evidence.RunId (or Result.RunId) of the accepted run.
	 * @param InputLog   Raw input log bytes of that run.
	 * @param OnComplete Called with (bSuccess, Result, ErrorCode, ErrorMessage).
	 */
	void UploadEvidence(const FString& RunId, const TArray<uint8>& InputLog, FOnEvidenceUploaded OnComplete);

	/**
	 * True when a failed upload may be sent again: EVIDENCE_HASH_MISMATCH (with the correct bytes)
	 * and a network error (CONNECTION_FAILED, the Unreal SDK's network error code). Every other
	 * code is final.
	 */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	static bool IsEvidenceUploadRetryable(const FString& ErrorCode);

	/** Error code of the last failed evidence upload (automatic or UploadEvidence); empty after a success. */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	FString GetLastEvidenceErrorCode() const { return LastEvidenceErrorCode; }

	/** Fired after every successful evidence upload. */
	UPROPERTY(BlueprintAssignable, Category = "horizOn|ValidatedActions|Events")
	FOnHorizonEvidenceUploaded OnEvidenceUploaded;

	/** Fired after every failed evidence upload (see FOnHorizonEvidenceUploadFailed). */
	UPROPERTY(BlueprintAssignable, Category = "horizOn|ValidatedActions|Events")
	FOnHorizonEvidenceUploadFailed OnEvidenceUploadFailed;

	/**
	 * Error code of the last failed StartRun, submit or GetState; empty after a success.
	 * Evidence uploads report through GetLastEvidenceErrorCode() instead, so an upload never
	 * changes the code of the submit it belongs to.
	 */
	UFUNCTION(BlueprintPure, Category = "horizOn|ValidatedActions")
	FString GetLastErrorCode() const { return LastErrorCode; }

	/** Drop the current run without submitting it (the ticket expires on the server). */
	UFUNCTION(BlueprintCallable, Category = "horizOn|ValidatedActions")
	void DiscardRun();

	/**
	 * Upload the input log right after a SubmitValidated whose result requests evidence
	 * (Part 3, TASK-888). Off: the game calls UploadEvidence itself. Has no effect after
	 * SubmitValidatedWithHash (the SDK does not know the bytes) and with servers before Part 3,
	 * which never request evidence.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "horizOn|ValidatedActions")
	bool bAutoUploadEvidence = true;

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient = nullptr;

	UPROPERTY()
	UHorizonAuthManager* AuthManager = nullptr;

	UPROPERTY()
	UHorizonLeaderboardManager* LeaderboardManager = nullptr;

	FHorizonValidatedRun CurrentRun;

	/** Player the current run belongs to. */
	FString CurrentRunUserId;

	FString LastErrorCode;

	/** See GetLastEvidenceErrorCode(). */
	FString LastEvidenceErrorCode;

	/** evidence.maxBytes of requested runs, so UploadEvidence checks the size locally too. */
	TMap<FString, int32> EvidenceMaxBytesByRun;

	/** Cached server-owned values (Part 2) and the player they belong to. */
	FHorizonPlayerState CurrentState;
	FString CurrentStateUserId;

	/** Shared submit path; InputLog is set when the caller passed the raw bytes (for Part 3). */
	void SubmitInternal(int64 Score, const FString& InputLogHash, const FString& Stage, const FString& LeaderboardKey,
		const TArray<FHorizonEarnedValue>& Earned, TSharedPtr<const TArray<uint8>> InputLog, FOnValidatedSubmitComplete OnComplete);

	void HandleStartRunResponse(const FHorizonNetworkResponse& Response, const FString& RequestUserId,
		const FOnValidatedRunStarted& OnComplete);

	void HandleGetStateResponse(const FHorizonNetworkResponse& Response, const FString& RequestUserId,
		const FOnPlayerStateLoaded& OnComplete);

	/**
	 * Stores State as the cached state when RequestUserId is still the signed-in player and
	 * fires OnStateChanged. Requested and Credited are reset: they belong to one submit result.
	 */
	void UpdateCurrentState(const FHorizonPlayerState& State, const FString& RequestUserId);

	/** Drops the cached state (sign-out, another player) and fires OnStateChanged when it held one. */
	void ClearCurrentState();

	void HandleSubmitResponse(const FHorizonNetworkResponse& Response, const FString& RequestUserId,
		const FString& SubmittedRunId, const TSharedPtr<const TArray<uint8>>& InputLog,
		const FOnValidatedSubmitComplete& OnComplete);

	/**
	 * Work after an accepted run, before OnComplete: clear the leaderboard cache when a board
	 * was written, cache Result.State (Part 2).
	 */
	void OnRunAccepted(const FHorizonValidatedSubmitResult& Result, const FString& RequestUserId);

	/**
	 * Part 3, after OnComplete delivered the submit result: upload the input log when
	 * Result.Evidence.bRequired, bAutoUploadEvidence and the raw InputLog is known. The run ID is
	 * Result.Evidence.RunId, falling back to Result.RunId; Evidence.MaxBytes is checked locally.
	 */
	void StartEvidenceUpload(const FHorizonValidatedSubmitResult& Result, const FString& RequestUserId,
		const TSharedPtr<const TArray<uint8>>& InputLog);

	/**
	 * Shared upload path. ExpectedUserId (non empty for the automatic upload) must still be the
	 * signed-in player. MaxBytes > 0 checks the size locally. Fires the evidence events.
	 */
	void UploadEvidenceInternal(const FString& RunId, const TArray<uint8>& InputLog, int64 MaxBytes,
		const FString& ExpectedUserId, FOnEvidenceUploaded OnComplete);

	void HandleEvidenceResponse(const FHorizonNetworkResponse& Response, const FString& RunId,
		const FOnEvidenceUploaded& OnComplete);

	/** Records and reports a failed upload (LastEvidenceErrorCode, OnEvidenceUploadFailed, OnComplete). */
	void FailEvidenceUpload(const FString& RunId, const FString& ErrorCode, const FString& ErrorMessage,
		const FOnEvidenceUploaded& OnComplete);

	/** Error code for a failed response (server code, NOT_SUPPORTED for a bare 404, HTTP mapping). */
	static FString MapErrorCode(const FHorizonNetworkResponse& Response);

	UFUNCTION()
	void HandleUserSignedIn();

	UFUNCTION()
	void HandleUserSignedOut();
};
