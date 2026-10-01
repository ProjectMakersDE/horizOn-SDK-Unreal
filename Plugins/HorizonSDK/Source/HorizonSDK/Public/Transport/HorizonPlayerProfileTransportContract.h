// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "Transport/HorizonLeaderboardTransportContract.h"

#include <cstddef>
#include <string>
#include <vector>

namespace HorizonTransportContract
{
	/** Badges a player may show at once (server rule, `limits.maxBadges`). */
	constexpr std::size_t MaxPlayerProfileBadges = 3;

	/** Longest cosmetic ID the server accepts. */
	constexpr std::size_t MaxCosmeticIdLength = 32;

	/**
	 * Request plan for the player profile endpoints (TASK-881).
	 * Both endpoints need the player's Bearer session. When a local check fails,
	 * bShouldSend stays false and ErrorCode / ErrorMessage explain why; nothing
	 * reaches the server.
	 */
	struct FPlayerProfileRequestPlan
	{
		bool bShouldSend = false;
		bool bUseSessionToken = true;
		std::string Verb;
		std::string Endpoint;
		std::string BodyJson;
		std::string ErrorCode;
		std::string ErrorMessage;
	};

	/** Mirrors the server pattern `^[a-z0-9][a-z0-9._-]{0,31}$`. */
	inline bool IsValidCosmeticId(const std::string& CosmeticId)
	{
		if (CosmeticId.empty() || CosmeticId.size() > MaxCosmeticIdLength)
		{
			return false;
		}

		const auto IsLowerAlnum = [](unsigned char Character)
		{
			return (Character >= 'a' && Character <= 'z') || (Character >= '0' && Character <= '9');
		};

		if (!IsLowerAlnum(static_cast<unsigned char>(CosmeticId[0])))
		{
			return false;
		}

		for (const unsigned char Character : CosmeticId)
		{
			if (!IsLowerAlnum(Character) && Character != '.' && Character != '_' && Character != '-')
			{
				return false;
			}
		}
		return true;
	}

	/** JSON string for a slot, or `null` when the slot is empty (clears it). */
	inline std::string CosmeticSlotJson(const std::string& CosmeticId)
	{
		return CosmeticId.empty() ? std::string("null") : "\"" + EscapeJson(CosmeticId) + "\"";
	}

	inline FPlayerProfileRequestPlan PlayerProfileSessionRequiredPlan()
	{
		FPlayerProfileRequestPlan Plan;
		Plan.ErrorCode = "SESSION_REQUIRED";
		Plan.ErrorMessage = "A signed-in player is required.";
		return Plan;
	}

	/** GET /api/v1/app/player-profile?userId=... for the signed-in player. */
	inline FPlayerProfileRequestPlan BuildPlayerProfileGetPlan(
		const std::string& UserId,
		const std::string& SessionToken)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return PlayerProfileSessionRequiredPlan();
		}

		FPlayerProfileRequestPlan Plan;
		Plan.bShouldSend = true;
		Plan.Verb = "GET";
		Plan.Endpoint = "/api/v1/app/player-profile?userId=" + EncodePathSegment(UserId);
		return Plan;
	}

	/**
	 * PUT /api/v1/app/player-profile. Replaces the whole visible profile:
	 * an empty AvatarId / FrameId is sent as `null` (clears the slot), an empty
	 * Badges list clears all badges.
	 *
	 * Local pre-checks (the server checks everything again):
	 * more than 3 badges or a badge listed twice gives INVALID_BADGES,
	 * an ID that does not match the cosmetic ID pattern gives INVALID_COSMETIC_ID.
	 */
	inline FPlayerProfileRequestPlan BuildPlayerProfileSetPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& AvatarId,
		const std::string& FrameId,
		const std::vector<std::string>& Badges)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return PlayerProfileSessionRequiredPlan();
		}

		FPlayerProfileRequestPlan Plan;

		if (Badges.size() > MaxPlayerProfileBadges)
		{
			Plan.ErrorCode = "INVALID_BADGES";
			Plan.ErrorMessage = "At most 3 badges can be shown.";
			return Plan;
		}
		for (std::size_t Index = 0; Index < Badges.size(); ++Index)
		{
			for (std::size_t Other = Index + 1; Other < Badges.size(); ++Other)
			{
				if (Badges[Index] == Badges[Other])
				{
					Plan.ErrorCode = "INVALID_BADGES";
					Plan.ErrorMessage = "Badge '" + Badges[Index] + "' is listed twice.";
					return Plan;
				}
			}
		}

		std::vector<std::string> IdsToCheck;
		if (!AvatarId.empty())
		{
			IdsToCheck.push_back(AvatarId);
		}
		if (!FrameId.empty())
		{
			IdsToCheck.push_back(FrameId);
		}
		IdsToCheck.insert(IdsToCheck.end(), Badges.begin(), Badges.end());
		for (const std::string& CosmeticId : IdsToCheck)
		{
			if (!IsValidCosmeticId(CosmeticId))
			{
				Plan.ErrorCode = "INVALID_COSMETIC_ID";
				Plan.ErrorMessage = "Invalid cosmetic ID '" + CosmeticId + "'.";
				return Plan;
			}
		}

		Plan.bShouldSend = true;
		Plan.Verb = "PUT";
		Plan.Endpoint = "/api/v1/app/player-profile";
		Plan.BodyJson = "{\"userId\":\"" + EscapeJson(UserId) + "\""
			+ ",\"avatarId\":" + CosmeticSlotJson(AvatarId)
			+ ",\"frameId\":" + CosmeticSlotJson(FrameId)
			+ ",\"badges\":[";
		for (std::size_t Index = 0; Index < Badges.size(); ++Index)
		{
			if (Index > 0)
			{
				Plan.BodyJson += ",";
			}
			Plan.BodyJson += "\"" + EscapeJson(Badges[Index]) + "\"";
		}
		Plan.BodyJson += "]}";
		return Plan;
	}
}
