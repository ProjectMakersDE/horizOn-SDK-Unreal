// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Models/HorizonPlayerProfile.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	/** Reads a string field; JSON null or a missing field gives "" without warnings. */
	FString ReadProfileJsonString(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		FString Value;
		if (JsonObject.IsValid() && JsonObject->TryGetStringField(FieldName, Value))
		{
			return Value;
		}
		return FString();
	}

	/** Reads an array of strings; JSON null, a missing field or non string items are skipped. */
	TArray<FString> ReadProfileJsonStringArray(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		TArray<FString> Result;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, Values) || !Values)
		{
			return Result;
		}

		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			if (Value.IsValid() && Value->Type == EJson::String)
			{
				Result.Add(Value->AsString());
			}
		}
		return Result;
	}
}

// ============================================================
// FHorizonPlayerProfile
// ============================================================

FHorizonPlayerProfile FHorizonPlayerProfile::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonPlayerProfile Profile;
	if (!JsonObject.IsValid())
	{
		return Profile;
	}

	Profile.AvatarId = ReadProfileJsonString(JsonObject, TEXT("avatarId"));
	Profile.FrameId = ReadProfileJsonString(JsonObject, TEXT("frameId"));
	Profile.Badges = ReadProfileJsonStringArray(JsonObject, TEXT("badges"));
	return Profile;
}

// ============================================================
// FHorizonCosmetic
// ============================================================

FHorizonCosmetic FHorizonCosmetic::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonCosmetic Cosmetic;
	if (!JsonObject.IsValid())
	{
		return Cosmetic;
	}

	Cosmetic.Id = ReadProfileJsonString(JsonObject, TEXT("id"));
	Cosmetic.Type = ReadProfileJsonString(JsonObject, TEXT("type"));
	JsonObject->TryGetBoolField(TEXT("locked"), Cosmetic.bLocked);
	JsonObject->TryGetBoolField(TEXT("available"), Cosmetic.bAvailable);
	return Cosmetic;
}

// ============================================================
// FHorizonPlayerProfileResult
// ============================================================

TArray<FHorizonCosmetic> FHorizonPlayerProfileResult::GetCosmetics(const FString& Type) const
{
	TArray<FHorizonCosmetic> Result;
	for (const FHorizonCosmetic& Cosmetic : Cosmetics)
	{
		if (Cosmetic.Type.Equals(Type, ESearchCase::IgnoreCase))
		{
			Result.Add(Cosmetic);
		}
	}
	return Result;
}

bool FHorizonPlayerProfileResult::IsAvailable(const FString& CosmeticId) const
{
	for (const FHorizonCosmetic& Cosmetic : Cosmetics)
	{
		if (Cosmetic.Id == CosmeticId)
		{
			return Cosmetic.bAvailable;
		}
	}
	return false;
}

FHorizonPlayerProfileResult FHorizonPlayerProfileResult::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonPlayerProfileResult Result;
	if (!JsonObject.IsValid())
	{
		return Result;
	}

	Result.UserId = ReadProfileJsonString(JsonObject, TEXT("userId"));

	const TSharedPtr<FJsonObject>* ProfileObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("profile"), ProfileObject) && ProfileObject)
	{
		Result.Profile = FHorizonPlayerProfile::FromJson(*ProfileObject);
	}

	Result.Unlocks = ReadProfileJsonStringArray(JsonObject, TEXT("unlocks"));

	const TArray<TSharedPtr<FJsonValue>>* CosmeticValues = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("cosmetics"), CosmeticValues) && CosmeticValues)
	{
		for (const TSharedPtr<FJsonValue>& Value : *CosmeticValues)
		{
			if (Value.IsValid() && Value->Type == EJson::Object)
			{
				Result.Cosmetics.Add(FHorizonCosmetic::FromJson(Value->AsObject()));
			}
		}
	}

	const TSharedPtr<FJsonObject>* LimitsObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("limits"), LimitsObject) && LimitsObject && LimitsObject->IsValid())
	{
		(*LimitsObject)->TryGetNumberField(TEXT("maxBadges"), Result.MaxBadges);
		(*LimitsObject)->TryGetNumberField(TEXT("maxUnlocks"), Result.MaxUnlocks);
	}

	return Result;
}
