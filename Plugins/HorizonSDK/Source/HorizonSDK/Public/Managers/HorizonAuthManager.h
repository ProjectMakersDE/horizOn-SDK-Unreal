// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Models/HorizonUserData.h"
#include "Http/HorizonHttpClient.h"
#include "HorizonAuthManager.generated.h"

/**
 * Manages all authentication flows for the horizOn SDK.
 *
 * Supports anonymous, email, and Google sign-up / sign-in.
 * Handles session caching (via UHorizonSessionSave) and session
 * restoration so that returning players are logged in automatically.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonAuthManager : public UObject
{
	GENERATED_BODY()

public:
	/** Called by UHorizonSubsystem::Initialize(). */
	void Initialize(UHorizonHttpClient* InHttpClient);

	// --- Sign Up ---

	/**
	 * Register a new anonymous user.
	 * @param DisplayName  Visible username.
	 * @param OnComplete   Called with true on success.
	 * @param AnonymousToken  Optional pre-existing token. If empty a new GUID is generated.
	 */
	void SignUpAnonymous(const FString& DisplayName, FOnAuthComplete OnComplete, const FString& AnonymousToken = TEXT(""));

	/** Register with email and password. */
	void SignUpEmail(const FString& Email, const FString& Password, const FString& Username, FOnAuthComplete OnComplete);

	/** Register with a Google authorization code. */
	void SignUpGoogle(const FString& GoogleAuthCode, const FString& RedirectUri, const FString& Username, FOnAuthComplete OnComplete);

	// --- Sign In ---

	/** Sign in with email and password. */
	void SignInEmail(const FString& Email, const FString& Password, FOnAuthComplete OnComplete);

	/** Sign in with an existing anonymous token. */
	void SignInAnonymous(const FString& AnonymousToken, FOnAuthComplete OnComplete);

	/** Sign in with a Google authorization code. */
	void SignInGoogle(const FString& GoogleAuthCode, const FString& RedirectUri, FOnAuthComplete OnComplete);

	/** Restore a previous session from disk and verify with the server. */
	void RestoreSession(FOnAuthComplete OnComplete);

	/** Verify the current session token is still valid server-side. */
	void CheckAuth(FOnAuthComplete OnComplete);

	// --- State ---

	/** Returns true if a user is currently signed in (has a valid user ID and token). */
	UFUNCTION(BlueprintPure, Category = "horizOn|Auth")
	bool IsSignedIn() const;

	/** Returns the current user data (empty if not signed in). */
	UFUNCTION(BlueprintPure, Category = "horizOn|Auth")
	FHorizonUserData GetCurrentUser() const;

	/** Sign out the current user and clear the in-memory session. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Auth")
	void SignOut();

	// --- Account management ---

	/** Change the display name of the signed-in user. */
	void ChangeName(const FString& NewName, FOnRequestComplete OnComplete);

	/** Request a password-reset email. */
	void ForgotPassword(const FString& Email, FOnRequestComplete OnComplete);

	/** Verify an email address with a server-issued token. */
	void VerifyEmail(const FString& Token, FOnRequestComplete OnComplete);

	/** Reset password using a server-issued token. */
	void ResetPassword(const FString& Token, const FString& NewPassword, FOnRequestComplete OnComplete);

	// --- Token helpers ---

	/** Returns true if a cached anonymous token exists on disk. */
	bool HasCachedAnonymousToken() const;

	/** Returns the cached anonymous token from disk (empty if none). */
	FString GetCachedAnonymousToken() const;

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "horizOn|Auth|Events")
	FOnHorizonUserSignedIn OnUserSignedIn;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|Auth|Events")
	FOnHorizonUserSignedOut OnUserSignedOut;

private:
	UPROPERTY()
	UHorizonHttpClient* HttpClient;

	FHorizonUserData CurrentUser;

	/** Common handler for all auth responses (signup / signin). */
	void HandleAuthResponse(const FHorizonNetworkResponse& Response, FOnAuthComplete OnComplete);

	/** Persist the current session to disk via UHorizonSessionSave. */
	void CacheSession();

	/** Clear in-memory session state (does NOT delete the save file). */
	void ClearSession();
};
