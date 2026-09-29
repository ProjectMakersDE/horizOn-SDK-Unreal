// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_ValidatedActions.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonValidatedActionsManager.h"

// ============================================================
// StartRun
// ============================================================

UHorizonAsync_StartRun* UHorizonAsync_StartRun::StartRun(const UObject* WorldContextObject, const FString& LeaderboardKey)
{
	UHorizonAsync_StartRun* Action = NewObject<UHorizonAsync_StartRun>();
	Action->WorldContext = WorldContextObject;
	Action->LeaderboardKeyStr = LeaderboardKey;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_StartRun::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->ValidatedActions)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or ValidatedActions manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->ValidatedActions->StartRun(
		LeaderboardKeyStr,
		FOnValidatedRunStarted::CreateUObject(this, &UHorizonAsync_StartRun::HandleResult)
	);
}

void UHorizonAsync_StartRun::HandleResult(bool bSuccess, const FHorizonValidatedRun& Run,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Run);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// SubmitValidated
// ============================================================

UHorizonAsync_SubmitValidated* UHorizonAsync_SubmitValidated::SubmitValidated(const UObject* WorldContextObject, int64 Score,
	const TArray<uint8>& InputLog, const FString& Stage, const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned)
{
	UHorizonAsync_SubmitValidated* Action = NewObject<UHorizonAsync_SubmitValidated>();
	Action->WorldContext = WorldContextObject;
	Action->ScoreValue = Score;
	Action->InputLogBytes = InputLog;
	Action->StageStr = Stage;
	Action->LeaderboardKeyStr = LeaderboardKey;
	Action->EarnedValues = Earned;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_SubmitValidated::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->ValidatedActions)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or ValidatedActions manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->ValidatedActions->SubmitValidated(
		ScoreValue, InputLogBytes, StageStr, LeaderboardKeyStr, EarnedValues,
		FOnValidatedSubmitComplete::CreateUObject(this, &UHorizonAsync_SubmitValidated::HandleResult)
	);
}

void UHorizonAsync_SubmitValidated::HandleResult(bool bSuccess, const FHorizonValidatedSubmitResult& Result,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// SubmitValidatedWithHash
// ============================================================

UHorizonAsync_SubmitValidatedWithHash* UHorizonAsync_SubmitValidatedWithHash::SubmitValidatedWithHash(const UObject* WorldContextObject,
	int64 Score, const FString& InputLogHash, const FString& Stage, const FString& LeaderboardKey, const TArray<FHorizonEarnedValue>& Earned)
{
	UHorizonAsync_SubmitValidatedWithHash* Action = NewObject<UHorizonAsync_SubmitValidatedWithHash>();
	Action->WorldContext = WorldContextObject;
	Action->ScoreValue = Score;
	Action->InputLogHashStr = InputLogHash;
	Action->StageStr = Stage;
	Action->LeaderboardKeyStr = LeaderboardKey;
	Action->EarnedValues = Earned;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_SubmitValidatedWithHash::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->ValidatedActions)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or ValidatedActions manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->ValidatedActions->SubmitValidatedWithHash(
		ScoreValue, InputLogHashStr, StageStr, LeaderboardKeyStr, EarnedValues,
		FOnValidatedSubmitComplete::CreateUObject(this, &UHorizonAsync_SubmitValidatedWithHash::HandleResult)
	);
}

void UHorizonAsync_SubmitValidatedWithHash::HandleResult(bool bSuccess, const FHorizonValidatedSubmitResult& Result,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// GetValidatedState (Part 2)
// ============================================================

UHorizonAsync_GetValidatedState* UHorizonAsync_GetValidatedState::GetValidatedState(const UObject* WorldContextObject)
{
	UHorizonAsync_GetValidatedState* Action = NewObject<UHorizonAsync_GetValidatedState>();
	Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_GetValidatedState::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->ValidatedActions)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or ValidatedActions manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->ValidatedActions->GetState(
		FOnPlayerStateLoaded::CreateUObject(this, &UHorizonAsync_GetValidatedState::HandleResult)
	);
}

void UHorizonAsync_GetValidatedState::HandleResult(bool bSuccess, const FHorizonPlayerState& State,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(State);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}

// ============================================================
// UploadEvidence (Part 3)
// ============================================================

UHorizonAsync_UploadEvidence* UHorizonAsync_UploadEvidence::UploadEvidence(const UObject* WorldContextObject,
	const FString& RunId, const TArray<uint8>& InputLog)
{
	UHorizonAsync_UploadEvidence* Action = NewObject<UHorizonAsync_UploadEvidence>();
	Action->WorldContext = WorldContextObject;
	Action->RunIdStr = RunId;
	Action->InputLogBytes = InputLog;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_UploadEvidence::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->ValidatedActions)
	{
		OnFailure.Broadcast(TEXT("UNKNOWN"), TEXT("horizOn Subsystem or ValidatedActions manager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->ValidatedActions->UploadEvidence(
		RunIdStr,
		InputLogBytes,
		FOnEvidenceUploaded::CreateUObject(this, &UHorizonAsync_UploadEvidence::HandleResult)
	);
}

void UHorizonAsync_UploadEvidence::HandleResult(bool bSuccess, const FHorizonEvidenceUploadResult& Result,
	const FString& ErrorCode, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}
