// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "HorizonUserLogManager.generated.h"

DECLARE_DELEGATE_ThreeParams(FOnUserLogComplete, bool /*bSuccess*/, const FString& /*LogId*/, const FString& /*CreatedAt*/);

/**
 * User Log Manager for the horizOn SDK.
 *
 * Sends user-level log entries to the horizOn server for server-side
 * analytics and debugging. Requires the user to be signed in.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonUserLogManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/**
	 * Create a log entry on the server.
	 * @param Level      Log level (Info, Warning, Error).
	 * @param Message    The log message (max 1000 chars, truncated with warning).
	 * @param OnComplete Called with (bSuccess, LogId, CreatedAt).
	 * @param ErrorCode  Optional error code string (max 50 chars).
	 */
	void CreateLog(EHorizonLogLevel Level, const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));

	/** Convenience: create an INFO-level log. */
	void Info(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));

	/** Convenience: create a WARN-level log. */
	void Warn(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));

	/** Convenience: create an ERROR-level log. */
	void Error(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	UPROPERTY()
	UHorizonAuthManager* AuthManager;

	/** Convert EHorizonLogLevel to the server-expected string. */
	static FString LogLevelToString(EHorizonLogLevel Level);

	/** Maximum allowed message length. */
	static constexpr int32 MaxMessageLength = 1000;

	/** Maximum allowed error code length. */
	static constexpr int32 MaxErrorCodeLength = 50;
};
