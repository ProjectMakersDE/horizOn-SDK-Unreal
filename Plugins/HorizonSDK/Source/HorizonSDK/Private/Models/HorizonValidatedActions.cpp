// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Models/HorizonValidatedActions.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	/** Reads a string field; JSON null or a missing field gives "". */
	FString ReadValidatedString(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		FString Value;
		if (JsonObject.IsValid() && JsonObject->TryGetStringField(FieldName, Value))
		{
			return Value;
		}
		return FString();
	}

	/** Reads a whole number; JSON null or a missing field gives 0. Values up to 2^53 stay exact. */
	int64 ReadValidatedInt64(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		double Value = 0.0;
		if (JsonObject.IsValid() && JsonObject->TryGetNumberField(FieldName, Value))
		{
			return static_cast<int64>(Value);
		}
		return 0;
	}

	/** Reads a bool; JSON null or a missing field gives false. */
	bool ReadValidatedBool(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		bool bValue = false;
		if (JsonObject.IsValid() && JsonObject->TryGetBoolField(FieldName, bValue))
		{
			return bValue;
		}
		return false;
	}

	/** Reads a nested object; JSON null or a missing field gives an invalid pointer. */
	TSharedPtr<FJsonObject> ReadValidatedObject(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		const TSharedPtr<FJsonObject>* Nested = nullptr;
		if (JsonObject.IsValid() && JsonObject->TryGetObjectField(FieldName, Nested) && Nested && Nested->IsValid())
		{
			return *Nested;
		}
		return nullptr;
	}
}

// ============================================================
// FHorizonValidatedRun
// ============================================================

FHorizonValidatedRun FHorizonValidatedRun::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonValidatedRun Run;
	if (!JsonObject.IsValid())
	{
		return Run;
	}

	Run.RunId = ReadValidatedString(JsonObject, TEXT("runId"));
	Run.Ticket = ReadValidatedString(JsonObject, TEXT("ticket"));
	Run.Seed = static_cast<int32>(ReadValidatedInt64(JsonObject, TEXT("seed")));
	Run.LeaderboardKey = ReadValidatedString(JsonObject, TEXT("leaderboardKey"));
	Run.IssuedAt = ReadValidatedString(JsonObject, TEXT("issuedAt"));
	Run.ExpiresAt = ReadValidatedString(JsonObject, TEXT("expiresAt"));
	Run.ExpiresInSeconds = static_cast<int32>(ReadValidatedInt64(JsonObject, TEXT("expiresInSeconds")));
	return Run;
}

// ============================================================
// FHorizonPlayerStateValue / FHorizonPlayerState (Part 2)
// ============================================================

FHorizonPlayerStateValue FHorizonPlayerStateValue::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonPlayerStateValue Value;
	if (!JsonObject.IsValid())
	{
		return Value;
	}

	Value.Key = ReadValidatedString(JsonObject, TEXT("key"));
	Value.Balance = ReadValidatedInt64(JsonObject, TEXT("balance"));
	Value.EarnedToday = ReadValidatedInt64(JsonObject, TEXT("earnedToday"));
	// null (no cap) gives 0; requested and credited are omitted for untouched values and in GET .../state.
	Value.DailyCap = ReadValidatedInt64(JsonObject, TEXT("dailyCap"));
	Value.Requested = ReadValidatedInt64(JsonObject, TEXT("requested"));
	Value.Credited = ReadValidatedInt64(JsonObject, TEXT("credited"));
	return Value;
}

int64 FHorizonPlayerState::GetBalance(const FString& Key) const
{
	const FHorizonPlayerStateValue* Value = FindValue(Key);
	return Value ? Value->Balance : 0;
}

const FHorizonPlayerStateValue* FHorizonPlayerState::FindValue(const FString& Key) const
{
	// Keys are lower case by rule; compare exactly like the server.
	for (const FHorizonPlayerStateValue& Value : Values)
	{
		if (Value.Key.Equals(Key, ESearchCase::CaseSensitive))
		{
			return &Value;
		}
	}
	return nullptr;
}

FHorizonPlayerState FHorizonPlayerState::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonPlayerState State;
	if (!JsonObject.IsValid())
	{
		return State;
	}

	State.UserId = ReadValidatedString(JsonObject, TEXT("userId"));
	State.Day = ReadValidatedString(JsonObject, TEXT("day"));

	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("values"), Values) && Values)
	{
		for (const TSharedPtr<FJsonValue>& Item : *Values)
		{
			const TSharedPtr<FJsonObject>* ItemObject = nullptr;
			if (Item.IsValid() && Item->TryGetObject(ItemObject) && ItemObject && ItemObject->IsValid())
			{
				State.Values.Add(FHorizonPlayerStateValue::FromJson(*ItemObject));
			}
		}
	}
	return State;
}

// ============================================================
// FHorizonEvidenceRequest (Part 3)
// ============================================================

FHorizonEvidenceRequest FHorizonEvidenceRequest::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonEvidenceRequest Evidence;
	if (!JsonObject.IsValid())
	{
		return Evidence;
	}

	Evidence.bRequired = ReadValidatedBool(JsonObject, TEXT("required"));
	Evidence.RunId = ReadValidatedString(JsonObject, TEXT("runId"));
	Evidence.UploadBefore = ReadValidatedString(JsonObject, TEXT("uploadBefore"));
	Evidence.MaxBytes = static_cast<int32>(ReadValidatedInt64(JsonObject, TEXT("maxBytes")));
	return Evidence;
}

// ============================================================
// FHorizonValidatedSubmitResult
// ============================================================

FHorizonValidatedSubmitResult FHorizonValidatedSubmitResult::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonValidatedSubmitResult Result;
	if (!JsonObject.IsValid())
	{
		return Result;
	}

	Result.bAccepted = ReadValidatedBool(JsonObject, TEXT("accepted"));
	Result.RunId = ReadValidatedString(JsonObject, TEXT("runId"));
	Result.LeaderboardKey = ReadValidatedString(JsonObject, TEXT("leaderboardKey"));
	Result.Score = ReadValidatedInt64(JsonObject, TEXT("score"));
	Result.BestScore = ReadValidatedInt64(JsonObject, TEXT("bestScore"));
	// The contract names the field isNewHighScore; accept newHighScore too (Jackson bean naming).
	Result.bIsNewHighScore = ReadValidatedBool(JsonObject, TEXT("isNewHighScore"))
		|| ReadValidatedBool(JsonObject, TEXT("newHighScore"));
	Result.Rank = ReadValidatedInt64(JsonObject, TEXT("rank"));
	Result.DurationSeconds = ReadValidatedInt64(JsonObject, TEXT("durationSeconds"));
	Result.State = FHorizonPlayerState::FromJson(ReadValidatedObject(JsonObject, TEXT("state")));
	Result.Evidence = FHorizonEvidenceRequest::FromJson(ReadValidatedObject(JsonObject, TEXT("evidence")));
	return Result;
}
