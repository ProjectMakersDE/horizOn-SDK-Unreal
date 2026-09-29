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

// Validated Actions (TASK-883 Part 1, TASK-887 Part 2, TASK-888 Part 3): input log hash, start
// run, submit, player state and evidence upload plans, run lifecycle, value codes, base64 and
// the upload retry rule.
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
	Require(ShouldClearRunAfterSubmit(422, "TICKET_INVALID"), "TICKET_INVALID must clear the current run");
	Require(ShouldClearRunAfterSubmit(422, "TICKET_FOREIGN"), "TICKET_FOREIGN must clear the current run");
	Require(ShouldClearRunAfterSubmit(422, "TICKET_CONSUMED"), "TICKET_CONSUMED must clear the current run");
	Require(ShouldClearRunAfterSubmit(422, "INSUFFICIENT_BALANCE"), "value rejection must clear the current run");
	// Checked before the ticket is consumed: the game may resubmit with the same ticket.
	Require(!ShouldClearRunAfterSubmit(422, "LEADERBOARD_MISMATCH"), "LEADERBOARD_MISMATCH must keep the run");
	Require(!ShouldClearRunAfterSubmit(404, "LEADERBOARD_NOT_FOUND"), "LEADERBOARD_NOT_FOUND must keep the run");
	Require(!ShouldClearRunAfterSubmit(400, "SCORE_REQUIRED"), "SCORE_REQUIRED must keep the run");
	Require(!ShouldClearRunAfterSubmit(400, "PLAYER_NAME_REQUIRED"), "PLAYER_NAME_REQUIRED must keep the run");
	Require(!ShouldClearRunAfterSubmit(429, "RUN_RATE_LIMITED"), "429 must keep the run");
	Require(!ShouldClearRunAfterSubmit(503, "VALIDATED_ACTIONS_UNAVAILABLE"), "503 must keep the run");

	// Part 2: player state read. GET with the Bearer session, userId in the query, no body.
	const FValidatedRequestPlan StatePlan = BuildValidatedGetStatePlan("0d7e1c2a-5b3f-4e8a-9c1d-2f3e4a5b6c7d", "session-token-887");
	Require(StatePlan.bShouldSend, "signed user did not produce a state plan");
	Require(StatePlan.Verb == "GET", "state plan must use GET");
	Require(StatePlan.bUseSessionToken, "state plan must send the session token");
	Require(StatePlan.Endpoint == "/api/v1/app/validated-actions/state?userId=0d7e1c2a-5b3f-4e8a-9c1d-2f3e4a5b6c7d",
		"incorrect state endpoint");
	Require(StatePlan.BodyJson.empty(), "state plan must not send a body");
	Require(HasBearer("session-token-887", StatePlan.bUseSessionToken), "state headers are missing the bearer session");
	Require(BuildValidatedGetStatePlan("user 887&x", "session-token-887").Endpoint
		== "/api/v1/app/validated-actions/state?userId=user%20887%26x", "state user id must be URL encoded");

	const FValidatedRequestPlan StateWithoutSession = BuildValidatedGetStatePlan("user-887", "");
	Require(!StateWithoutSession.bShouldSend && StateWithoutSession.ErrorCode == "SESSION_REQUIRED",
		"missing session must block the state read with SESSION_REQUIRED");
	Require(!BuildValidatedGetStatePlan("", "session-token-887").bShouldSend, "missing user id did not block the state read");

	// Part 2: earned values are sent with int64 amounts, spends negative, largest amount exact.
	const FValidatedRequestPlan EarnedPlan = BuildValidatedSubmitPlan(
		"user-887", "session-token-887", "ticket-887", ValidHash, 0, "", "",
		{{"gold", 9007199254740991LL}, {"chest.gold", -9007199254740991LL}});
	Require(EarnedPlan.BodyJson.find(
		"\"earned\":[{\"key\":\"gold\",\"amount\":9007199254740991},{\"key\":\"chest.gold\",\"amount\":-9007199254740991}]")
		!= std::string::npos, "earned amounts must be sent exactly as int64");
	Require(EarnedPlan.BodyJson.find("leaderboardKey") == std::string::npos, "a run without board must omit leaderboardKey");

	// Part 2: value rejections are rule rejections, the ticket is consumed.
	Require(IsValueRejectionCode("UNKNOWN_VALUE_KEY"), "UNKNOWN_VALUE_KEY is a value rejection");
	Require(IsValueRejectionCode("DUPLICATE_VALUE_KEY"), "DUPLICATE_VALUE_KEY is a value rejection");
	Require(IsValueRejectionCode("EARNED_ABOVE_MAX"), "EARNED_ABOVE_MAX is a value rejection");
	Require(IsValueRejectionCode("EARNED_BELOW_MIN"), "EARNED_BELOW_MIN is a value rejection");
	Require(IsValueRejectionCode("INSUFFICIENT_BALANCE"), "INSUFFICIENT_BALANCE is a value rejection");
	Require(!IsValueRejectionCode("SCORE_ABOVE_MAX"), "SCORE_ABOVE_MAX is not a value rejection");
	Require(ShouldClearRunAfterSubmit(422, "UNKNOWN_VALUE_KEY"), "UNKNOWN_VALUE_KEY must clear the current run");
	Require(ShouldClearRunAfterSubmit(422, "EARNED_ABOVE_MAX"), "EARNED_ABOVE_MAX must clear the current run");

	// Part 2: a purchase is granted only when the spend was applied in full.
	Require(IsFullyCredited(-500, -500), "a full spend is fully credited");
	Require(!IsFullyCredited(-500, 0), "a spend a concurrent run made unaffordable is not credited");
	Require(!IsFullyCredited(250, 100), "a clamped credit is not fully credited");
	Require(IsFullyCredited(0, 0), "an untouched value counts as fully credited");

	// Run limits are not retried automatically; the account request limit (no code) is.
	Require(IsNonRetryableRateLimitCode("RUN_RATE_LIMITED"), "RUN_RATE_LIMITED must not be retried");
	Require(IsNonRetryableRateLimitCode("RUN_CAPACITY_REACHED"), "RUN_CAPACITY_REACHED must not be retried");
	Require(!IsNonRetryableRateLimitCode(""), "a 429 without code keeps the Retry-After handling");

	// Error codes: server code first, NOT_SUPPORTED for a bare 404, HTTP mapping otherwise.
	Require(MapValidatedErrorCode(422, "SCORE_ABOVE_MAX", "UNKNOWN") == "SCORE_ABOVE_MAX", "server code must win");
	Require(MapValidatedErrorCode(404, "LEADERBOARD_NOT_FOUND", "NOT_FOUND") == "LEADERBOARD_NOT_FOUND", "404 with code keeps the code");
	Require(MapValidatedErrorCode(404, "", "NOT_FOUND") == "NOT_SUPPORTED", "404 without code must give NOT_SUPPORTED");
	Require(MapValidatedErrorCode(404, "NOT_FOUND", "NOT_FOUND") == "NOT_SUPPORTED", "404 with the generic NOT_FOUND code must give NOT_SUPPORTED");
	Require(MapValidatedErrorCode(429, "", "RATE_LIMITED") == "RATE_LIMITED", "429 without code keeps the HTTP mapping");
	Require(MapValidatedErrorCode(0, "", "CONNECTION_FAILED") == "CONNECTION_FAILED", "network error keeps the HTTP mapping");

	// Part 3: standard base64 with padding (RFC 4648 test vectors plus binary bytes).
	Require(Base64Encode(Bytes("")) == "", "base64 of empty input");
	Require(Base64Encode(Bytes("f")) == "Zg==", "base64 of f");
	Require(Base64Encode(Bytes("fo")) == "Zm8=", "base64 of fo");
	Require(Base64Encode(Bytes("foo")) == "Zm9v", "base64 of foo");
	Require(Base64Encode(Bytes("foob")) == "Zm9vYg==", "base64 of foob");
	Require(Base64Encode(Bytes("fooba")) == "Zm9vYmE=", "base64 of fooba");
	Require(Base64Encode(Bytes("foobar")) == "Zm9vYmFy", "base64 of foobar");
	Require(Base64Encode(std::vector<std::uint8_t>{0xFB, 0xFF, 0xBF}) == "+/+/", "base64 must use the standard alphabet (+ and /)");
	Require(Base64Encode(std::vector<std::uint8_t>{0x00, 0x01, 0x02, 0x03}) == "AAECAw==", "base64 of binary bytes");

	// Part 3: evidence upload. PUT with the Bearer session, run ID in the path, log as base64 of
	// the same bytes whose SHA-256 was submitted.
	const std::vector<std::uint8_t> EvidenceLog{0x00, 0x03, 0x01, 0x02, 0xFF};
	const FValidatedRequestPlan EvidencePlan = BuildValidatedEvidenceUploadPlan(
		"user-888", "session-token-888", "5f1c2d3e-4a5b-4c6d-8e7f-9a0b1c2d3e4f", EvidenceLog, 32768);
	Require(EvidencePlan.bShouldSend, "valid evidence upload did not produce a plan");
	Require(EvidencePlan.Verb == "PUT", "evidence plan must use PUT");
	Require(EvidencePlan.bUseSessionToken, "evidence plan must send the session token");
	Require(EvidencePlan.Endpoint == "/api/v1/app/validated-actions/runs/5f1c2d3e-4a5b-4c6d-8e7f-9a0b1c2d3e4f/evidence",
		"incorrect evidence endpoint");
	Require(EvidencePlan.BodyJson == "{\"userId\":\"user-888\",\"log\":\"AAMBAv8=\"}", "incorrect evidence body");
	Require(HasBearer("session-token-888", EvidencePlan.bUseSessionToken), "evidence headers are missing the bearer session");
	Require(BuildValidatedEvidenceUploadPlan("user-888", "session-token-888", " run/1 ", EvidenceLog, 0).Endpoint
		== "/api/v1/app/validated-actions/runs/run%2F1/evidence", "evidence run id must be trimmed and URL encoded");

	// The largest accepted log (maxBytes) is sent; one byte more fails locally. MaxBytes 0 = unknown, the server decides.
	const std::vector<std::uint8_t> MaxLog(32768, 0x2A);
	const FValidatedRequestPlan MaxLogPlan = BuildValidatedEvidenceUploadPlan("user-888", "session-token-888", "run-1", MaxLog, 32768);
	Require(MaxLogPlan.bShouldSend, "a log of exactly maxBytes must be sent");
	Require(MaxLogPlan.BodyJson.size() == std::string("{\"userId\":\"user-888\",\"log\":\"\"}").size() + 43692,
		"a 32768 byte log must be 43692 base64 characters");
	const std::vector<std::uint8_t> TooLargeLog(32769, 0x2A);
	const FValidatedRequestPlan TooLargePlan = BuildValidatedEvidenceUploadPlan("user-888", "session-token-888", "run-1", TooLargeLog, 32768);
	Require(!TooLargePlan.bShouldSend && TooLargePlan.ErrorCode == "EVIDENCE_TOO_LARGE", "a log above maxBytes must fail locally");
	Require(BuildValidatedEvidenceUploadPlan("user-888", "session-token-888", "run-1", TooLargeLog, 0).bShouldSend,
		"without maxBytes the size is left to the server");

	// Local checks in order: SESSION_REQUIRED, INVALID_RUN_ID, EMPTY_INPUT_LOG, EVIDENCE_TOO_LARGE.
	const FValidatedRequestPlan EvidenceWithoutSession = BuildValidatedEvidenceUploadPlan("user-888", "", "", {}, 1);
	Require(!EvidenceWithoutSession.bShouldSend && EvidenceWithoutSession.ErrorCode == "SESSION_REQUIRED",
		"missing session must block the upload with SESSION_REQUIRED first");
	Require(!BuildValidatedEvidenceUploadPlan("", "session-token-888", "run-1", EvidenceLog, 0).bShouldSend,
		"missing user id did not block the upload");
	const FValidatedRequestPlan EvidenceWithoutRun = BuildValidatedEvidenceUploadPlan("user-888", "session-token-888", "  ", {}, 1);
	Require(!EvidenceWithoutRun.bShouldSend && EvidenceWithoutRun.ErrorCode == "INVALID_RUN_ID",
		"blank run id must give INVALID_RUN_ID");
	const FValidatedRequestPlan EmptyEvidence = BuildValidatedEvidenceUploadPlan("user-888", "session-token-888", "run-1", {}, 1);
	Require(!EmptyEvidence.bShouldSend && EmptyEvidence.ErrorCode == "EMPTY_INPUT_LOG", "empty log must give EMPTY_INPUT_LOG");

	// Retry an upload only after 422 EVIDENCE_HASH_MISMATCH or a network error.
	Require(IsEvidenceUploadRetryable("EVIDENCE_HASH_MISMATCH"), "EVIDENCE_HASH_MISMATCH may be retried with the correct bytes");
	Require(IsEvidenceUploadRetryable("CONNECTION_FAILED"), "a network error may be retried");
	Require(!IsEvidenceUploadRetryable("SERVER_ERROR"), "a 5xx is final for the game (the HTTP client already retried it)");
	Require(!IsEvidenceUploadRetryable("EVIDENCE_ALREADY_UPLOADED"), "409 is final");
	Require(!IsEvidenceUploadRetryable("EVIDENCE_EXPIRED"), "410 is final");
	Require(!IsEvidenceUploadRetryable("EVIDENCE_TOO_LARGE"), "413 is final");
	Require(!IsEvidenceUploadRetryable("EVIDENCE_INVALID_ENCODING"), "400 EVIDENCE_INVALID_ENCODING is final");
	Require(!IsEvidenceUploadRetryable("EVIDENCE_NOT_REQUESTED"), "404 EVIDENCE_NOT_REQUESTED is final");
	Require(!IsEvidenceUploadRetryable("SESSION_REQUIRED"), "a local session error is final");
	Require(!IsEvidenceUploadRetryable("EMPTY_INPUT_LOG"), "a local input error is final");
	Require(MapValidatedErrorCode(404, "EVIDENCE_NOT_REQUESTED", "NOT_FOUND") == "EVIDENCE_NOT_REQUESTED",
		"404 EVIDENCE_NOT_REQUESTED keeps its code");
	Require(MapValidatedErrorCode(413, "EVIDENCE_TOO_LARGE", "UNKNOWN") == "EVIDENCE_TOO_LARGE", "413 keeps its code");

	// Part 3: PLAYER_BANNED is checked before the ticket is consumed, so the run stays.
	Require(!ShouldClearRunAfterSubmit(403, "PLAYER_BANNED"), "PLAYER_BANNED must keep the run");
	Require(MapValidatedErrorCode(403, "PLAYER_BANNED", "FORBIDDEN") == "PLAYER_BANNED", "PLAYER_BANNED keeps its code");

	std::cout << "Unreal SDK validated actions transport contract passed\n";
	return 0;
}
