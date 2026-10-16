// Copyright (c) 2025-2026 horizOn. All rights reserved.
#include "Managers/HorizonGiftCodeManager.h"
#include "Managers/HorizonPlayerProfileManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "HorizonSDKModule.h"
#include "Transport/HorizonRetryContract.h"
#include "Dom/JsonObject.h"

void UHorizonGiftCodeManager::Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth)
{ HttpClient=Http; AuthManager=Auth; ClearCache(); }
void UHorizonGiftCodeManager::SetPlayerProfileManager(UHorizonPlayerProfileManager* Profile) { PlayerProfileManager=Profile; }
void UHorizonGiftCodeManager::SetPlayerValuesManager(UHorizonPlayerValuesManager* Values) { PlayerValues=Values; }
void UHorizonGiftCodeManager::SyncIdentity()
{
    const FString Next=AuthManager && HttpClient?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();
    const uint64 Generation=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if (Identity!=Next || TransportEpoch!=Generation)
    { Identity=Next; TransportEpoch=Generation; ++Epoch; LastResult={}; LastGrantedUnlocks.Empty(); }
}
void UHorizonGiftCodeManager::ClearCache() { SyncIdentity(); ++Epoch; LastResult={}; LastGrantedUnlocks.Empty(); }
FHorizonGiftCodeRedeemResult UHorizonGiftCodeManager::GetLastRedeemResult() { SyncIdentity(); return LastResult; }
const TArray<FString>& UHorizonGiftCodeManager::GetLastGrantedUnlocks() const
{ const_cast<UHorizonGiftCodeManager*>(this)->SyncIdentity(); return LastGrantedUnlocks; }
void UHorizonGiftCodeManager::Redeem(const FString& Code,FOnGiftCodeRedeemComplete Complete)
{
    RedeemInternal(Code,TEXT(""),false,FOnGiftCodeRedeemResultComplete::CreateLambda([Complete](bool Ok,const FHorizonGiftCodeRedeemResult& Result,const FString&,const FString& Message)
    { Complete.ExecuteIfBound(Ok,Result.GiftData,Message); }));
}
void UHorizonGiftCodeManager::RedeemWithIdempotency(const FString& Code,const FString& Id,FOnGiftCodeRedeemResultComplete Complete)
{ RedeemInternal(Code,Id,true,Complete); }
void UHorizonGiftCodeManager::RedeemCodeWithIdempotency(const FString& Code,const FString& Id)
{
    RedeemWithIdempotency(Code,Id,FOnGiftCodeRedeemResultComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGiftCodeRedeemResult& Result,const FString& Error,const FString& Message)
    { OnRedeemResult.Broadcast(Ok,Result,Error,Message); }));
}
void UHorizonGiftCodeManager::RedeemInternal(const FString& Code,const FString& Id,bool Explicit,FOnGiftCodeRedeemResultComplete Complete)
{
    SyncIdentity(); FGuid UserGuid;
    const FString User=AuthManager?AuthManager->GetCurrentUser().UserId:FString();
    if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)
    { Complete.ExecuteIfBound(false,{},TEXT("SESSION_REQUIRED"),TEXT("User is not signed in.")); return; }
    if (!FGuid::ParseExact(User,EGuidFormats::DigitsWithHyphens,UserGuid) || Code.TrimStartAndEnd().IsEmpty() || Code.Len()>50 ||
        (Explicit && (Id.Len()>128 || !HorizonTransportContract::HasNonBlankIdempotencyId(TCHAR_TO_UTF8(*Id)))))
    { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Gift code and original nonblank ID are required.")); return; }
    if (bRedeeming) { Complete.ExecuteIfBound(false,{},TEXT("REQUEST_IN_PROGRESS"),TEXT("A redeem is already pending.")); return; }
    bRedeeming=true;
    const FString SentIdentity=Identity; const uint64 SentTransport=TransportEpoch; const int64 SentEpoch=Epoch;
    const int64 ValuesEpoch=PlayerValues?PlayerValues->GetCacheEpoch():-1;
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("code"),Code); Body->SetStringField(TEXT("userId"),User);
    if (Explicit) Body->SetStringField(TEXT("idempotencyId"),Id);
    TWeakObjectPtr<UHorizonGiftCodeManager> Weak(this);
    HttpClient->PostJson(Body,TEXT("/api/v1/app/gift-codes/redeem"),true,FOnHttpResponse::CreateLambda(
        [Weak,User,SentIdentity,SentTransport,SentEpoch,ValuesEpoch,Complete](const FHorizonNetworkResponse& Response)
        {
            auto* Self=Weak.Get(); if (!Self) return; Self->bRedeeming=false; Self->SyncIdentity();
            if (Self->Identity!=SentIdentity || Self->TransportEpoch!=SentTransport || Self->Epoch!=SentEpoch ||
                (Self->PlayerValues && Self->PlayerValues->GetCacheEpoch()!=ValuesEpoch))
            { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Player session or cache changed; retain the original ID.")); return; }
            if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
            FHorizonGiftCodeRedeemResult Result;
            if (!FHorizonGiftCodeRedeemResult::TryFromJson(Response.JsonData,Result) || (Result.PlayerValues.bPresent && Result.PlayerValues.UserId!=User))
            { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid redeem receipt; retain the original ID.")); return; }
            if (!Result.bSuccess) { Complete.ExecuteIfBound(false,Result,TEXT("REDEEM_REJECTED"),Result.Message); return; }
            if (Result.PlayerValues.bPresent && (!Self->PlayerValues || !Self->PlayerValues->AcceptServerState(User,Result.PlayerValues)))
            { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Cannot adopt the complete player-value snapshot.")); return; }
            Self->LastResult=Result; Self->LastGrantedUnlocks=Result.GrantedUnlocks;
            if (!Result.GrantedUnlocks.IsEmpty() && Self->PlayerProfileManager) Self->PlayerProfileManager->ClearCache();
            Complete.ExecuteIfBound(true,Result,TEXT(""),Result.Message);
        }));
}

// ============================================================
// Validate
// ============================================================

void UHorizonGiftCodeManager::Validate(const FString& Code, FOnGiftCodeValidateComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Validate -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, false);
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("code"), Code);
	Body->SetStringField(TEXT("userId"), UserId);

	TWeakObjectPtr<UHorizonGiftCodeManager> WeakSelf(this);
	FOnGiftCodeValidateComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/gift-codes/validate"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("GiftCode::Validate -- Request failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, false);
					return;
				}

				bool bValid = false;
				if (Response.JsonData.IsValid())
				{
					bValid = Response.JsonData->GetBoolField(TEXT("valid"));
				}

				UE_LOG(LogHorizonSDK, Log, TEXT("GiftCode::Validate -- Code is %s."), bValid ? TEXT("valid") : TEXT("invalid"));
				CapturedOnComplete.ExecuteIfBound(true, bValid);
			}
		));
}
