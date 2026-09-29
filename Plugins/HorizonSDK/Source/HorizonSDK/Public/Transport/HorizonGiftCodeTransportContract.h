// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "Transport/HorizonLeaderboardTransportContract.h"

#include <string>

namespace HorizonTransportContract
{
	struct FGiftCodeRedeemPlan
	{
		bool bShouldSend = false;
		// horizOn-Server binds gift code redemption to the player's Bearer session (TASK-886).
		bool bUseSessionToken = true;
		std::string Endpoint;
		std::string BodyJson;
	};

	/**
	 * Builds the redeem request only for a signed-in user with a session token.
	 * Without both, bShouldSend stays false and nothing reaches the server.
	 */
	inline FGiftCodeRedeemPlan BuildGiftCodeRedeemPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& Code)
	{
		FGiftCodeRedeemPlan Plan;
		if (UserId.empty() || SessionToken.empty() || Code.empty())
		{
			return Plan;
		}

		Plan.bShouldSend = true;
		Plan.Endpoint = "/api/v1/app/gift-codes/redeem";
		Plan.BodyJson = "{\"code\":\"" + EscapeJson(Code) + "\",\"userId\":\"" + EscapeJson(UserId) + "\"}";
		return Plan;
	}
}
