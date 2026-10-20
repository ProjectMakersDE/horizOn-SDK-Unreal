// Copyright (c) 2025-2026 horizOn. All rights reserved.
#include "Models/HorizonStatistics.h"
#include "Transport/HorizonStatisticsTransportContract.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	bool Integer(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, int64& Out)
	{
		const auto Value = Json.IsValid() ? Json->TryGetField(Field) : nullptr;
		if (!Value.IsValid() || Value->Type != EJson::Number || !Value->PreferStringRepresentation()) return false;
		int64_t Exact = 0;
		if (!HorizonTransportContract::ParseStatisticsInteger(TCHAR_TO_UTF8(*Value->AsString()), Exact)) return false;
		Out = Exact; return true;
	}
	bool String(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, FString& Out)
	{
		const auto Value = Json.IsValid() ? Json->TryGetField(Field) : nullptr;
		if (!Value.IsValid() || Value->Type != EJson::String) return false;
		Out = Value->AsString(); return true;
	}
	bool ReadBoolean(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, bool& Out)
	{
		const auto Value = Json.IsValid() ? Json->TryGetField(Field) : nullptr;
		if (!Value.IsValid() || Value->Type != EJson::Boolean) return false;
		Out = Value->AsBool(); return true;
	}
}

FHorizonStatisticsCatalog FHorizonStatisticsCatalog::FromJson(const TSharedPtr<FJsonObject>& Json)
{
	FHorizonStatisticsCatalog Result;
	if (!Integer(Json, TEXT("definitionVersion"), Result.DefinitionVersion) || Result.DefinitionVersion < 0) return Result;
	const auto Field = Json->TryGetField(TEXT("definitions"));
	if (!Field.IsValid()) return Result;
	if (Field->Type == EJson::Null) { Result.bValid = true; return Result; }
	if (Field->Type != EJson::Array) return Result;
	TSet<FString> Keys;
	for (const auto& Value : Field->AsArray())
	{
		if (!Value.IsValid() || Value->Type != EJson::Object) return Result;
		const auto Definition = Value->AsObject();
		FHorizonStatisticDefinition Entry;
		if (!String(Definition, TEXT("key"), Entry.Key) || !HorizonTransportContract::IsStatisticKey(TCHAR_TO_UTF8(*Entry.Key)) || Keys.Contains(Entry.Key)
			|| !String(Definition, TEXT("name"), Entry.Name) || Entry.Name.IsEmpty() || !String(Definition, TEXT("mode"), Entry.Mode)
			|| !String(Definition, TEXT("reset"), Entry.Reset)) return Result;
		if (Entry.Mode != TEXT("sum") && Entry.Mode != TEXT("min") && Entry.Mode != TEXT("max") && Entry.Mode != TEXT("last")) return Result;
		if (Entry.Reset != TEXT("none") && Entry.Reset != TEXT("daily") && Entry.Reset != TEXT("weekly")) return Result;
		if (Definition->HasField(TEXT("maxPerUpdate")) && (!Integer(Definition, TEXT("maxPerUpdate"), Entry.MaxPerUpdate) || Entry.MaxPerUpdate <= 0)) return Result;
		Keys.Add(Entry.Key); Result.Definitions.Add(Entry);
	}
	Result.bDefinitionsIncluded = true; Result.bValid = true;
	return Result;
}

FHorizonStatisticsSnapshot FHorizonStatisticsSnapshot::FromJson(const TSharedPtr<FJsonObject>& Json)
{
	FHorizonStatisticsSnapshot Result;
	const auto Catalog = FHorizonStatisticsCatalog::FromJson(Json);
	if (!Catalog.bValid || !String(Json, TEXT("userId"), Result.UserId) || Result.UserId.IsEmpty()
		|| !Integer(Json, TEXT("revision"), Result.Revision) || Result.Revision < 0) return Result;
	Result.DefinitionVersion = Catalog.DefinitionVersion; Result.Definitions = Catalog.Definitions; Result.bDefinitionsIncluded = Catalog.bDefinitionsIncluded;
	const auto Values = Json->TryGetField(TEXT("values"));
	if (!Values.IsValid() || Values->Type != EJson::Array) return Result;
	TSet<FString> Keys;
	for (const auto& Value : Values->AsArray())
	{
		if (!Value.IsValid() || Value->Type != EJson::Object) return Result;
		const auto JsonValue = Value->AsObject(); FHorizonStatisticValue Entry;
		if (!String(JsonValue, TEXT("key"), Entry.Key) || !HorizonTransportContract::IsStatisticKey(TCHAR_TO_UTF8(*Entry.Key)) || Keys.Contains(Entry.Key)
			|| !Integer(JsonValue, TEXT("value"), Entry.Value) || !ReadBoolean(JsonValue, TEXT("hasValue"), Entry.bHasValue)) return Result;
		const auto Period = JsonValue->TryGetField(TEXT("period"));
		if (!Period.IsValid() || (Period->Type != EJson::Null && !String(JsonValue, TEXT("period"), Entry.Period))) return Result;
		Keys.Add(Entry.Key); Result.Values.Add(Entry);
	}
	const auto Window = Json->TryGetField(TEXT("operationWindow"));
	if (!Window.IsValid() || Window->Type != EJson::Object) return Result;
	const auto WindowJson = Window->AsObject();
	if (!Integer(WindowJson, TEXT("serverTimeUnixMillis"), Result.OperationWindow.ServerTimeUnixMillis)
		|| Result.OperationWindow.ServerTimeUnixMillis < 1000000000000LL || Result.OperationWindow.ServerTimeUnixMillis > 9999999999999LL
		|| !Integer(WindowJson, TEXT("maxAgeSeconds"), Result.OperationWindow.MaxAgeSeconds) || Result.OperationWindow.MaxAgeSeconds <= 0
		|| !Integer(WindowJson, TEXT("maxFutureSkewSeconds"), Result.OperationWindow.MaxFutureSkewSeconds) || Result.OperationWindow.MaxFutureSkewSeconds < 0) return Result;
	const auto Operation = Json->TryGetField(TEXT("operationId"));
	if (Operation.IsValid() && Operation->Type != EJson::Null && !String(Json, TEXT("operationId"), Result.OperationId)) return Result;
	const auto Replay = Json->TryGetField(TEXT("replayed"));
	if (Replay.IsValid() && !ReadBoolean(Json, TEXT("replayed"), Result.bReplayed)) return Result;
	const auto Unlocked = Json->TryGetField(TEXT("newlyUnlocked"));
	if (Unlocked.IsValid())
	{
		if (Unlocked->Type != EJson::Array) return Result;
		for (const auto& Entry : Unlocked->AsArray()) { if (!Entry.IsValid() || Entry->Type != EJson::String) return Result; Result.NewlyUnlocked.Add(Entry->AsString()); }
	}
	Result.bValid = true; return Result;
}
