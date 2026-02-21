// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Auth.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"

// ============================================================
// Factory methods
// ============================================================

UHorizonAsync_Auth* UHorizonAsync_Auth::SignUpAnonymous(const UObject* WorldContextObject, const FString& DisplayName)
{
	UHorizonAsync_Auth* Action = NewObject<UHorizonAsync_Auth>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EAuthOp::SignUpAnonymous;
	Action->ParamDisplayName = DisplayName;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_Auth* UHorizonAsync_Auth::SignUpEmail(const UObject* WorldContextObject, const FString& Email, const FString& Password, const FString& Username)
{
	UHorizonAsync_Auth* Action = NewObject<UHorizonAsync_Auth>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EAuthOp::SignUpEmail;
	Action->ParamEmail = Email;
	Action->ParamPassword = Password;
	Action->ParamUsername = Username;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_Auth* UHorizonAsync_Auth::SignInEmail(const UObject* WorldContextObject, const FString& Email, const FString& Password)
{
	UHorizonAsync_Auth* Action = NewObject<UHorizonAsync_Auth>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EAuthOp::SignInEmail;
	Action->ParamEmail = Email;
	Action->ParamPassword = Password;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_Auth* UHorizonAsync_Auth::SignInAnonymous(const UObject* WorldContextObject, const FString& AnonymousToken)
{
	UHorizonAsync_Auth* Action = NewObject<UHorizonAsync_Auth>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EAuthOp::SignInAnonymous;
	Action->ParamAnonymousToken = AnonymousToken;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UHorizonAsync_Auth* UHorizonAsync_Auth::RestoreSession(const UObject* WorldContextObject)
{
	UHorizonAsync_Auth* Action = NewObject<UHorizonAsync_Auth>();
	Action->WorldContext = WorldContextObject;
	Action->Operation = EAuthOp::RestoreSession;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

// ============================================================
// Activate
// ============================================================

void UHorizonAsync_Auth::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Auth)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or Auth manager not found."));
		SetReadyToDestroy();
		return;
	}

	FOnAuthComplete Callback = FOnAuthComplete::CreateUObject(this, &UHorizonAsync_Auth::HandleAuthResult);

	switch (Operation)
	{
	case EAuthOp::SignUpAnonymous:
		Subsystem->Auth->SignUpAnonymous(ParamDisplayName, Callback);
		break;
	case EAuthOp::SignUpEmail:
		Subsystem->Auth->SignUpEmail(ParamEmail, ParamPassword, ParamUsername, Callback);
		break;
	case EAuthOp::SignInEmail:
		Subsystem->Auth->SignInEmail(ParamEmail, ParamPassword, Callback);
		break;
	case EAuthOp::SignInAnonymous:
		Subsystem->Auth->SignInAnonymous(ParamAnonymousToken, Callback);
		break;
	case EAuthOp::RestoreSession:
		Subsystem->Auth->RestoreSession(Callback);
		break;
	}
}

void UHorizonAsync_Auth::HandleAuthResult(bool bSuccess)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast(TEXT("Authentication failed."));
	}
	SetReadyToDestroy();
}
