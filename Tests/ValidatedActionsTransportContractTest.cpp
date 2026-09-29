#include "Transport/HorizonValidatedActionsTransportContract.h"

#include <cstdint>
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

	std::vector<std::uint8_t> Bytes(const std::string& Text)
	{
		return std::vector<std::uint8_t>(Text.begin(), Text.end());
	}

	bool HasBearer(const std::string& SessionToken, bool bUseSessionToken)
	{
		for (const auto& Header : HorizonTransportContract::BuildHeaders("project-key-883", SessionToken, bUseSessionToken))
		{
			if (Header.first == "Authorization" && Header.second == "Bearer " + SessionToken)
			{
				return true;
			}
		}
		return false;
	}
}

// Validated Actions (TASK-883, Part 1): input log hash, start run and submit plans, run lifecycle.
int main()
{
	using namespace HorizonTransportContract;

	// SHA-256, lower case hex (FIPS 180-4 test vectors plus padding edges at 55, 56 and 64 bytes).
	Require(ComputeInputLogHash({}) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "sha256 of empty input");
	Require(ComputeInputLogHash(Bytes("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 of abc");
	Require(ComputeInputLogHash(Bytes("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"))
		== "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", "sha256 of the two block vector");
	Require(ComputeInputLogHash(Bytes(std::string(55, 'a'))) == "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318", "sha256 of 55 bytes");
	Require(ComputeInputLogHash(Bytes(std::string(56, 'a'))) == "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a", "sha256 of 56 bytes");
	Require(ComputeInputLogHash(Bytes(std::string(64, 'a'))) == "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb", "sha256 of 64 bytes");
	std::vector<std::uint8_t> AllBytes;
	for (int Round = 0; Round < 2; ++Round)
	{
		for (int Value = 0; Value < 256; ++Value)
		{
			AllBytes.push_back(static_cast<std::uint8_t>(Value));
		}
	}
	Require(ComputeInputLogHash(AllBytes) == "110009dcee21620b166f3abfecb5eff7a873be729d1c2d53822e7acc5f34eb9b", "sha256 of binary bytes");

	// Hash format.
	const std::string ValidHash = ComputeInputLogHash(Bytes("abc"));
	Require(IsValidInputLogHash(ValidHash), "valid lower case hash rejected");
	Require(IsValidInputLogHash("BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"), "upper case hash rejected");
	Require(!IsValidInputLogHash(ValidHash.substr(1)), "63 characters accepted");
	Require(!IsValidInputLogHash(ValidHash + "0"), "65 characters accepted");
	Require(!IsValidInputLogHash(std::string(63, 'a') + "g"), "non hex character accepted");
	Require(!IsValidInputLogHash(""), "empty hash accepted");

	// Start run: session required, Bearer, leaderboardKey omitted when empty.
	const FValidatedRequestPlan StartPlan = BuildValidatedStartRunPlan("user-883", "session-token-883", "weekly");
	Require(StartPlan.bShouldSend, "signed user did not produce a start plan");
	Require(StartPlan.Verb == "POST", "start plan must use POST");
	Require(StartPlan.bUseSessionToken, "start plan must send the session token");
	Require(StartPlan.Endpoint == "/api/v1/app/validated-actions/runs", "incorrect start endpoint");
	Require(StartPlan.BodyJson == "{\"userId\":\"user-883\",\"leaderboardKey\":\"weekly\"}", "incorrect start body");
	Require(HasBearer("session-token-883", StartPlan.bUseSessionToken), "start headers are missing the bearer session");

	const FValidatedRequestPlan UnboundStart = BuildValidatedStartRunPlan("user-883", "session-token-883", "  ");
	Require(UnboundStart.BodyJson == "{\"userId\":\"user-883\"}", "blank leaderboard key must be omitted");

	const FValidatedRequestPlan StartWithoutSession = BuildValidatedStartRunPlan("user-883", "", "weekly");
	Require(!StartWithoutSession.bShouldSend && StartWithoutSession.ErrorCode == "SESSION_REQUIRED",
		"missing session must block start with SESSION_REQUIRED");
	Require(!BuildValidatedStartRunPlan("", "session-token-883", "").bShouldSend, "missing user id did not block start");

	// Submit: full body, optional fields omitted when empty, hash lower cased.
	const FValidatedRequestPlan SubmitPlan = BuildValidatedSubmitPlan(
		"user-883", "session-token-883", "hzn-rt1:2026-09:abc", ValidHash, 18250, "wave_10", "weekly",
		{{"gold", 250}, {"chest.gold", -1}});
	Require(SubmitPlan.bShouldSend, "valid submit did not produce a plan");
	Require(SubmitPlan.Verb == "POST", "submit plan must use POST");
	Require(SubmitPlan.bUseSessionToken, "submit plan must send the session token");
	Require(SubmitPlan.Endpoint == "/api/v1/app/validated-actions/submit", "incorrect submit endpoint");
	Require(SubmitPlan.BodyJson ==
		"{\"userId\":\"user-883\",\"ticket\":\"hzn-rt1:2026-09:abc\",\"inputLogHash\":\"" + ValidHash + "\""
		",\"score\":18250,\"stage\":\"wave_10\",\"leaderboardKey\":\"weekly\""
		",\"earned\":[{\"key\":\"gold\",\"amount\":250},{\"key\":\"chest.gold\",\"amount\":-1}]}",
		"incorrect submit body");
	Require(HasBearer("session-token-883", SubmitPlan.bUseSessionToken), "submit headers are missing the bearer session");

	const FValidatedRequestPlan MinimalSubmit = BuildValidatedSubmitPlan(
		"user-883", "session-token-883", "ticket-1",
		"BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD", 0, "", " ", {});
	Require(MinimalSubmit.bShouldSend, "minimal submit did not produce a plan");
	Require(MinimalSubmit.BodyJson ==
		"{\"userId\":\"user-883\",\"ticket\":\"ticket-1\",\"inputLogHash\":\"" + ValidHash + "\",\"score\":0}",
		"minimal submit must omit stage, leaderboardKey and earned and lower case the hash");

	const FValidatedRequestPlan MaxScore = BuildValidatedSubmitPlan(
		"user-883", "session-token-883", "ticket-1", ValidHash, 9007199254740991LL, "", "", {});
	Require(MaxScore.BodyJson.find("\"score\":9007199254740991") != std::string::npos, "largest score must be sent exactly");

	// Local checks in order: SESSION_REQUIRED, NO_ACTIVE_RUN, INVALID_INPUT_LOG_HASH.
	const FValidatedRequestPlan SubmitWithoutSession = BuildValidatedSubmitPlan(
		"user-883", "", "", "not-a-hash", 1, "", "", {});
	Require(!SubmitWithoutSession.bShouldSend && SubmitWithoutSession.ErrorCode == "SESSION_REQUIRED",
		"missing session must block submit with SESSION_REQUIRED first");

	const FValidatedRequestPlan SubmitWithoutRun = BuildValidatedSubmitPlan(
		"user-883", "session-token-883", "", ValidHash, 1, "", "", {});
	Require(!SubmitWithoutRun.bShouldSend && SubmitWithoutRun.ErrorCode == "NO_ACTIVE_RUN",
		"missing ticket must give NO_ACTIVE_RUN");

	const FValidatedRequestPlan SubmitBadHash = BuildValidatedSubmitPlan(
		"user-883", "session-token-883", "ticket-1", "abc", 1, "", "", {});
	Require(!SubmitBadHash.bShouldSend && SubmitBadHash.ErrorCode == "INVALID_INPUT_LOG_HASH",
		"malformed hash must give INVALID_INPUT_LOG_HASH");

	// Run lifecycle: the ticket is single use.
	Require(ShouldClearRunAfterSubmit(200, ""), "accepted run must clear the current run");
	Require(ShouldClearRunAfterSubmit(422, "DURATION_TOO_SHORT"), "rule rejection must clear the current run");
	Require(ShouldClearRunAfterSubmit(422, "TICKET_EXPIRED"), "ticket rejection must clear the current run");
	Require(ShouldClearRunAfterSubmit(403, "SCORE_LIMIT_REACHED"), "SCORE_LIMIT_REACHED must clear the current run");
	Require(!ShouldClearRunAfterSubmit(0, ""), "network error must keep the run");
	Require(!ShouldClearRunAfterSubmit(401, "SESSION_REQUIRED"), "401 must keep the run");
	Require(!ShouldClearRunAfterSubmit(403, "SESSION_FORBIDDEN"), "other 403 must keep the run");
	Require(!ShouldClearRunAfterSubmit(404, "PLAYER_NOT_FOUND"), "404 must keep the run");
	Require(!ShouldClearRunAfterSubmit(400, "PLAYER_NAME_REQUIRED"), "400 must keep the run");
	Require(!ShouldClearRunAfterSubmit(429, "RUN_RATE_LIMITED"), "429 must keep the run");
	Require(!ShouldClearRunAfterSubmit(503, "VALIDATED_ACTIONS_UNAVAILABLE"), "503 must keep the run");

	// Run limits are not retried automatically; the account request limit (no code) is.
	Require(IsNonRetryableRateLimitCode("RUN_RATE_LIMITED"), "RUN_RATE_LIMITED must not be retried");
	Require(IsNonRetryableRateLimitCode("RUN_CAPACITY_REACHED"), "RUN_CAPACITY_REACHED must not be retried");
	Require(!IsNonRetryableRateLimitCode(""), "a 429 without code keeps the Retry-After handling");

	// Error codes: server code first, NOT_SUPPORTED for a bare 404, HTTP mapping otherwise.
	Require(MapValidatedErrorCode(422, "SCORE_ABOVE_MAX", "UNKNOWN") == "SCORE_ABOVE_MAX", "server code must win");
	Require(MapValidatedErrorCode(404, "LEADERBOARD_NOT_FOUND", "NOT_FOUND") == "LEADERBOARD_NOT_FOUND", "404 with code keeps the code");
	Require(MapValidatedErrorCode(404, "", "NOT_FOUND") == "NOT_SUPPORTED", "404 without code must give NOT_SUPPORTED");
	Require(MapValidatedErrorCode(429, "", "RATE_LIMITED") == "RATE_LIMITED", "429 without code keeps the HTTP mapping");
	Require(MapValidatedErrorCode(0, "", "CONNECTION_FAILED") == "CONNECTION_FAILED", "network error keeps the HTTP mapping");

	std::cout << "Unreal SDK validated actions transport contract passed\n";
	return 0;
}
