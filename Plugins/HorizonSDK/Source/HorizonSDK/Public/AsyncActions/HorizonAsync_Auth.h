// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_Auth.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAuthAsyncSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint nodes for horizOn authentication.
 *
 * Provides factory methods for all supported auth flows:
 * anonymous sign-up/in, email sign-up/in, and session restoration.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_Auth : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnAuthAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnAuthAsyncFailure OnFailure;

	/** Register a new anonymous user. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Sign Up Anonymous"), Category = "horizOn|Auth")
	static UHorizonAsync_Auth* SignUpAnonymous(const UObject* WorldContextObject, const FString& DisplayName);

	/** Register with email and password. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Sign Up Email"), Category = "horizOn|Auth")
	static UHorizonAsync_Auth* SignUpEmail(const UObject* WorldContextObject, const FString& Email, const FString& Password, const FString& Username);

	/** Sign in with email and password. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Sign In Email"), Category = "horizOn|Auth")
	static UHorizonAsync_Auth* SignInEmail(const UObject* WorldContextObject, const FString& Email, const FString& Password);

	/** Sign in with an existing anonymous token. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Sign In Anonymous"), Category = "horizOn|Auth")
	static UHorizonAsync_Auth* SignInAnonymous(const UObject* WorldContextObject, const FString& AnonymousToken);

	/** Restore a previous session from disk. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Restore Session"), Category = "horizOn|Auth")
	static UHorizonAsync_Auth* RestoreSession(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	/** Which auth operation to perform. */
	enum class EAuthOp : uint8
	{
		SignUpAnonymous,
		SignUpEmail,
		SignInEmail,
		SignInAnonymous,
		RestoreSession
	};

	TWeakObjectPtr<const UObject> WorldContext;
	EAuthOp Operation;

	// Stored parameters
	FString ParamDisplayName;
	FString ParamEmail;
	FString ParamPassword;
	FString ParamUsername;
	FString ParamAnonymousToken;

	void HandleAuthResult(bool bSuccess);
};
