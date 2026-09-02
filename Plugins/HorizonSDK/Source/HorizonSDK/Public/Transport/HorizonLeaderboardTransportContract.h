// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace HorizonTransportContract
{
	struct FLeaderboardSubmitPlan
	{
		bool bShouldSend = false;
		bool bUseSessionToken = true;
		std::string Endpoint;
		std::string BodyJson;
	};

	inline std::string EscapeJson(const std::string& Value)
	{
		std::ostringstream Result;
		for (const unsigned char Character : Value)
		{
			switch (Character)
			{
			case '\\': Result << "\\\\"; break;
			case '"': Result << "\\\""; break;
			case '\n': Result << "\\n"; break;
			case '\r': Result << "\\r"; break;
			case '\t': Result << "\\t"; break;
			default:
				if (Character < 0x20)
				{
					Result << "\\u" << std::hex << std::setw(4) << std::setfill('0')
						<< static_cast<int>(Character) << std::dec;
				}
				else
				{
					Result << Character;
				}
			}
		}
		return Result.str();
	}

	inline std::string EncodePathSegment(const std::string& Value)
	{
		std::ostringstream Result;
		Result << std::uppercase << std::hex;
		for (const unsigned char Character : Value)
		{
			if (std::isalnum(Character) || Character == '-' || Character == '_' || Character == '.' || Character == '~')
			{
				Result << Character;
			}
			else
			{
				Result << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(Character);
			}
		}
		return Result.str();
	}

	inline std::string Trim(const std::string& Value)
	{
		const std::size_t First = Value.find_first_not_of(" \t\r\n");
		if (First == std::string::npos)
		{
			return {};
		}
		const std::size_t Last = Value.find_last_not_of(" \t\r\n");
		return Value.substr(First, Last - First + 1);
	}

	inline FLeaderboardSubmitPlan BuildLeaderboardSubmitPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		std::int64_t Score,
		const std::string& BoardKey,
		const std::string& Metadata)
	{
		FLeaderboardSubmitPlan Plan;
		if (UserId.empty() || SessionToken.empty())
		{
			return Plan;
		}

		const std::string NormalizedBoardKey = Trim(BoardKey);
		Plan.bShouldSend = true;
		Plan.Endpoint = NormalizedBoardKey.empty()
			? "/api/v1/app/leaderboard/submit"
			: "/api/v1/app/leaderboards/" + EncodePathSegment(NormalizedBoardKey) + "/submit";
		Plan.BodyJson = "{\"userId\":\"" + EscapeJson(UserId) + "\",\"score\":" + std::to_string(Score);
		if (!NormalizedBoardKey.empty())
		{
			Plan.BodyJson += ",\"leaderboardKey\":\"" + EscapeJson(NormalizedBoardKey) + "\"";
		}
		if (!Metadata.empty())
		{
			Plan.BodyJson += ",\"metadata\":\"" + EscapeJson(Metadata) + "\"";
		}
		Plan.BodyJson += "}";
		return Plan;
	}

	inline std::vector<std::pair<std::string, std::string>> BuildHeaders(
		const std::string& ProjectKey,
		const std::string& SessionToken,
		bool bUseSessionToken)
	{
		std::vector<std::pair<std::string, std::string>> Headers = {
			{"X-API-Key", ProjectKey}
		};
		if (bUseSessionToken && !SessionToken.empty())
		{
			Headers.emplace_back("Authorization", "Bearer " + SessionToken);
		}
		return Headers;
	}
}
