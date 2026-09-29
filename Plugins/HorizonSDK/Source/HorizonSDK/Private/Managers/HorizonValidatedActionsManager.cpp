// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonValidatedActionsManager.h"
#include "HorizonSDKModule.h"
#include "Managers/HorizonLeaderboardManager.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <string>
#include <vector>

// ============================================================
// Initialization
// ============================================================

void UHorizonValidatedActionsManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;

	// A run belongs to one player: drop it on sign-out and when another player signs in.
	if (AuthManager)
	{
		AuthManager->OnUserSignedIn.AddUniqueDynamic(this, &UHorizonValidatedActionsManager::HandleUserSignedIn);
		AuthManager->OnUserSignedOut.AddUniqueDynamic(this, &UHorizonValidatedActionsManager::HandleUserSignedOut);
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonValidatedActionsManager initialized."));
}

void UHorizonValidatedActionsManager::SetLeaderboardManager(UHorizonLeaderboardManager* InLeaderboardManager)
{
	LeaderboardManager = InLeaderboardManager;
}

// ============================================================
// Hash helper
// ============================================================

FString UHorizonValidatedActionsManager::ComputeInputLogHash(const TArray<uint8>& InputLog)
{
	const std::string Hash = HorizonTransportContract::Sha256Hex(
		InputLog.Num() > 0 ? InputLog.GetData() : nullptr,
		static_cast<std::size_t>(InputLog.Num()));
	return UTF8_TO_TCHAR(Hash.c_str());
}

// ============================================================
// Start Run
// ============================================================

void UHorizonValidatedActionsManager::StartRun(const FString& LeaderboardKey, FOnValidatedRunStarted OnComplete)
{
	if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn())
	{
		LastErrorCode = TEXT("SESSION_REQUIRED");
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::StartRun -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, FHorizonValidatedRun(), LastErrorCode, TEXT("A signed-in player is required."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const HorizonTransportContract::FValidatedRequestPlan Plan =
		HorizonTransportContract::BuildValidatedStartRunPlan(
			TCHAR_TO_UTF8(*UserId),
			TCHAR_TO_UTF8(*HttpClient->GetSessionToken()),
			TCHAR_TO_UTF8(*LeaderboardKey));
	if (!Plan.bShouldSend)
	{
		LastErrorCode = UTF8_TO_TCHAR(Plan.ErrorCode.c_str());
		const FString ErrorMessage = UTF8_TO_TCHAR(Plan.ErrorMessage.c_str());
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::StartRun -- %s"), *ErrorMessage);
		OnComplete.ExecuteIfBound(false, FHorizonValidatedRun(), LastErrorCode, ErrorMessage);
		return;
	}

	TSharedPtr<FJsonObject> ParsedBody;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Plan.BodyJson.c_str()));
	if (!FJsonSerializer::Deserialize(Reader, ParsedBody) || !ParsedBody.IsValid())
	{
		LastErrorCode = TEXT("INVALID_REQUEST");
		OnComplete.ExecuteIfBound(false, FHorizonValidatedRun(), LastErrorCode, TEXT("Failed to build start run request."));
		return;
	}

	TWeakObjectPtr<UHorizonValidatedActionsManager> WeakSelf(this);
	FOnValidatedRunStarted CapturedOnComplete = OnComplete;
	const FString Endpoint = UTF8_TO_TCHAR(Plan.Endpoint.c_str());

	HttpClient->PostJson(ParsedBody.ToSharedRef(), Endpoint, Plan.bUseSessionToken,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, UserId](const FHorizonNetworkResponse& Response)
			{
				UHorizonValidatedActionsManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}
				Self->HandleStartRunResponse(Response, UserId, CapturedOnComplete);
			}
		));
}

void UHorizonValidatedActionsManager::HandleStartRunResponse(const FHorizonNetworkResponse& Response,
	const FString& RequestUserId, const FOnValidatedRunStarted& OnComplete)
{
	if (!Response.bSuccess)
	{
		LastErrorCode = MapErrorCode(Response);
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::StartRun -- Failed (%s): %s"), *LastErrorCode, *Response.ErrorMessage);
		OnComplete.ExecuteIfBound(false, FHorizonValidatedRun(), LastErrorCode, Response.ErrorMessage);
		return;
	}

	const FHorizonValidatedRun Run = FHorizonValidatedRun::FromJson(Response.JsonData);
	if (!Run.IsValid())
	{
		LastErrorCode = TEXT("INVALID_RESPONSE");
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::StartRun -- Response has no ticket."));
		OnComplete.ExecuteIfBound(false, FHorizonValidatedRun(), LastErrorCode, TEXT("The server response could not be read."));
		return;
	}

	// Keep the run only when the same player is still signed in (a sign-out may have happened meanwhile).
	if (AuthManager && AuthManager->IsSignedIn() && AuthManager->GetCurrentUser().UserId == RequestUserId)
	{
		CurrentRun = Run;
		CurrentRunUserId = RequestUserId;
	}

	LastErrorCode.Empty();
	UE_LOG(LogHorizonSDK, Log, TEXT("ValidatedActions::StartRun -- Run %s started (seed %d, board '%s', expires %s)."),
		*Run.RunId, Run.Seed, *Run.LeaderboardKey, *Run.ExpiresAt);
	OnComplete.ExecuteIfBound(true, Run, FString(), FString());
}

// ============================================================
// Submit
// ============================================================

void UHorizonValidatedActionsManager::SubmitValidated(int64 Score, const TArray<uint8>& InputLog, const FString& Stage,
	const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned, FOnValidatedSubmitComplete OnComplete)
{
	// Keep the bytes for the evidence upload of Part 3 (logs are small, at most a few KiB).
	const TSharedPtr<const TArray<uint8>> InputLogCopy = MakeShared<TArray<uint8>>(InputLog);
	SubmitInternal(Score, ComputeInputLogHash(InputLog), Stage, LeaderboardKey, Earned, InputLogCopy, OnComplete);
}

void UHorizonValidatedActionsManager::SubmitValidatedWithHash(int64 Score, const FString& InputLogHash, const FString& Stage,
	const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned, FOnValidatedSubmitComplete OnComplete)
{
	SubmitInternal(Score, InputLogHash, Stage, LeaderboardKey, Earned, nullptr, OnComplete);
}

void UHorizonValidatedActionsManager::SubmitInternal(int64 Score, const FString& InputLogHash, const FString& Stage,
	const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned, TSharedPtr<const TArray<uint8>> InputLog,
	FOnValidatedSubmitComplete OnComplete)
{
	if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn())
	{
		LastErrorCode = TEXT("SESSION_REQUIRED");
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::SubmitValidated -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, FHorizonValidatedSubmitResult(), LastErrorCode, TEXT("A signed-in player is required."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	// A run of another player is never sent (the run is cleared on sign-in of someone else).
	const FString Ticket = (CurrentRun.IsValid() && CurrentRunUserId == UserId) ? CurrentRun.Ticket : FString();
	const FString SubmittedRunId = CurrentRun.RunId;

	std::vector<HorizonTransportContract::FValidatedEarnedValue> EarnedValues;
	EarnedValues.reserve(Earned.Num());
	for (const FHorizonEarnedValue& Value : Earned)
	{
		HorizonTransportContract::FValidatedEarnedValue Entry;
		Entry.Key = TCHAR_TO_UTF8(*Value.Key);
		Entry.Amount = Value.Amount;
		EarnedValues.push_back(Entry);
	}

	const HorizonTransportContract::FValidatedRequestPlan Plan =
		HorizonTransportContract::BuildValidatedSubmitPlan(
			TCHAR_TO_UTF8(*UserId),
			TCHAR_TO_UTF8(*HttpClient->GetSessionToken()),
			TCHAR_TO_UTF8(*Ticket),
			TCHAR_TO_UTF8(*InputLogHash),
			Score,
			TCHAR_TO_UTF8(*Stage),
			TCHAR_TO_UTF8(*LeaderboardKey),
			EarnedValues);
	if (!Plan.bShouldSend)
	{
		LastErrorCode = UTF8_TO_TCHAR(Plan.ErrorCode.c_str());
		const FString ErrorMessage = UTF8_TO_TCHAR(Plan.ErrorMessage.c_str());
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::SubmitValidated -- %s"), *ErrorMessage);
		OnComplete.ExecuteIfBound(false, FHorizonValidatedSubmitResult(), LastErrorCode, ErrorMessage);
		return;
	}

	TSharedPtr<FJsonObject> ParsedBody;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Plan.BodyJson.c_str()));
	if (!FJsonSerializer::Deserialize(Reader, ParsedBody) || !ParsedBody.IsValid())
	{
		LastErrorCode = TEXT("INVALID_REQUEST");
		OnComplete.ExecuteIfBound(false, FHorizonValidatedSubmitResult(), LastErrorCode, TEXT("Failed to build submit request."));
		return;
	}

	TWeakObjectPtr<UHorizonValidatedActionsManager> WeakSelf(this);
	FOnValidatedSubmitComplete CapturedOnComplete = OnComplete;
	const FString Endpoint = UTF8_TO_TCHAR(Plan.Endpoint.c_str());

	HttpClient->PostJson(ParsedBody.ToSharedRef(), Endpoint, Plan.bUseSessionToken,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, UserId, SubmittedRunId, InputLog](const FHorizonNetworkResponse& Response)
			{
				UHorizonValidatedActionsManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}
				Self->HandleSubmitResponse(Response, UserId, SubmittedRunId, InputLog, CapturedOnComplete);
			}
		));
}

void UHorizonValidatedActionsManager::HandleSubmitResponse(const FHorizonNetworkResponse& Response, const FString& RequestUserId,
	const FString& SubmittedRunId, const TSharedPtr<const TArray<uint8>>& InputLog, const FOnValidatedSubmitComplete& OnComplete)
{
	// A ticket is single use: clear the run once the server used it up. Only the submitted run
	// is cleared (StartRun may have replaced it while the request was in flight).
	if (HorizonTransportContract::ShouldClearRunAfterSubmit(Response.StatusCode, TCHAR_TO_UTF8(*Response.ServerErrorCode))
		&& CurrentRun.RunId == SubmittedRunId && CurrentRunUserId == RequestUserId)
	{
		DiscardRun();
	}

	if (!Response.bSuccess)
	{
		LastErrorCode = MapErrorCode(Response);
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::SubmitValidated -- Run %s rejected (%s): %s"),
			*SubmittedRunId, *LastErrorCode, *Response.ErrorMessage);
		OnComplete.ExecuteIfBound(false, FHorizonValidatedSubmitResult(), LastErrorCode, Response.ErrorMessage);
		return;
	}

	if (!Response.JsonData.IsValid())
	{
		LastErrorCode = TEXT("INVALID_RESPONSE");
		UE_LOG(LogHorizonSDK, Warning, TEXT("ValidatedActions::SubmitValidated -- Response has no JSON body."));
		OnComplete.ExecuteIfBound(false, FHorizonValidatedSubmitResult(), LastErrorCode, TEXT("The server response could not be read."));
		return;
	}

	const FHorizonValidatedSubmitResult Result = FHorizonValidatedSubmitResult::FromJson(Response.JsonData);
	LastErrorCode.Empty();
	OnRunAccepted(Result, InputLog);

	UE_LOG(LogHorizonSDK, Log, TEXT("ValidatedActions::SubmitValidated -- Run %s accepted (board '%s', score %lld, best %lld, rank %lld, %lld s)."),
		*Result.RunId, *Result.LeaderboardKey, Result.Score, Result.BestScore, Result.Rank, Result.DurationSeconds);
	OnComplete.ExecuteIfBound(true, Result, FString(), FString());
}

void UHorizonValidatedActionsManager::OnRunAccepted(const FHorizonValidatedSubmitResult& Result,
	const TSharedPtr<const TArray<uint8>>& /*InputLog*/)
{
	// Same as after SubmitScore: cached top, around and rank lists are stale now.
	if (Result.HasLeaderboard() && LeaderboardManager)
	{
		LeaderboardManager->ClearCache();
	}

	// Part 3 (TASK-888): when Result.Evidence.bRequired and bAutoUploadEvidence and the raw
	// InputLog is known, upload it here (PUT /api/v1/app/validated-actions/runs/{runId}/evidence).
}

// ============================================================
// Run state
// ============================================================

void UHorizonValidatedActionsManager::DiscardRun()
{
	CurrentRun = FHorizonValidatedRun();
	CurrentRunUserId.Empty();
}

void UHorizonValidatedActionsManager::HandleUserSignedIn()
{
	// A session restore of the same player keeps the run; another player drops it.
	if (!AuthManager || AuthManager->GetCurrentUser().UserId != CurrentRunUserId)
	{
		DiscardRun();
	}
}

void UHorizonValidatedActionsManager::HandleUserSignedOut()
{
	DiscardRun();
}

// ============================================================
// Helpers
// ============================================================

FString UHorizonValidatedActionsManager::MapErrorCode(const FHorizonNetworkResponse& Response)
{
	const std::string Code = HorizonTransportContract::MapValidatedErrorCode(
		Response.StatusCode,
		TCHAR_TO_UTF8(*Response.ServerErrorCode),
		TCHAR_TO_UTF8(*Response.GetErrorCodeString()));
	return UTF8_TO_TCHAR(Code.c_str());
}
