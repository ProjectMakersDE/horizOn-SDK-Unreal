// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonAuthManager.h"
#include "HorizonSDKModule.h"
#include "HorizonSessionSave.h"
#include "Dom/JsonObject.h"
#include "Misc/Guid.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonAuthManager::Initialize(UHorizonHttpClient* InHttpClient)
{
	HttpClient = InHttpClient;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonAuthManager initialized."));
}

// ============================================================
// Sign Up
// ============================================================

void UHorizonAuthManager::SignUpAnonymous(const FString& DisplayName, FOnAuthComplete OnComplete, const FString& AnonymousToken)
{
	FString Token = AnonymousToken;
	if (Token.IsEmpty())
	{
		Token = FGuid::NewGuid().ToString(EGuidFormats::DigitsLower);
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), TEXT("ANONYMOUS"));
	if (!DisplayName.IsEmpty())
	{
		Body->SetStringField(TEXT("username"), DisplayName);
	}
	Body->SetStringField(TEXT("anonymousToken"), Token);

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/signup"), false,
		FOnHttpResponse::CreateUObject(this, &UHorizonAuthManager::HandleAuthResponse, OnComplete));
}

void UHorizonAuthManager::SignUpEmail(const FString& Email, const FString& Password, const FString& Username, FOnAuthComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), TEXT("EMAIL"));
	if (!Email.IsEmpty())
	{
		Body->SetStringField(TEXT("email"), Email);
	}
	if (!Password.IsEmpty())
	{
		Body->SetStringField(TEXT("password"), Password);
	}
	if (!Username.IsEmpty())
	{
		Body->SetStringField(TEXT("username"), Username);
	}

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/signup"), false,
		FOnHttpResponse::CreateUObject(this, &UHorizonAuthManager::HandleAuthResponse, OnComplete));
}

void UHorizonAuthManager::SignUpGoogle(const FString& GoogleAuthCode, const FString& RedirectUri, const FString& Username, FOnAuthComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), TEXT("GOOGLE"));
	if (!GoogleAuthCode.IsEmpty())
	{
		Body->SetStringField(TEXT("googleAuthorizationCode"), GoogleAuthCode);
	}
	if (!RedirectUri.IsEmpty())
	{
		Body->SetStringField(TEXT("googleRedirectUri"), RedirectUri);
	}
	if (!Username.IsEmpty())
	{
		Body->SetStringField(TEXT("username"), Username);
	}

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/signup"), false,
		FOnHttpResponse::CreateUObject(this, &UHorizonAuthManager::HandleAuthResponse, OnComplete));
}

// ============================================================
// Sign In
// ============================================================

void UHorizonAuthManager::SignInEmail(const FString& Email, const FString& Password, FOnAuthComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), TEXT("EMAIL"));
	if (!Email.IsEmpty())
	{
		Body->SetStringField(TEXT("email"), Email);
	}
	if (!Password.IsEmpty())
	{
		Body->SetStringField(TEXT("password"), Password);
	}

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/signin"), false,
		FOnHttpResponse::CreateUObject(this, &UHorizonAuthManager::HandleAuthResponse, OnComplete));
}

void UHorizonAuthManager::SignInAnonymous(const FString& AnonymousToken, FOnAuthComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), TEXT("ANONYMOUS"));
	if (!AnonymousToken.IsEmpty())
	{
		Body->SetStringField(TEXT("anonymousToken"), AnonymousToken);
	}

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/signin"), false,
		FOnHttpResponse::CreateUObject(this, &UHorizonAuthManager::HandleAuthResponse, OnComplete));
}

void UHorizonAuthManager::SignInGoogle(const FString& GoogleAuthCode, const FString& RedirectUri, FOnAuthComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), TEXT("GOOGLE"));
	if (!GoogleAuthCode.IsEmpty())
	{
		Body->SetStringField(TEXT("googleAuthorizationCode"), GoogleAuthCode);
	}
	if (!RedirectUri.IsEmpty())
	{
		Body->SetStringField(TEXT("googleRedirectUri"), RedirectUri);
	}

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/signin"), false,
		FOnHttpResponse::CreateUObject(this, &UHorizonAuthManager::HandleAuthResponse, OnComplete));
}

// ============================================================
// Session management
// ============================================================

void UHorizonAuthManager::RestoreSession(FOnAuthComplete OnComplete)
{
	UHorizonSessionSave* Save = UHorizonSessionSave::LoadFromDisk();
	if (!Save || Save->CachedUserId.IsEmpty() || Save->CachedAccessToken.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Log, TEXT("RestoreSession -- No saved session found."));
		OnComplete.ExecuteIfBound(false);
		return;
	}

	// Populate CurrentUser from cached data
	CurrentUser.UserId      = Save->CachedUserId;
	CurrentUser.AccessToken = Save->CachedAccessToken;
	CurrentUser.DisplayName = Save->CachedDisplayName;
	CurrentUser.bIsAnonymous = Save->bIsAnonymous;
	CurrentUser.AnonymousToken = Save->CachedAnonymousToken;

	HttpClient->SetSessionToken(CurrentUser.AccessToken);

	TWeakObjectPtr<UHorizonAuthManager> WeakSelf(this);
	FOnAuthComplete CapturedOnComplete = OnComplete;

	CheckAuth(FOnAuthComplete::CreateLambda(
		[WeakSelf, CapturedOnComplete](bool bSuccess)
		{
			UHorizonAuthManager* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			if (bSuccess)
			{
				UE_LOG(LogHorizonSDK, Log, TEXT("Session restored for user %s."), *Self->CurrentUser.UserId);
				Self->OnUserSignedIn.Broadcast();
				CapturedOnComplete.ExecuteIfBound(true);
			}
			else
			{
				UE_LOG(LogHorizonSDK, Warning, TEXT("RestoreSession -- CheckAuth failed. Session cleared."));
				Self->ClearSession();
				CapturedOnComplete.ExecuteIfBound(false);
			}
		}
	));
}

void UHorizonAuthManager::CheckAuth(FOnAuthComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), CurrentUser.UserId);
	Body->SetStringField(TEXT("sessionToken"), CurrentUser.AccessToken);

	TWeakObjectPtr<UHorizonAuthManager> WeakSelf(this);
	FOnAuthComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/check-auth"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				UHorizonAuthManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CheckAuth request failed: %s"), *Response.ErrorMessage);
					Self->ClearSession();
					CapturedOnComplete.ExecuteIfBound(false);
					return;
				}

				bool bIsAuthenticated = false;
				if (Response.JsonData.IsValid() && Response.JsonData->HasField(TEXT("isAuthenticated")))
				{
					bIsAuthenticated = Response.JsonData->GetBoolField(TEXT("isAuthenticated"));
				}

				if (bIsAuthenticated)
				{
					CapturedOnComplete.ExecuteIfBound(true);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CheckAuth -- Server reports session is not authenticated."));
					Self->ClearSession();
					CapturedOnComplete.ExecuteIfBound(false);
				}
			}
		));
}

// ============================================================
// State
// ============================================================

bool UHorizonAuthManager::IsSignedIn() const
{
	return CurrentUser.IsValid();
}

FHorizonUserData UHorizonAuthManager::GetCurrentUser() const
{
	return CurrentUser;
}

void UHorizonAuthManager::SignOut()
{
	ClearSession();
	OnUserSignedOut.Broadcast();
	UE_LOG(LogHorizonSDK, Log, TEXT("User signed out."));
}

// ============================================================
// Account management
// ============================================================

void UHorizonAuthManager::ChangeName(const FString& NewName, FOnRequestComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), CurrentUser.UserId);
	Body->SetStringField(TEXT("sessionToken"), CurrentUser.AccessToken);
	if (!NewName.IsEmpty())
	{
		Body->SetStringField(TEXT("newName"), NewName);
	}

	TWeakObjectPtr<UHorizonAuthManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/change-name"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete, NewName](const FHorizonNetworkResponse& Response)
			{
				UHorizonAuthManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (Response.bSuccess)
				{
					Self->CurrentUser.DisplayName = NewName;
					Self->CacheSession();
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonAuthManager::ForgotPassword(const FString& Email, FOnRequestComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	if (!Email.IsEmpty())
	{
		Body->SetStringField(TEXT("email"), Email);
	}

	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/forgot-password"), false,
		FOnHttpResponse::CreateLambda(
			[CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (Response.bSuccess)
				{
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonAuthManager::VerifyEmail(const FString& Token, FOnRequestComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	if (!Token.IsEmpty())
	{
		Body->SetStringField(TEXT("token"), Token);
	}

	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/verify-email"), false,
		FOnHttpResponse::CreateLambda(
			[CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (Response.bSuccess)
				{
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonAuthManager::ResetPassword(const FString& Token, const FString& NewPassword, FOnRequestComplete OnComplete)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	if (!Token.IsEmpty())
	{
		Body->SetStringField(TEXT("token"), Token);
	}
	if (!NewPassword.IsEmpty())
	{
		Body->SetStringField(TEXT("newPassword"), NewPassword);
	}

	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/user-management/reset-password"), false,
		FOnHttpResponse::CreateLambda(
			[CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (Response.bSuccess)
				{
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

// ============================================================
// Token helpers
// ============================================================

bool UHorizonAuthManager::HasCachedAnonymousToken() const
{
	UHorizonSessionSave* Save = UHorizonSessionSave::LoadFromDisk();
	return Save && !Save->CachedAnonymousToken.IsEmpty();
}

FString UHorizonAuthManager::GetCachedAnonymousToken() const
{
	UHorizonSessionSave* Save = UHorizonSessionSave::LoadFromDisk();
	if (Save)
	{
		return Save->CachedAnonymousToken;
	}
	return FString();
}

// ============================================================
// Internal
// ============================================================

void UHorizonAuthManager::HandleAuthResponse(const FHorizonNetworkResponse& Response, FOnAuthComplete OnComplete)
{
	if (!Response.bSuccess)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Auth request failed: %s"), *Response.ErrorMessage);
		OnComplete.ExecuteIfBound(false);
		return;
	}

	CurrentUser.UpdateFromAuthResponse(Response.JsonData);
	HttpClient->SetSessionToken(CurrentUser.AccessToken);
	CacheSession();
	OnUserSignedIn.Broadcast();

	UE_LOG(LogHorizonSDK, Log, TEXT("User signed in: %s (%s)"), *CurrentUser.DisplayName, *CurrentUser.UserId);
	OnComplete.ExecuteIfBound(true);
}

void UHorizonAuthManager::CacheSession()
{
	UHorizonSessionSave* Save = NewObject<UHorizonSessionSave>();
	Save->CachedUserId         = CurrentUser.UserId;
	Save->CachedAccessToken    = CurrentUser.AccessToken;
	Save->CachedAnonymousToken = CurrentUser.AnonymousToken;
	Save->CachedDisplayName    = CurrentUser.DisplayName;
	Save->bIsAnonymous         = CurrentUser.bIsAnonymous;

	if (Save->SaveToDisk())
	{
		UE_LOG(LogHorizonSDK, Verbose, TEXT("Session cached to disk."));
	}
	else
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("Failed to cache session to disk."));
	}
}

void UHorizonAuthManager::ClearSession()
{
	CurrentUser.Clear();
	if (HttpClient)
	{
		HttpClient->ClearSessionToken();
	}
	// Note: do NOT delete save file here -- keep the anonymous token for future RestoreSession
}
