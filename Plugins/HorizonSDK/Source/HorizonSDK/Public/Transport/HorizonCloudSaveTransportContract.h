// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

/**
 * Cloud Save revision contract, checked 2026-10-11 against horizOn-Server
 * AppCloudSaveController and CloudSaveExceptionHandler (released in 1.64.1):
 * every load answers X-Cloud-Save-Revision (0 = empty slot), a save accepts
 * If-Match: <revision>, a mismatch answers 409 and writes nothing, and a save
 * response carries the new revision (>= 1). simpleServer sends no header, so
 * the revision is unknown there, not an empty slot.
 */
namespace HorizonTransportContract
{
	/** Revision when the server sent none (simpleServer) or an unusable value. */
	constexpr int64_t CloudSaveRevisionUnknown = -1;

	constexpr const char* CloudSaveRevisionHeader = "X-Cloud-Save-Revision";
	constexpr const char* CloudSaveIfMatchHeader = "If-Match";

	/** Value of a header from "Name: value" lines, name matched case-insensitively. Empty when missing. */
	inline std::string FindHeader(const std::vector<std::string>& Lines, const std::string& Name)
	{
		auto Lower = [](std::string Value)
		{
			for (char& Character : Value)
			{
				Character = static_cast<char>(std::tolower(static_cast<unsigned char>(Character)));
			}
			return Value;
		};
		auto Trim = [](const std::string& Value)
		{
			const std::size_t First = Value.find_first_not_of(" \t\r\n");
			if (First == std::string::npos)
			{
				return std::string();
			}
			return Value.substr(First, Value.find_last_not_of(" \t\r\n") - First + 1);
		};

		const std::string Wanted = Lower(Name);
		for (const std::string& Line : Lines)
		{
			const std::size_t Colon = Line.find(':');
			if (Colon != std::string::npos && Lower(Trim(Line.substr(0, Colon))) == Wanted)
			{
				return Trim(Line.substr(Colon + 1));
			}
		}
		return {};
	}

	/** Revision of a load from X-Cloud-Save-Revision: a whole number from 0 to INT64_MAX, else unknown. */
	inline int64_t ParseCloudSaveRevision(const std::string& HeaderValue)
	{
		if (HeaderValue.empty())
		{
			return CloudSaveRevisionUnknown;
		}
		int64_t Revision = 0;
		for (const char Character : HeaderValue)
		{
			if (!std::isdigit(static_cast<unsigned char>(Character)))
			{
				return CloudSaveRevisionUnknown;
			}
			const int64_t Digit = Character - '0';
			if (Revision > (INT64_MAX - Digit) / 10)
			{
				return CloudSaveRevisionUnknown;
			}
			Revision = Revision * 10 + Digit;
		}
		return Revision;
	}

	/** Revision of a save response (JSON number field "revision"): a whole number >= 1 within int64, else unknown. */
	inline int64_t CloudSaveRevisionFromSave(bool bHasField, double Value)
	{
		const bool bValid = bHasField && Value >= 1.0 && Value < 9223372036854775808.0 && Value == std::floor(Value);
		return bValid ? static_cast<int64_t>(Value) : CloudSaveRevisionUnknown;
	}

	/** If-Match header that makes a save conditional on the given revision. */
	inline std::pair<std::string, std::string> BuildCloudSaveIfMatch(int64_t ExpectedRevision)
	{
		return {CloudSaveIfMatchHeader, std::to_string(ExpectedRevision)};
	}
}
