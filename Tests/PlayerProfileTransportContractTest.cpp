#include "Transport/HorizonPlayerProfileTransportContract.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	void Require(bool Condition, const char* Message)
	{
		if (!Condition)
		{
			throw std::runtime_error(Message);
		}
	}

	bool HasBearer(const std::string& SessionToken, bool bUseSessionToken)
	{
		for (const auto& Header : HorizonTransportContract::BuildHeaders("project-key-881", SessionToken, bUseSessionToken))
		{
			if (Header.first == "Authorization" && Header.second == "Bearer " + SessionToken)
			{
				return true;
			}
		}
		return false;
	}
}

// Player profile (TASK-881): both endpoints need the player's Bearer session, PUT replaces the whole profile.
int main()
{
	using namespace HorizonTransportContract;

	// GET: session required, userId in the query, Bearer header.
	const FPlayerProfileRequestPlan GetPlan = BuildPlayerProfileGetPlan("user-881", "session-token-881");
	Require(GetPlan.bShouldSend, "signed user did not produce a get plan");
	Require(GetPlan.Verb == "GET", "get plan must use GET");
	Require(GetPlan.bUseSessionToken, "get plan must send the session token");
	Require(GetPlan.Endpoint == "/api/v1/app/player-profile?userId=user-881", "incorrect get endpoint");
	Require(GetPlan.BodyJson.empty(), "get plan must not have a body");
	Require(HasBearer("session-token-881", GetPlan.bUseSessionToken), "get headers are missing the bearer session");

	const FPlayerProfileRequestPlan GetWithoutSession = BuildPlayerProfileGetPlan("user-881", "");
	Require(!GetWithoutSession.bShouldSend, "missing session token did not block get");
	Require(GetWithoutSession.ErrorCode == "SESSION_REQUIRED", "missing session must give SESSION_REQUIRED");
	Require(!BuildPlayerProfileGetPlan("", "session-token-881").bShouldSend, "missing user id did not block get");

	// PUT: full body, empty slots as null, Bearer header.
	const FPlayerProfileRequestPlan SetPlan = BuildPlayerProfileSetPlan(
		"user-881", "session-token-881", "avatar.zombie_07", "", {"badge.supporter", "badge.beta"});
	Require(SetPlan.bShouldSend, "valid profile did not produce a set plan");
	Require(SetPlan.Verb == "PUT", "set plan must use PUT");
	Require(SetPlan.bUseSessionToken, "set plan must send the session token");
	Require(SetPlan.Endpoint == "/api/v1/app/player-profile", "incorrect set endpoint");
	Require(SetPlan.BodyJson ==
		"{\"userId\":\"user-881\",\"avatarId\":\"avatar.zombie_07\",\"frameId\":null,\"badges\":[\"badge.supporter\",\"badge.beta\"]}",
		"incorrect set body");
	Require(HasBearer("session-token-881", SetPlan.bUseSessionToken), "set headers are missing the bearer session");

	const FPlayerProfileRequestPlan ClearPlan = BuildPlayerProfileSetPlan("user-881", "session-token-881", "", "", {});
	Require(ClearPlan.bShouldSend, "clearing the profile did not produce a set plan");
	Require(ClearPlan.BodyJson == "{\"userId\":\"user-881\",\"avatarId\":null,\"frameId\":null,\"badges\":[]}",
		"clearing must send null slots and an empty badge list");

	// Local pre-checks: nothing is sent.
	const FPlayerProfileRequestPlan SetWithoutSession = BuildPlayerProfileSetPlan("user-881", "", "avatar.a", "", {});
	Require(!SetWithoutSession.bShouldSend && SetWithoutSession.ErrorCode == "SESSION_REQUIRED",
		"missing session must block set with SESSION_REQUIRED");

	const FPlayerProfileRequestPlan TooManyBadges = BuildPlayerProfileSetPlan(
		"user-881", "session-token-881", "", "", {"badge.a", "badge.b", "badge.c", "badge.d"});
	Require(!TooManyBadges.bShouldSend && TooManyBadges.ErrorCode == "INVALID_BADGES",
		"more than 3 badges must give INVALID_BADGES");

	const FPlayerProfileRequestPlan DuplicateBadges = BuildPlayerProfileSetPlan(
		"user-881", "session-token-881", "", "", {"badge.a", "badge.a"});
	Require(!DuplicateBadges.bShouldSend && DuplicateBadges.ErrorCode == "INVALID_BADGES",
		"a duplicate badge must give INVALID_BADGES");

	const FPlayerProfileRequestPlan InvalidAvatar = BuildPlayerProfileSetPlan(
		"user-881", "session-token-881", "Avatar Zombie", "", {});
	Require(!InvalidAvatar.bShouldSend && InvalidAvatar.ErrorCode == "INVALID_COSMETIC_ID",
		"an invalid avatar ID must give INVALID_COSMETIC_ID");

	// Cosmetic ID pattern ^[a-z0-9][a-z0-9._-]{0,31}$
	Require(IsValidCosmeticId("avatar.zombie_07"), "valid ID rejected");
	Require(IsValidCosmeticId("0-frame"), "ID starting with a digit rejected");
	Require(IsValidCosmeticId(std::string(32, 'a')), "32 character ID rejected");
	Require(!IsValidCosmeticId(std::string(33, 'a')), "33 character ID accepted");
	Require(!IsValidCosmeticId(""), "empty ID accepted");
	Require(!IsValidCosmeticId(".avatar"), "ID starting with a dot accepted");
	Require(!IsValidCosmeticId("Avatar"), "upper case ID accepted");
	Require(!IsValidCosmeticId("avatar zombie"), "ID with a space accepted");

	std::cout << "Unreal SDK player profile transport contract passed\n";
	return 0;
}
