// Copyright (c) 2025-2026 horizOn. All rights reserved.
#pragma once
#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace HorizonTransportContract
{
	constexpr int64_t StatisticsMaxValue = 9007199254740991LL;
	/** Strict JSON integer token, checked before any floating-point conversion. */
	inline bool ParseStatisticsInteger(const std::string& Text, int64_t& Out)
	{
		if (Text.empty()) return false;
		const bool Negative = Text[0] == '-'; const size_t Start = Negative ? 1 : 0;
		if (Text.size() == Start || Text.size() - Start > 16 || (Text.size() - Start > 1 && Text[Start] == '0')) return false;
		int64_t Value = 0;
		for (size_t Index = Start; Index < Text.size(); ++Index)
		{
			const char C = Text[Index]; if (C < '0' || C > '9') return false;
			const int64_t Digit = C - '0'; if (Value > (StatisticsMaxValue - Digit) / 10) return false;
			Value = Value * 10 + Digit;
		}
		Out = Negative ? -Value : Value; return true;
	}
	inline bool IsStatisticKey(const std::string& Key)
	{
		if (Key.empty() || Key.size() > 24) return false;
		for (size_t Index = 0; Index < Key.size(); ++Index)
		{
			const char C = Key[Index];
			if ((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9')) continue;
			if (Index > 0 && (C == '.' || C == '_' || C == '-')) continue;
			return false;
		}
		return true;
	}
	inline bool IsStatisticsOperationId(const std::string& Id)
	{
		if (Id.size() != 50 || Id[13] != ':') return false;
		for (size_t Index = 0; Index < Id.size(); ++Index)
		{
			if (Index == 13) continue;
			const char C = Id[Index];
			if (Index < 13) { if (C < '0' || C > '9') return false; }
			else if (Index == 22 || Index == 27 || Index == 32 || Index == 37) { if (C != '-') return false; }
			else if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F'))) return false;
		}
		return true;
	}
	inline bool IsStatisticsNumber(double Value)
	{
		return std::isfinite(Value) && Value >= -StatisticsMaxValue && Value <= StatisticsMaxValue && Value == std::floor(Value);
	}
	inline bool IsStatisticsBatch(const std::vector<std::pair<std::string, int64_t>>& Updates)
	{
		if (Updates.empty() || Updates.size() > 100) return false;
		std::set<std::string> Keys;
		for (const auto& Entry : Updates)
			if (!IsStatisticKey(Entry.first) || Entry.second < -StatisticsMaxValue || Entry.second > StatisticsMaxValue || !Keys.insert(Entry.first).second) return false;
		return true;
	}
}
