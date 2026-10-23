// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.
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
void UHorizonGiftCodeManager::Redeem(const FString&,FOnGiftCodeRedeemComplete Complete)
{ Complete.ExecuteIfBound(false,TEXT(""),TEXT("UPGRADE_REQUIRED: prepare and persist a V2 intent")); }
void UHorizonGiftCodeManager::RedeemWithIdempotency(const FString&,const FString&,FOnGiftCodeRedeemResultComplete Complete)
{ Complete.ExecuteIfBound(false,{},TEXT("UPGRADE_REQUIRED"),TEXT("V1 IDs cannot be translated; reconcile old operations")); }
void UHorizonGiftCodeManager::RedeemCodeWithIdempotency(const FString& Code,const FString& Id)
{ RedeemWithIdempotency(Code,Id,FOnGiftCodeRedeemResultComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGiftCodeRedeemResult& R,const FString& E,const FString& M) { OnRedeemResult.Broadcast(Ok,R,E,M); })); }
void UHorizonGiftCodeManager::PrepareRedemptionCode(const FString& Code)
{ PrepareRedemption(Code,FOnGiftCodePrepareComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGiftCodeIntent& I,const FString& E,const FString& M) { OnPrepared.Broadcast(Ok,I,E,M); })); }
void UHorizonGiftCodeManager::RedeemPreparedCode(const FHorizonGiftCodeIntent& I)
{ RedeemPrepared(I,FOnGiftCodeRedeemResultComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonGiftCodeRedeemResult& R,const FString& E,const FString& M) { OnRedeemResult.Broadcast(Ok,R,E,M); })); }
void UHorizonGiftCodeManager::PrepareRedemption(const FString& Code,FOnGiftCodePrepareComplete Complete)
{
    SyncIdentity();
    if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)
    { Complete.ExecuteIfBound(false,{},TEXT("SESSION_REQUIRED"),TEXT("Player session required")); return; }
    if (Code.TrimStartAndEnd().IsEmpty() || Code.Len()>50) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Code required")); return; }
    const FString User=AuthManager->GetCurrentUser().UserId,Scope=FHorizonGiftCodeIntent::Fingerprint(HttpClient->GetApiKey()),SentIdentity=Identity;
    const uint64 SentTransport=TransportEpoch; const int64 SentEpoch=Epoch;
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("code"),Code); Body->SetStringField(TEXT("userId"),User); Body->SetBoolField(TEXT("includeIntent"),true);
    TWeakObjectPtr<UHorizonGiftCodeManager> Weak(this);
    HttpClient->PostJson(Body,TEXT("/api/v1/app/gift-codes/validate"),true,FOnHttpResponse::CreateLambda([Weak,User,Scope,Code,SentIdentity,SentTransport,SentEpoch,Complete](const FHorizonNetworkResponse& R)
    {
        auto* Self=Weak.Get(); if (!Self) return; Self->SyncIdentity();
        if (Self->Identity!=SentIdentity || Self->TransportEpoch!=SentTransport || Self->Epoch!=SentEpoch) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Identity changed")); return; }
        if (!R.bSuccess) { Complete.ExecuteIfBound(false,{},R.GetErrorCodeString(),R.ErrorMessage); return; }
        bool Valid=false;
        if (R.JsonData && R.JsonData->TryGetBoolField(TEXT("valid"),Valid) && !Valid) { Complete.ExecuteIfBound(false,{},TEXT("CODE_NOT_ELIGIBLE"),TEXT("Code cannot be redeemed")); return; }
        FHorizonGiftCodeIntent I;
        if (!FHorizonGiftCodeIntent::TryPrepare(R.JsonData,Scope,User,Code,I)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid prepared intent")); return; }
        Complete.ExecuteIfBound(true,I,TEXT(""),TEXT("Persist every intent field before sending"));
    }));
}
void UHorizonGiftCodeManager::RedeemPrepared(const FHorizonGiftCodeIntent& Original,FOnGiftCodeRedeemResultComplete Complete)
{
    SyncIdentity();
    const FString User=AuthManager?AuthManager->GetCurrentUser().UserId:FString();
    if (!HttpClient || !AuthManager || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken)
    { Complete.ExecuteIfBound(false,{},TEXT("SESSION_REQUIRED"),TEXT("Player session required")); return; }
    const FHorizonGiftCodeIntent Intent=Original;
    if (!Intent.IsValidFor(FHorizonGiftCodeIntent::Fingerprint(HttpClient->GetApiKey()),User)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Original scoped V2 tuple required")); return; }
    if (bRedeeming) { Complete.ExecuteIfBound(false,{},TEXT("REQUEST_IN_PROGRESS"),TEXT("A redemption is pending")); return; }
    bRedeeming=true;
    const FString SentIdentity=Identity; const uint64 SentTransport=TransportEpoch; const int64 SentEpoch=Epoch;
    const int64 ValuesEpoch=PlayerValues?PlayerValues->GetCacheEpoch():-1;
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("code"),Intent.Code); Body->SetStringField(TEXT("userId"),User); Body->SetNumberField(TEXT("protocolVersion"),2);
    Body->SetStringField(TEXT("campaignId"),Intent.CampaignId); Body->SetStringField(TEXT("expectedCount"),Intent.ExpectedCount); Body->SetStringField(TEXT("configRevision"),Intent.ConfigRevision);
    TWeakObjectPtr<UHorizonGiftCodeManager> Weak(this);
    HttpClient->PostJson(Body,TEXT("/api/v1/app/gift-codes/redeem"),true,FOnHttpResponse::CreateLambda([Weak,Intent,User,SentIdentity,SentTransport,SentEpoch,ValuesEpoch,Complete](const FHorizonNetworkResponse& Response)
    {
        auto* Self=Weak.Get(); if (!Self) return; Self->bRedeeming=false; Self->SyncIdentity();
        if (Self->Identity!=SentIdentity || Self->TransportEpoch!=SentTransport || Self->Epoch!=SentEpoch) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Identity changed; keep original tuple")); return; }
        if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
        FHorizonGiftCodeRedeemResult R;
        if (!FHorizonGiftCodeRedeemResult::TryFromJson(Response.JsonData,R) || R.CampaignId!=Intent.CampaignId || FCString::Atoi64(*R.Count)<=FCString::Atoi64(*Intent.ExpectedCount) || (R.PlayerValues.bPresent && R.PlayerValues.UserId!=User))
        { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid sequence result")); return; }
        if (!R.bAlreadyApplied && (R.Count!=LexToString(FCString::Atoi64(*Intent.ExpectedCount)+1) || R.ConfigRevision!=Intent.ConfigRevision)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Fresh sequence changed")); return; }
        if (!R.bSuccess) { Complete.ExecuteIfBound(false,R,TEXT("REDEEM_REJECTED"),R.Message); return; }
        if (R.PlayerValues.bPresent && (!Self->PlayerValues || Self->PlayerValues->GetCacheEpoch()!=ValuesEpoch || !Self->PlayerValues->AcceptServerState(User,R.PlayerValues)))
        { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Value snapshot superseded")); return; }
        Self->LastResult=R; Self->LastGrantedUnlocks=R.GrantedUnlocks;
        if ((R.bAlreadyApplied || !R.GrantedUnlocks.IsEmpty()) && Self->PlayerProfileManager) Self->PlayerProfileManager->ClearCache();
        Complete.ExecuteIfBound(true,R,TEXT(""),R.Message);
    }));
}

// ============================================================
// Validate
// ============================================================

void UHorizonGiftCodeManager::Validate(const FString& Code, FOnGiftCodeValidateComplete OnComplete)
{
    if (!HttpClient || Code.TrimStartAndEnd().IsEmpty() || Code.Len()>50)
    { OnComplete.ExecuteIfBound(false,false); return; }

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("code"), Code);

	TWeakObjectPtr<UHorizonGiftCodeManager> WeakSelf(this);
	FOnGiftCodeValidateComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("/api/v1/app/gift-codes/validate"), false,
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
