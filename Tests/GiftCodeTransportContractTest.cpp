#include "Transport/HorizonGiftCodeTransportContract.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	void Require(bool Condition, const char* Message)
	{
		if (!Condition)
		{
			throw std::runtime_error(Message);
		}
	}
}

// Gift code redemption is bound to the player's Bearer session on the server (TASK-886).
int main()
{
	using namespace HorizonTransportContract;

	const FGiftCodeRedeemPlan Plan = BuildGiftCodeRedeemPlan("user-886", "session-token-886", "SUMMER2026");
	Require(Plan.bShouldSend, "signed user did not produce a redeem plan");
	Require(Plan.bUseSessionToken, "redeem must send the session token");
	Require(Plan.Endpoint == "/api/v1/app/gift-codes/redeem", "incorrect redeem endpoint");
	Require(Plan.BodyJson == "{\"code\":\"SUMMER2026\",\"userId\":\"user-886\"}", "incorrect redeem body");

	bool bHasBearer = false;
	for (const auto& Header : BuildHeaders("project-key-886", "session-token-886", Plan.bUseSessionToken))
	{
		if (Header.first == "Authorization" && Header.second == "Bearer session-token-886")
		{
			bHasBearer = true;
		}
	}
	Require(bHasBearer, "redeem headers are missing the bearer session");

	Require(!BuildGiftCodeRedeemPlan("", "session-token-886", "SUMMER2026").bShouldSend,
		"missing user id did not block redeem");
	Require(!BuildGiftCodeRedeemPlan("user-886", "", "SUMMER2026").bShouldSend,
		"missing session token did not block redeem");
	Require(!BuildGiftCodeRedeemPlan("user-886", "session-token-886", "").bShouldSend,
		"empty code did not block redeem");
	Require(BuildGiftCodeRedeemPlan("user-886", "session-token-886", "A\"B").BodyJson ==
		"{\"code\":\"A\\\"B\",\"userId\":\"user-886\"}", "code is not JSON escaped");

	std::cout << "Unreal SDK gift code transport contract passed\n";
	return 0;
}
