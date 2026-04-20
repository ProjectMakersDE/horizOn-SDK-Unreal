// Copyright (c) 2025-2026 horizOn. All rights reserved.
//
// Objective-C++ bridge between UHorizonAuthManager (cross-platform C++) and Apple's
// AuthenticationServices framework on iOS. This header is iOS-only and never included
// on non-iOS platforms — UHorizonAuthManager guards the include with #if PLATFORM_IOS.

#pragma once

#if PLATFORM_IOS

#include "CoreMinimal.h"
#include "Templates/Function.h"

/**
 * Plain C++ facade around the Objective-C++ AuthenticationServices code.
 *
 * The implementation lives in HorizonAppleSignInBridge.mm. It owns a single static
 * delegate object that implements ASAuthorizationControllerDelegate +
 * ASAuthorizationControllerPresentationContextProviding, presents the Apple sign-in
 * sheet, and dispatches the result back on the game thread via the supplied callback.
 *
 * Callback signature:
 *   bSuccess     -- true iff the user completed the sheet and Apple returned a token
 *   IdentityToken -- the JWT to forward to horizOn (empty on failure)
 *   FirstName / LastName -- only populated on the very first login for this Apple ID
 *   ErrorCode    -- one of "USER_CANCELLED", "INVALID_RESPONSE", "NETWORK_ERROR",
 *                   "NOT_HANDLED", "FAILED" (mirrors ASAuthorizationError codes)
 */
struct HorizonAppleSignInBridge
{
	using FAppleSignInCallback = TFunction<void(
		bool bSuccess,
		const FString& IdentityToken,
		const FString& FirstName,
		const FString& LastName,
		const FString& ErrorCode)>;

	/** Begin a sign-in flow. Callback runs on the game thread exactly once. */
	static void StartSignIn(FAppleSignInCallback Callback);
};

#endif // PLATFORM_IOS
