// Compiles the production Validated Actions models (Public/Models/HorizonValidatedActions.h and
// Private/Models/HorizonValidatedActions.cpp) against an in-memory UE boundary and checks the
// null safe parsing of the submit result, in particular `sus` (TASK-911), and the run start
// context's IsEmpty. run_transport_test.sh provides empty stand-ins for the engine headers
// (CoreMinimal.h, the generated header, Dom/JsonObject.h, Dom/JsonValue.h). It does not replace
// UnrealBuildTool validation.

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ---- UE boundary ---------------------------------------------------------------------------

#define USTRUCT(...)
#define UPROPERTY(...)
#define GENERATED_BODY()
#define HORIZONSDK_API
#define TEXT(Value) Value

using TCHAR = char;
using uint8 = std::uint8_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

namespace ESearchCase
{
	enum Type { CaseSensitive, IgnoreCase };
}

struct FString : std::string
{
	using std::string::string;
	FString() = default;
	FString(const std::string& Value) : std::string(Value) {}
	bool IsEmpty() const { return empty(); }
	FString TrimStartAndEnd() const
	{
		const std::size_t First = find_first_not_of(" \t\r\n");
		if (First == std::string::npos)
		{
			return FString();
		}
		const std::size_t Last = find_last_not_of(" \t\r\n");
		return FString(substr(First, Last - First + 1));
	}
	bool Equals(const FString& Other, ESearchCase::Type) const { return *this == Other; }
};

template <typename T>
struct TArray : std::vector<T>
{
	using std::vector<T>::vector;
	int32 Num() const { return static_cast<int32>(this->size()); }
	void Add(const T& Value) { this->push_back(Value); }
};

template <typename T>
struct TSharedPtr : std::shared_ptr<T>
{
	using std::shared_ptr<T>::shared_ptr;
	TSharedPtr(const std::shared_ptr<T>& Value) : std::shared_ptr<T>(Value) {}
	bool IsValid() const { return this->get() != nullptr; }
};

class FJsonObject;

/** One JSON value: string, number, bool, object, array or null. */
class FJsonValue
{
public:
	enum class EKind { Null, String, Number, Bool, Object, Array };
	EKind Kind = EKind::Null;
	FString String;
	double Number = 0.0;
	bool bBool = false;
	TSharedPtr<FJsonObject> Object;
	TArray<TSharedPtr<FJsonValue>> Array;

	bool TryGetObject(const TSharedPtr<FJsonObject>*& Out) const
	{
		if (Kind != EKind::Object)
		{
			return false;
		}
		Out = &Object;
		return true;
	}
};

/** Mirrors the UE TryGet*Field semantics the models rely on: a JSON null or a missing field fails. */
class FJsonObject
{
public:
	std::map<std::string, TSharedPtr<FJsonValue>> Values;

	bool TryGetStringField(const char* Name, FString& Out) const
	{
		const FJsonValue* Value = Find(Name, FJsonValue::EKind::String);
		if (!Value)
		{
			return false;
		}
		Out = Value->String;
		return true;
	}

	bool TryGetNumberField(const char* Name, double& Out) const
	{
		const FJsonValue* Value = Find(Name, FJsonValue::EKind::Number);
		if (!Value)
		{
			return false;
		}
		Out = Value->Number;
		return true;
	}

	bool TryGetBoolField(const char* Name, bool& Out) const
	{
		const FJsonValue* Value = Find(Name, FJsonValue::EKind::Bool);
		if (!Value)
		{
			return false;
		}
		Out = Value->bBool;
		return true;
	}

	bool TryGetObjectField(const char* Name, const TSharedPtr<FJsonObject>*& Out) const
	{
		const FJsonValue* Value = Find(Name, FJsonValue::EKind::Object);
		if (!Value)
		{
			return false;
		}
		Out = &Value->Object;
		return true;
	}

	bool TryGetArrayField(const char* Name, const TArray<TSharedPtr<FJsonValue>>*& Out) const
	{
		const FJsonValue* Value = Find(Name, FJsonValue::EKind::Array);
		if (!Value)
		{
			return false;
		}
		Out = &Value->Array;
		return true;
	}

private:
	const FJsonValue* Find(const char* Name, FJsonValue::EKind Kind) const
	{
		const auto Iterator = Values.find(Name);
		if (Iterator == Values.end() || !Iterator->second.IsValid() || Iterator->second->Kind != Kind)
		{
			return nullptr;
		}
		return Iterator->second.get();
	}
};

// ---- Production code ------------------------------------------------------------------------

#include "Models/HorizonValidatedActions.h"
#include "../Plugins/HorizonSDK/Source/HorizonSDK/Private/Models/HorizonValidatedActions.cpp"

// ---- Test -----------------------------------------------------------------------------------

namespace
{
	void Require(bool Condition, const char* Message)
	{
		if (!Condition)
		{
			throw std::runtime_error(Message);
		}
	}

	TSharedPtr<FJsonValue> Str(const char* Text)
	{
		auto Value = std::make_shared<FJsonValue>();
		Value->Kind = FJsonValue::EKind::String;
		Value->String = Text;
		return Value;
	}

	TSharedPtr<FJsonValue> Num(double Number)
	{
		auto Value = std::make_shared<FJsonValue>();
		Value->Kind = FJsonValue::EKind::Number;
		Value->Number = Number;
		return Value;
	}

	TSharedPtr<FJsonValue> Bool(bool bFlag)
	{
		auto Value = std::make_shared<FJsonValue>();
		Value->Kind = FJsonValue::EKind::Bool;
		Value->bBool = bFlag;
		return Value;
	}

	TSharedPtr<FJsonValue> Null()
	{
		return std::make_shared<FJsonValue>();
	}

	TSharedPtr<FJsonValue> Obj(const TSharedPtr<FJsonObject>& Object)
	{
		auto Value = std::make_shared<FJsonValue>();
		Value->Kind = FJsonValue::EKind::Object;
		Value->Object = Object;
		return Value;
	}

	/** The accepted submit response of API-ENDPOINTS.md, without `sus` (an older server). */
	TSharedPtr<FJsonObject> AcceptedResponse()
	{
		auto Json = std::make_shared<FJsonObject>();
		Json->Values["accepted"] = Bool(true);
		Json->Values["runId"] = Str("run-911");
		Json->Values["leaderboardKey"] = Str("weekly");
		Json->Values["score"] = Num(18250);
		Json->Values["bestScore"] = Num(21000);
		Json->Values["isNewHighScore"] = Bool(false);
		Json->Values["rank"] = Num(17);
		Json->Values["durationSeconds"] = Num(734);
		Json->Values["state"] = Null();
		Json->Values["evidence"] = Null();
		return Json;
	}
}

int main()
{
	// An older server omits `sus`: false. The other fields still parse.
	const FHorizonValidatedSubmitResult Old = FHorizonValidatedSubmitResult::FromJson(AcceptedResponse());
	Require(Old.bAccepted && Old.RunId == "run-911" && Old.Rank == 17 && Old.BestScore == 21000, "submit result fields");
	Require(!Old.bSus, "a missing sus must read as false");
	Require(!Old.Evidence.bRequired && Old.State.IsEmpty(), "null state and evidence must be empty");

	// `sus: true` with an evidence request (the log of a sus run is uploaded like a top N record).
	TSharedPtr<FJsonObject> SusJson = AcceptedResponse();
	auto Evidence = std::make_shared<FJsonObject>();
	Evidence->Values["required"] = Bool(true);
	Evidence->Values["runId"] = Str("run-911");
	Evidence->Values["uploadBefore"] = Str("2026-10-03T14:00:00.000Z");
	Evidence->Values["maxBytes"] = Num(32768);
	SusJson->Values["evidence"] = Obj(Evidence);
	SusJson->Values["sus"] = Bool(true);
	const FHorizonValidatedSubmitResult Sus = FHorizonValidatedSubmitResult::FromJson(SusJson);
	Require(Sus.bSus, "sus true must be read");
	Require(Sus.Evidence.bRequired && Sus.Evidence.RunId == "run-911" && Sus.Evidence.MaxBytes == 32768, "evidence of a sus run");

	// `sus: false`, `sus: null` and a wrongly typed `sus` read as false.
	SusJson->Values["sus"] = Bool(false);
	Require(!FHorizonValidatedSubmitResult::FromJson(SusJson).bSus, "sus false must be read");
	SusJson->Values["sus"] = Null();
	Require(!FHorizonValidatedSubmitResult::FromJson(SusJson).bSus, "sus null must read as false");
	SusJson->Values["sus"] = Str("true");
	Require(!FHorizonValidatedSubmitResult::FromJson(SusJson).bSus, "a string sus must read as false");
	Require(!FHorizonValidatedSubmitResult::FromJson(nullptr).bSus, "an invalid object must read as false");

	// Run start context: blank fields count as absent.
	FHorizonRunContext Context;
	Require(Context.IsEmpty(), "a default context is empty");
	Context.GameVersion = "  ";
	Require(Context.IsEmpty(), "a blank version counts as absent");
	Context.InitialState.Add(1);
	Require(!Context.IsEmpty(), "an initial state makes the context non empty");

	std::cout << "Unreal SDK validated actions model parsing passed\n";
	return 0;
}
