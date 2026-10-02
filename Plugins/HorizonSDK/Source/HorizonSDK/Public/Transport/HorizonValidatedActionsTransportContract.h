// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "Transport/HorizonLeaderboardTransportContract.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

/**
 * Engine free transport contract of Validated Actions (TASK-883 Part 1, TASK-887 Part 2,
 * TASK-888 Part 3, TASK-911 run start context).
 *
 * Holds everything that decides what goes over the wire so it can be compiled and
 * checked without the engine: the SHA-256 input log hash, the local pre-checks
 * (SESSION_REQUIRED, NO_ACTIVE_RUN, INVALID_INPUT_LOG_HASH), the request plans of
 * start run, submit, (Part 2) the player state read and (Part 3) the evidence upload
 * with its standard base64 encoding, (TASK-911) the optional run start context, and the rules
 * for keeping or clearing the current run and for retrying an upload.
 */
namespace HorizonTransportContract
{
	/** Length of a SHA-256 hash as hex characters. */
	constexpr std::size_t InputLogHashLength = 64;

	/** Endpoints of Part 1. */
	constexpr const char* ValidatedStartRunEndpoint = "/api/v1/app/validated-actions/runs";
	constexpr const char* ValidatedSubmitEndpoint = "/api/v1/app/validated-actions/submit";

	/** Endpoint of Part 2: the signed-in player's server-owned values (read only). */
	constexpr const char* ValidatedStateEndpoint = "/api/v1/app/validated-actions/state";

	/** Endpoint of Part 3: PUT {prefix}{runId}{suffix} uploads the input log of a run as evidence. */
	constexpr const char* ValidatedEvidenceEndpointPrefix = "/api/v1/app/validated-actions/runs/";
	constexpr const char* ValidatedEvidenceEndpointSuffix = "/evidence";

	/** Local error codes (no request is sent). */
	constexpr const char* ValidatedCodeSessionRequired = "SESSION_REQUIRED";
	constexpr const char* ValidatedCodeNoActiveRun = "NO_ACTIVE_RUN";
	constexpr const char* ValidatedCodeInvalidInputLogHash = "INVALID_INPUT_LOG_HASH";

	/** Local error codes of the evidence upload (Part 3, no request is sent). */
	constexpr const char* ValidatedCodeInvalidRunId = "INVALID_RUN_ID";
	constexpr const char* ValidatedCodeEmptyInputLog = "EMPTY_INPUT_LOG";

	/** Local error code of the run start context (TASK-911): a set content digest that is not 64 hex characters. */
	constexpr const char* ValidatedCodeInvalidContentDigest = "INVALID_CONTENT_DIGEST";

	/** Server codes of the run start context (TASK-911): the run is not started, both are final. */
	constexpr const char* ValidatedCodeInitialStateInvalidEncoding = "INITIAL_STATE_INVALID_ENCODING";
	constexpr const char* ValidatedCodeInitialStateTooLarge = "INITIAL_STATE_TOO_LARGE";

	/** A 404 without a server code (for example a simpleServer) means the feature is missing. */
	constexpr const char* ValidatedCodeNotSupported = "NOT_SUPPORTED";

	/** Server codes the SDK acts on (the full list is in the README). */
	constexpr const char* ValidatedCodeLeaderboardMismatch = "LEADERBOARD_MISMATCH";
	constexpr const char* ValidatedCodeScoreLimitReached = "SCORE_LIMIT_REACHED";
	constexpr const char* ValidatedCodeRunRateLimited = "RUN_RATE_LIMITED";
	constexpr const char* ValidatedCodeRunCapacityReached = "RUN_CAPACITY_REACHED";

	/**
	 * Part 2 codes of the submit (422, the ticket is consumed with status REJECTED): an
	 * `earned` key the rules do not define (also when the rules define no values), a key
	 * twice, an amount above `maxPerRun` or below `minPerRun`, a spend larger than the balance.
	 */
	constexpr const char* ValidatedCodeUnknownValueKey = "UNKNOWN_VALUE_KEY";
	constexpr const char* ValidatedCodeDuplicateValueKey = "DUPLICATE_VALUE_KEY";
	constexpr const char* ValidatedCodeEarnedAboveMax = "EARNED_ABOVE_MAX";
	constexpr const char* ValidatedCodeEarnedBelowMin = "EARNED_BELOW_MIN";
	constexpr const char* ValidatedCodeInsufficientBalance = "INSUFFICIENT_BALANCE";

	/**
	 * Part 3 codes. PLAYER_BANNED (403) answers both the plain SubmitScore and the validated
	 * submit of a player banned from the board; the validated submit checks it before the ticket
	 * is consumed. The evidence codes answer the upload; EVIDENCE_TOO_LARGE is also the local
	 * code when the log is larger than the `maxBytes` of the evidence request.
	 */
	constexpr const char* ValidatedCodePlayerBanned = "PLAYER_BANNED";
	constexpr const char* ValidatedCodeEvidenceInvalidEncoding = "EVIDENCE_INVALID_ENCODING";
	constexpr const char* ValidatedCodeEvidenceNotRequested = "EVIDENCE_NOT_REQUESTED";
	constexpr const char* ValidatedCodeEvidenceAlreadyUploaded = "EVIDENCE_ALREADY_UPLOADED";
	constexpr const char* ValidatedCodeEvidenceExpired = "EVIDENCE_EXPIRED";
	constexpr const char* ValidatedCodeEvidenceTooLarge = "EVIDENCE_TOO_LARGE";
	constexpr const char* ValidatedCodeEvidenceHashMismatch = "EVIDENCE_HASH_MISMATCH";

	/** HTTP mapping of the SDK for a network error (FHorizonNetworkResponse, NETWORK_ERROR in Unity and Godot). */
	constexpr const char* HttpCodeConnectionFailed = "CONNECTION_FAILED";

	/** Largest number of `earned` entries per submit (the server answers 400 without code above it). */
	constexpr std::size_t MaxEarnedValuesPerRun = 64;

	namespace ValidatedActionsDetail
	{
		inline std::uint32_t RotateRight(std::uint32_t Value, unsigned int Bits)
		{
			return (Value >> Bits) | (Value << (32u - Bits));
		}

		/** One SHA-256 compression round over a 64 byte block (FIPS 180-4, section 6.2.2). */
		inline void Sha256ProcessBlock(std::uint32_t (&State)[8], const std::uint8_t* Block)
		{
			static const std::uint32_t RoundConstants[64] = {
				0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
				0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
				0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
				0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
				0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
				0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
				0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
				0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
			};

			std::uint32_t Schedule[64];
			for (std::size_t Index = 0; Index < 16; ++Index)
			{
				Schedule[Index] =
					(static_cast<std::uint32_t>(Block[Index * 4]) << 24)
					| (static_cast<std::uint32_t>(Block[Index * 4 + 1]) << 16)
					| (static_cast<std::uint32_t>(Block[Index * 4 + 2]) << 8)
					| static_cast<std::uint32_t>(Block[Index * 4 + 3]);
			}
			for (std::size_t Index = 16; Index < 64; ++Index)
			{
				const std::uint32_t Sigma0 = RotateRight(Schedule[Index - 15], 7)
					^ RotateRight(Schedule[Index - 15], 18) ^ (Schedule[Index - 15] >> 3);
				const std::uint32_t Sigma1 = RotateRight(Schedule[Index - 2], 17)
					^ RotateRight(Schedule[Index - 2], 19) ^ (Schedule[Index - 2] >> 10);
				Schedule[Index] = Schedule[Index - 16] + Sigma0 + Schedule[Index - 7] + Sigma1;
			}

			std::uint32_t A = State[0];
			std::uint32_t B = State[1];
			std::uint32_t C = State[2];
			std::uint32_t D = State[3];
			std::uint32_t E = State[4];
			std::uint32_t F = State[5];
			std::uint32_t G = State[6];
			std::uint32_t H = State[7];

			for (std::size_t Index = 0; Index < 64; ++Index)
			{
				const std::uint32_t BigSigma1 = RotateRight(E, 6) ^ RotateRight(E, 11) ^ RotateRight(E, 25);
				const std::uint32_t Choose = (E & F) ^ (~E & G);
				const std::uint32_t Temp1 = H + BigSigma1 + Choose + RoundConstants[Index] + Schedule[Index];
				const std::uint32_t BigSigma0 = RotateRight(A, 2) ^ RotateRight(A, 13) ^ RotateRight(A, 22);
				const std::uint32_t Majority = (A & B) ^ (A & C) ^ (B & C);
				const std::uint32_t Temp2 = BigSigma0 + Majority;

				H = G;
				G = F;
				F = E;
				E = D + Temp1;
				D = C;
				C = B;
				B = A;
				A = Temp1 + Temp2;
			}

			State[0] += A;
			State[1] += B;
			State[2] += C;
			State[3] += D;
			State[4] += E;
			State[5] += F;
			State[6] += G;
			State[7] += H;
		}
	}

	/**
	 * SHA-256 of raw bytes as 64 lower case hex characters.
	 *
	 * Self-contained on purpose: the engine has no portable SHA-256 in Core
	 * (FGenericPlatformMisc::GetSHA256Signature is not implemented on every platform,
	 * the OpenSSL hasher lives in the optional PlatformCrypto plugin). A plain
	 * implementation gives the same hash on every target and can be checked without
	 * the engine.
	 */
	inline std::string Sha256Hex(const std::uint8_t* Data, std::size_t Length)
	{
		std::uint32_t State[8] = {
			0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
			0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
		};

		std::size_t Offset = 0;
		while (Length - Offset >= 64)
		{
			ValidatedActionsDetail::Sha256ProcessBlock(State, Data + Offset);
			Offset += 64;
		}

		// Padding: 0x80, zeros, then the message length in bits as 64 bit big endian.
		const std::size_t Remaining = Length - Offset;
		std::uint8_t Tail[128] = {};
		if (Remaining > 0)
		{
			std::memcpy(Tail, Data + Offset, Remaining);
		}
		Tail[Remaining] = 0x80;
		const std::size_t TailLength = Remaining < 56 ? 64 : 128;
		const std::uint64_t BitLength = static_cast<std::uint64_t>(Length) * 8u;
		for (std::size_t Index = 0; Index < 8; ++Index)
		{
			Tail[TailLength - 1 - Index] = static_cast<std::uint8_t>(BitLength >> (8 * Index));
		}
		ValidatedActionsDetail::Sha256ProcessBlock(State, Tail);
		if (TailLength == 128)
		{
			ValidatedActionsDetail::Sha256ProcessBlock(State, Tail + 64);
		}

		static const char HexDigits[] = "0123456789abcdef";
		std::string Hex;
		Hex.reserve(InputLogHashLength);
		for (const std::uint32_t Word : State)
		{
			for (int Shift = 28; Shift >= 0; Shift -= 4)
			{
				Hex.push_back(HexDigits[(Word >> Shift) & 0xFu]);
			}
		}
		return Hex;
	}

	/** SHA-256 hash of an input log (the same bytes are uploaded as evidence in Part 3). */
	inline std::string ComputeInputLogHash(const std::vector<std::uint8_t>& InputLog)
	{
		return Sha256Hex(InputLog.empty() ? nullptr : InputLog.data(), InputLog.size());
	}

	/** True for exactly 64 hex characters (upper case accepted, like the server). */
	inline bool IsValidInputLogHash(const std::string& Hash)
	{
		if (Hash.size() != InputLogHashLength)
		{
			return false;
		}
		for (const unsigned char Character : Hash)
		{
			const bool bHex = (Character >= '0' && Character <= '9')
				|| (Character >= 'a' && Character <= 'f')
				|| (Character >= 'A' && Character <= 'F');
			if (!bHex)
			{
				return false;
			}
		}
		return true;
	}

	/** Lower case copy of a hash, so every SDK sends the same spelling. */
	inline std::string NormalizeInputLogHash(const std::string& Hash)
	{
		std::string Result = Hash;
		for (char& Character : Result)
		{
			if (Character >= 'A' && Character <= 'F')
			{
				Character = static_cast<char>(Character - 'A' + 'a');
			}
		}
		return Result;
	}

	/** One earned (positive) or spent (negative) value of a run. Sent from Part 1 on, used by Part 2 servers. */
	struct FValidatedEarnedValue
	{
		std::string Key;
		std::int64_t Amount = 0;
	};

	/**
	 * Request plan of a Validated Actions call. When a local check fails, bShouldSend
	 * stays false and ErrorCode / ErrorMessage explain why; nothing reaches the server.
	 */
	struct FValidatedRequestPlan
	{
		bool bShouldSend = false;
		bool bUseSessionToken = true;
		std::string Verb;
		std::string Endpoint;
		std::string BodyJson;
		std::string ErrorCode;
		std::string ErrorMessage;
	};

	inline FValidatedRequestPlan ValidatedFailedPlan(const char* ErrorCode, const std::string& ErrorMessage)
	{
		FValidatedRequestPlan Plan;
		Plan.ErrorCode = ErrorCode;
		Plan.ErrorMessage = ErrorMessage;
		return Plan;
	}

	/**
	 * POST /api/v1/app/validated-actions/runs: `{"userId", "leaderboardKey"}`.
	 * An empty (or blank) LeaderboardKey is omitted: the ticket is not bound to a board.
	 */
	inline FValidatedRequestPlan BuildValidatedStartRunPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& LeaderboardKey)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeSessionRequired, "A signed-in player is required.");
		}

		const std::string NormalizedKey = Trim(LeaderboardKey);

		FValidatedRequestPlan Plan;
		Plan.bShouldSend = true;
		Plan.Verb = "POST";
		Plan.Endpoint = ValidatedStartRunEndpoint;
		Plan.BodyJson = "{\"userId\":\"" + EscapeJson(UserId) + "\"";
		if (!NormalizedKey.empty())
		{
			Plan.BodyJson += ",\"leaderboardKey\":\"" + EscapeJson(NormalizedKey) + "\"";
		}
		Plan.BodyJson += "}";
		return Plan;
	}

	/**
	 * POST /api/v1/app/validated-actions/submit:
	 * `{"userId", "ticket", "inputLogHash", "score", "stage", "leaderboardKey", "earned"}`.
	 * `score` is always sent (the server ignores it for a run without board); `stage`,
	 * `leaderboardKey` and `earned` are omitted when empty.
	 *
	 * Local checks in this order: SESSION_REQUIRED (no user or session token),
	 * NO_ACTIVE_RUN (no ticket), INVALID_INPUT_LOG_HASH (not 64 hex characters).
	 */
	inline FValidatedRequestPlan BuildValidatedSubmitPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& Ticket,
		const std::string& InputLogHash,
		std::int64_t Score,
		const std::string& Stage,
		const std::string& LeaderboardKey,
		const std::vector<FValidatedEarnedValue>& Earned)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeSessionRequired, "A signed-in player is required.");
		}
		if (Ticket.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeNoActiveRun, "No active run: call StartRun first.");
		}
		if (!IsValidInputLogHash(InputLogHash))
		{
			return ValidatedFailedPlan(ValidatedCodeInvalidInputLogHash,
				"The input log hash must be 64 hex characters (SHA-256).");
		}

		const std::string NormalizedStage = Trim(Stage);
		const std::string NormalizedKey = Trim(LeaderboardKey);

		FValidatedRequestPlan Plan;
		Plan.bShouldSend = true;
		Plan.Verb = "POST";
		Plan.Endpoint = ValidatedSubmitEndpoint;
		Plan.BodyJson = "{\"userId\":\"" + EscapeJson(UserId) + "\""
			+ ",\"ticket\":\"" + EscapeJson(Ticket) + "\""
			+ ",\"inputLogHash\":\"" + NormalizeInputLogHash(InputLogHash) + "\""
			+ ",\"score\":" + std::to_string(Score);
		if (!NormalizedStage.empty())
		{
			Plan.BodyJson += ",\"stage\":\"" + EscapeJson(NormalizedStage) + "\"";
		}
		if (!NormalizedKey.empty())
		{
			Plan.BodyJson += ",\"leaderboardKey\":\"" + EscapeJson(NormalizedKey) + "\"";
		}
		if (!Earned.empty())
		{
			Plan.BodyJson += ",\"earned\":[";
			for (std::size_t Index = 0; Index < Earned.size(); ++Index)
			{
				if (Index > 0)
				{
					Plan.BodyJson += ",";
				}
				Plan.BodyJson += "{\"key\":\"" + EscapeJson(Earned[Index].Key) + "\",\"amount\":"
					+ std::to_string(Earned[Index].Amount) + "}";
			}
			Plan.BodyJson += "]";
		}
		Plan.BodyJson += "}";
		return Plan;
	}

	/**
	 * GET /api/v1/app/validated-actions/state?userId=... with the player session (Part 2).
	 * No body. SESSION_REQUIRED without user or session token (no request is sent).
	 */
	inline FValidatedRequestPlan BuildValidatedGetStatePlan(
		const std::string& UserId,
		const std::string& SessionToken)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeSessionRequired, "A signed-in player is required.");
		}

		FValidatedRequestPlan Plan;
		Plan.bShouldSend = true;
		Plan.Verb = "GET";
		Plan.Endpoint = std::string(ValidatedStateEndpoint) + "?userId=" + EncodePathSegment(UserId);
		return Plan;
	}

	/** True for the Part 2 value rejections of a submit (UNKNOWN_VALUE_KEY, ..., INSUFFICIENT_BALANCE). */
	inline bool IsValueRejectionCode(const std::string& ServerErrorCode)
	{
		return ServerErrorCode == ValidatedCodeUnknownValueKey
			|| ServerErrorCode == ValidatedCodeDuplicateValueKey
			|| ServerErrorCode == ValidatedCodeEarnedAboveMax
			|| ServerErrorCode == ValidatedCodeEarnedBelowMin
			|| ServerErrorCode == ValidatedCodeInsufficientBalance;
	}

	/**
	 * True when a value of a submit result was applied in full (Part 2). A positive amount can be
	 * clamped by the daily cap or maxBalance; a spend is either applied in full or not at all
	 * (credited 0 when a concurrent run used the balance first). Grant a purchase only when this
	 * is true for the spent value.
	 */
	inline bool IsFullyCredited(std::int64_t Requested, std::int64_t Credited)
	{
		return Credited == Requested;
	}

	/**
	 * True when a finished submit used up the ticket, so the SDK clears the current run:
	 * success (2xx), the 422 ticket codes (TICKET_INVALID, TICKET_EXPIRED, TICKET_FOREIGN,
	 * TICKET_CONSUMED), every 422 rule or value rejection (the server consumes the ticket with
	 * status REJECTED) and 403 SCORE_LIMIT_REACHED (consumed, status FAILED).
	 *
	 * The server checks the board (404 LEADERBOARD_NOT_FOUND, 422 LEADERBOARD_MISMATCH),
	 * 400 SCORE_REQUIRED and 400 PLAYER_NAME_REQUIRED before it touches the ticket, so the run
	 * stays for those and the game may resubmit with a corrected board, score or name. Part 3:
	 * 403 PLAYER_BANNED is checked first of all rules and does not consume the ticket either, so
	 * the run stays (the game drops it with DiscardRun; the ticket stays usable after an unban).
	 * The run also stays on network errors (status 0), other 400, 401, other 403, 404, 429 and 5xx.
	 */
	inline bool ShouldClearRunAfterSubmit(int StatusCode, const std::string& ServerErrorCode)
	{
		if (StatusCode >= 200 && StatusCode < 300)
		{
			return true;
		}
		if (StatusCode == 422)
		{
			return ServerErrorCode != ValidatedCodeLeaderboardMismatch;
		}
		return StatusCode == 403 && ServerErrorCode == ValidatedCodeScoreLimitReached;
	}

	/**
	 * 429 codes whose wait can be up to an hour: the HTTP client must not retry them
	 * automatically (a 429 without code keeps the normal Retry-After handling).
	 */
	inline bool IsNonRetryableRateLimitCode(const std::string& ServerErrorCode)
	{
		return ServerErrorCode == ValidatedCodeRunRateLimited || ServerErrorCode == ValidatedCodeRunCapacityReached;
	}

	/**
	 * Error code shown for a failed Validated Actions request: the server `code` when
	 * present, NOT_SUPPORTED for a 404 without code or with the generic code NOT_FOUND
	 * (the backend lacks the feature, for example the simpleServer's unknown route answer),
	 * otherwise the SDK's HTTP mapping passed in as FallbackCode.
	 */
	inline std::string MapValidatedErrorCode(int StatusCode, const std::string& ServerErrorCode, const std::string& FallbackCode)
	{
		if (StatusCode == 404 && (ServerErrorCode.empty() || ServerErrorCode == "NOT_FOUND"))
		{
			return ValidatedCodeNotSupported;
		}
		if (!ServerErrorCode.empty())
		{
			return ServerErrorCode;
		}
		return FallbackCode;
	}

	// ============================================================
	// Part 3 (TASK-888): evidence upload
	// ============================================================

	/**
	 * Standard base64 with padding (RFC 4648 section 4, alphabet A-Z a-z 0-9 + /), the encoding the
	 * server decodes with java.util.Base64.getDecoder(). Same output as FBase64::Encode of the engine.
	 */
	inline std::string Base64Encode(const std::uint8_t* Data, std::size_t Length)
	{
		static const char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		std::string Result;
		Result.reserve(((Length + 2) / 3) * 4);
		std::size_t Index = 0;
		for (; Index + 2 < Length; Index += 3)
		{
			const std::uint32_t Triple = (static_cast<std::uint32_t>(Data[Index]) << 16)
				| (static_cast<std::uint32_t>(Data[Index + 1]) << 8)
				| static_cast<std::uint32_t>(Data[Index + 2]);
			Result.push_back(Alphabet[(Triple >> 18) & 0x3Fu]);
			Result.push_back(Alphabet[(Triple >> 12) & 0x3Fu]);
			Result.push_back(Alphabet[(Triple >> 6) & 0x3Fu]);
			Result.push_back(Alphabet[Triple & 0x3Fu]);
		}
		const std::size_t Remaining = Length - Index;
		if (Remaining == 1)
		{
			const std::uint32_t Triple = static_cast<std::uint32_t>(Data[Index]) << 16;
			Result.push_back(Alphabet[(Triple >> 18) & 0x3Fu]);
			Result.push_back(Alphabet[(Triple >> 12) & 0x3Fu]);
			Result.append("==");
		}
		else if (Remaining == 2)
		{
			const std::uint32_t Triple = (static_cast<std::uint32_t>(Data[Index]) << 16)
				| (static_cast<std::uint32_t>(Data[Index + 1]) << 8);
			Result.push_back(Alphabet[(Triple >> 18) & 0x3Fu]);
			Result.push_back(Alphabet[(Triple >> 12) & 0x3Fu]);
			Result.push_back(Alphabet[(Triple >> 6) & 0x3Fu]);
			Result.push_back('=');
		}
		return Result;
	}

	inline std::string Base64Encode(const std::vector<std::uint8_t>& Data)
	{
		return Base64Encode(Data.empty() ? nullptr : Data.data(), Data.size());
	}

	/**
	 * PUT /api/v1/app/validated-actions/runs/{runId}/evidence: `{"userId", "log"}` with the player
	 * session. `log` is the standard base64 of the raw input log bytes, the same bytes whose
	 * SHA-256 was submitted as `inputLogHash`. The run ID is URL encoded into the path.
	 *
	 * Local checks in this order: SESSION_REQUIRED (no user or session token), INVALID_RUN_ID
	 * (blank run ID), EMPTY_INPUT_LOG (no bytes; the server rejects a blank `log`),
	 * EVIDENCE_TOO_LARGE (MaxBytes > 0 and the log is larger; 0 means unknown, the server decides).
	 */
	inline FValidatedRequestPlan BuildValidatedEvidenceUploadPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& RunId,
		const std::uint8_t* InputLog,
		std::size_t InputLogLength,
		std::int64_t MaxBytes)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeSessionRequired, "A signed-in player is required.");
		}
		const std::string NormalizedRunId = Trim(RunId);
		if (NormalizedRunId.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeInvalidRunId, "The run ID of the evidence request is missing.");
		}
		if (InputLog == nullptr || InputLogLength == 0)
		{
			return ValidatedFailedPlan(ValidatedCodeEmptyInputLog, "The input log is empty.");
		}
		if (MaxBytes > 0 && static_cast<std::uint64_t>(InputLogLength) > static_cast<std::uint64_t>(MaxBytes))
		{
			return ValidatedFailedPlan(ValidatedCodeEvidenceTooLarge,
				"The input log has " + std::to_string(InputLogLength) + " bytes, the server accepts at most "
				+ std::to_string(MaxBytes) + ".");
		}

		FValidatedRequestPlan Plan;
		Plan.bShouldSend = true;
		Plan.Verb = "PUT";
		Plan.Endpoint = std::string(ValidatedEvidenceEndpointPrefix) + EncodePathSegment(NormalizedRunId)
			+ ValidatedEvidenceEndpointSuffix;
		Plan.BodyJson = "{\"userId\":\"" + EscapeJson(UserId) + "\""
			+ ",\"log\":\"" + Base64Encode(InputLog, InputLogLength) + "\"}";
		return Plan;
	}

	inline FValidatedRequestPlan BuildValidatedEvidenceUploadPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& RunId,
		const std::vector<std::uint8_t>& InputLog,
		std::int64_t MaxBytes)
	{
		return BuildValidatedEvidenceUploadPlan(UserId, SessionToken, RunId,
			InputLog.empty() ? nullptr : InputLog.data(), InputLog.size(), MaxBytes);
	}

	/**
	 * True when a failed evidence upload may be sent again: after 422 EVIDENCE_HASH_MISMATCH (with
	 * the correct bytes, the request stays open until `uploadBefore`) and after a network error
	 * (CONNECTION_FAILED). Every other code is final: 400 EVIDENCE_INVALID_ENCODING, 404
	 * EVIDENCE_NOT_REQUESTED, 409 EVIDENCE_ALREADY_UPLOADED, 410 EVIDENCE_EXPIRED, 413
	 * EVIDENCE_TOO_LARGE, the local codes, session, server error and rate limit codes. The SDK
	 * never retries an upload by itself beyond the HTTP client's network retries.
	 */
	inline bool IsEvidenceUploadRetryable(const std::string& ErrorCode)
	{
		return ErrorCode == ValidatedCodeEvidenceHashMismatch
			|| ErrorCode == HttpCodeConnectionFailed;
	}

	// ============================================================
	// TASK-911: run start context
	// ============================================================

	/**
	 * What a run starts from, declared by the game. Every field is optional; blank strings and an
	 * empty InitialState count as absent and are not sent. Versions are at most 64 printable ASCII
	 * characters (checked by the server), ContentDigest is the SHA-256 of the game content as 64
	 * hex characters (checked locally), InitialState holds the raw bytes the simulation starts from.
	 */
	struct FValidatedRunContext
	{
		std::string GameVersion;
		std::string ContentVersion;
		std::string SimulationVersion;
		std::string ReplayFormatVersion;
		std::string ContentDigest;
		std::vector<std::uint8_t> InitialState;
	};

	/** True when no field of the context is set; such a context is left out of the request. */
	inline bool IsEmptyRunContext(const FValidatedRunContext& Context)
	{
		return Trim(Context.GameVersion).empty()
			&& Trim(Context.ContentVersion).empty()
			&& Trim(Context.SimulationVersion).empty()
			&& Trim(Context.ReplayFormatVersion).empty()
			&& Trim(Context.ContentDigest).empty()
			&& Context.InitialState.empty();
	}

	/**
	 * The `context` object of a run start, or "" when nothing is set (the field is then left out
	 * and older servers see the request they know). camelCase fields in a fixed order, blank fields
	 * left out, versions sent unchanged, the digest trimmed and in lower case, InitialState as
	 * standard base64 with padding. The caller checks the digest first (IsValidInputLogHash).
	 */
	inline std::string BuildValidatedRunContextJson(const FValidatedRunContext& Context)
	{
		if (IsEmptyRunContext(Context))
		{
			return std::string();
		}

		std::string Json;
		const auto Append = [&Json](const char* Field, const std::string& Value)
		{
			Json += Json.empty() ? "{" : ",";
			Json += "\"";
			Json += Field;
			Json += "\":\"" + EscapeJson(Value) + "\"";
		};
		if (!Trim(Context.GameVersion).empty())
		{
			Append("gameVersion", Context.GameVersion);
		}
		if (!Trim(Context.ContentVersion).empty())
		{
			Append("contentVersion", Context.ContentVersion);
		}
		if (!Trim(Context.SimulationVersion).empty())
		{
			Append("simulationVersion", Context.SimulationVersion);
		}
		if (!Trim(Context.ReplayFormatVersion).empty())
		{
			Append("replayFormatVersion", Context.ReplayFormatVersion);
		}
		const std::string Digest = Trim(Context.ContentDigest);
		if (!Digest.empty())
		{
			Append("contentDigest", NormalizeInputLogHash(Digest));
		}
		if (!Context.InitialState.empty())
		{
			Append("initialState", Base64Encode(Context.InitialState));
		}
		Json += "}";
		return Json;
	}

	/**
	 * POST /api/v1/app/validated-actions/runs with an optional run start context:
	 * `{"userId", "leaderboardKey", "context"}`. An empty context is left out, so the body equals
	 * the one of BuildValidatedStartRunPlan without context.
	 *
	 * Local checks in this order: SESSION_REQUIRED (no user or session token),
	 * INVALID_CONTENT_DIGEST (a set digest that is not 64 hex characters). Everything else (version
	 * format, initial state size) is left to the server: 400 INITIAL_STATE_INVALID_ENCODING,
	 * 413 INITIAL_STATE_TOO_LARGE, 400 without code for a bad version.
	 */
	inline FValidatedRequestPlan BuildValidatedStartRunPlan(
		const std::string& UserId,
		const std::string& SessionToken,
		const std::string& LeaderboardKey,
		const FValidatedRunContext& Context)
	{
		if (UserId.empty() || SessionToken.empty())
		{
			return ValidatedFailedPlan(ValidatedCodeSessionRequired, "A signed-in player is required.");
		}
		const std::string Digest = Trim(Context.ContentDigest);
		if (!Digest.empty() && !IsValidInputLogHash(Digest))
		{
			return ValidatedFailedPlan(ValidatedCodeInvalidContentDigest,
				"The content digest must be 64 hex characters (SHA-256).");
		}

		FValidatedRequestPlan Plan = BuildValidatedStartRunPlan(UserId, SessionToken, LeaderboardKey);
		const std::string ContextJson = BuildValidatedRunContextJson(Context);
		if (!ContextJson.empty())
		{
			// Insert before the closing brace of {"userId", "leaderboardKey"}.
			Plan.BodyJson.insert(Plan.BodyJson.size() - 1, ",\"context\":" + ContextJson);
		}
		return Plan;
	}
}
