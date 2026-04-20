// Copyright (c) 2025-2026 horizOn. All rights reserved.
//
// Objective-C++ implementation of the Apple Sign-In bridge for iOS.
// Compiled only when PLATFORM_IOS is defined; HorizonSDK.Build.cs gates the .mm file
// to the IOS target and links the AuthenticationServices framework.

#if PLATFORM_IOS

#include "iOS/HorizonAppleSignInBridge.h"
#include "HorizonSDKModule.h"
#include "Async/Async.h"
#include "IOS/IOSAppDelegate.h"

#import <AuthenticationServices/AuthenticationServices.h>
#import <CommonCrypto/CommonCrypto.h>
#import <UIKit/UIKit.h>

// ---------------------------------------------------------------------------
// Nonce helpers — Apple validates a SHA-256 hash of the client-supplied nonce
// against the `nonce` claim in the returned identity token.  The backend
// AppleIdTokenVerifier (Step 2) re-checks this server-side; mirror the format.
// ---------------------------------------------------------------------------

static NSString* HorizonApple_RandomNonce(NSUInteger Length)
{
	NSString* CharSet = @"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._";
	NSMutableString* Result = [NSMutableString stringWithCapacity:Length];

	while (Result.length < Length)
	{
		uint8_t RandomByte = 0;
		if (SecRandomCopyBytes(kSecRandomDefault, 1, &RandomByte) != errSecSuccess)
		{
			RandomByte = (uint8_t)(arc4random_uniform(256));
		}

		const NSUInteger Index = RandomByte % CharSet.length;
		[Result appendString:[CharSet substringWithRange:NSMakeRange(Index, 1)]];
	}

	return Result;
}

static NSString* HorizonApple_Sha256(NSString* Input)
{
	NSData* InputData = [Input dataUsingEncoding:NSUTF8StringEncoding];
	uint8_t Digest[CC_SHA256_DIGEST_LENGTH];
	CC_SHA256(InputData.bytes, (CC_LONG)InputData.length, Digest);

	NSMutableString* Hex = [NSMutableString stringWithCapacity:CC_SHA256_DIGEST_LENGTH * 2];
	for (NSUInteger i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i)
	{
		[Hex appendFormat:@"%02x", Digest[i]];
	}
	return Hex;
}

// ---------------------------------------------------------------------------
// Delegate object — implements ASAuthorizationControllerDelegate and the
// presentation-context-providing protocol so the system sheet has a window
// anchor.  The instance is retained as a static strong reference for the
// duration of the request and released after dispatch.
// ---------------------------------------------------------------------------

API_AVAILABLE(ios(13.0))
@interface HorizonAppleSignInDelegate : NSObject <ASAuthorizationControllerDelegate, ASAuthorizationControllerPresentationContextProviding>
@property (nonatomic, copy) void(^Completion)(BOOL bSuccess, NSString* IdentityToken, NSString* FirstName, NSString* LastName, NSString* ErrorCode);
@end

API_AVAILABLE(ios(13.0))
@implementation HorizonAppleSignInDelegate

- (void)authorizationController:(ASAuthorizationController*)Controller
	didCompleteWithAuthorization:(ASAuthorization*)Authorization
{
	if (![Authorization.credential isKindOfClass:[ASAuthorizationAppleIDCredential class]])
	{
		if (self.Completion) { self.Completion(NO, @"", @"", @"", @"INVALID_RESPONSE"); }
		return;
	}

	ASAuthorizationAppleIDCredential* Credential = (ASAuthorizationAppleIDCredential*)Authorization.credential;
	NSData* TokenData = Credential.identityToken;

	if (!TokenData)
	{
		if (self.Completion) { self.Completion(NO, @"", @"", @"", @"INVALID_RESPONSE"); }
		return;
	}

	NSString* TokenStr = [[NSString alloc] initWithData:TokenData encoding:NSUTF8StringEncoding] ?: @"";
	NSString* GivenName = Credential.fullName.givenName ?: @"";
	NSString* FamilyName = Credential.fullName.familyName ?: @"";

	if (self.Completion) { self.Completion(YES, TokenStr, GivenName, FamilyName, @""); }
}

- (void)authorizationController:(ASAuthorizationController*)Controller
		   didCompleteWithError:(NSError*)Error
{
	NSString* ErrorCode = @"FAILED";
	switch (Error.code)
	{
		case ASAuthorizationErrorCanceled:        ErrorCode = @"USER_CANCELLED"; break;
		case ASAuthorizationErrorInvalidResponse: ErrorCode = @"INVALID_RESPONSE"; break;
		case ASAuthorizationErrorNotHandled:      ErrorCode = @"NOT_HANDLED"; break;
		case ASAuthorizationErrorFailed:          ErrorCode = @"FAILED"; break;
		case ASAuthorizationErrorUnknown:         ErrorCode = @"FAILED"; break;
		default:                                  ErrorCode = @"FAILED"; break;
	}

	if (self.Completion) { self.Completion(NO, @"", @"", @"", ErrorCode); }
}

- (ASPresentationAnchor)presentationAnchorForAuthorizationController:(ASAuthorizationController*)Controller API_AVAILABLE(ios(13.0))
{
	UIWindow* KeyWindow = [IOSAppDelegate GetDelegate].Window;
	return KeyWindow ?: [[UIApplication sharedApplication].windows firstObject];
}

@end

// Static strong reference keeps the delegate alive between Start and the
// async system callback.  Single-flight: a second StartSignIn while the
// first is still pending overwrites the reference; the previous request
// will silently never fire its completion.  Game code that needs strict
// queueing should serialize calls on its side.
static HorizonAppleSignInDelegate* GHorizonAppleDelegate API_AVAILABLE(ios(13.0)) = nil;

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void HorizonAppleSignInBridge::StartSignIn(FAppleSignInCallback Callback)
{
	if (@available(iOS 13.0, *))
	{
		// Generate a fresh nonce per request and pass its SHA-256 to Apple.
		NSString* RawNonce = HorizonApple_RandomNonce(32);
		NSString* HashedNonce = HorizonApple_Sha256(RawNonce);

		ASAuthorizationAppleIDProvider* Provider = [[ASAuthorizationAppleIDProvider alloc] init];
		ASAuthorizationAppleIDRequest* Request = [Provider createRequest];
		Request.requestedScopes = @[ASAuthorizationScopeFullName, ASAuthorizationScopeEmail];
		Request.nonce = HashedNonce;

		HorizonAppleSignInDelegate* Delegate = [[HorizonAppleSignInDelegate alloc] init];
		Delegate.Completion = ^(BOOL bSuccess, NSString* IdentityToken, NSString* FirstName, NSString* LastName, NSString* ErrorCode)
		{
			// Hop back to the game thread so the manager callback can touch UObject state.
			const FString TokenFs = FString(IdentityToken ?: @"");
			const FString FirstFs = FString(FirstName ?: @"");
			const FString LastFs  = FString(LastName ?: @"");
			const FString CodeFs  = FString(ErrorCode ?: @"");
			const bool bOk = (bSuccess == YES);

			AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), bOk, TokenFs, FirstFs, LastFs, CodeFs]()
			{
				if (Callback)
				{
					Callback(bOk, TokenFs, FirstFs, LastFs, CodeFs);
				}
			});

			// Release the static strong reference now that we've dispatched.
			GHorizonAppleDelegate = nil;
		};

		GHorizonAppleDelegate = Delegate;

		ASAuthorizationController* Controller = [[ASAuthorizationController alloc] initWithAuthorizationRequests:@[Request]];
		Controller.delegate = Delegate;
		Controller.presentationContextProvider = Delegate;
		[Controller performRequests];
	}
	else
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Apple Sign-In requires iOS 13+; refusing on this device."));
		AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback)]()
		{
			if (Callback)
			{
				Callback(false, FString(), FString(), FString(), TEXT("NOT_HANDLED"));
			}
		});
	}
}

#endif // PLATFORM_IOS
