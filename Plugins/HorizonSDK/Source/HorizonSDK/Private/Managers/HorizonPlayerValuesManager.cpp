#include "Managers/HorizonPlayerValuesManager.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "String/LexFromString.h"

namespace { constexpr int64 MaxSafe = 9007199254740991LL; const TCHAR* Endpoint = TEXT("/api/v1/app/player-values"); }
void UHorizonPlayerValuesManager::Initialize(UHorizonHttpClient* Client, UHorizonAuthManager* Auth)
{
    HttpClient = Client; AuthManager = Auth;
    if (AuthManager) AuthManager->OnUserSignedOut.AddUniqueDynamic(this, &UHorizonPlayerValuesManager::ResetIdentity);
}
void UHorizonPlayerValuesManager::SyncIdentity()
{
    const FString Value = AuthManager && HttpClient ? AuthManager->GetCurrentUser().UserId + TEXT("|") + HttpClient->GetSessionToken() + TEXT("|") + HttpClient->GetApiKey() + TEXT("|") + HttpClient->GetActiveHost() : TEXT("");
    if (Value != Identity) { ResetIdentity(); Identity = Value; }
}
void UHorizonPlayerValuesManager::ResetIdentity()
{ Identity.Empty(); ++Epoch; Revision = Definition = ServerMillis = -1; State = {}; Offers = {}; LastErrorCode.Empty(); }
void UHorizonPlayerValuesManager::ClearCache() { SyncIdentity(); ++Epoch; State = {}; Offers = {}; }
bool UHorizonPlayerValuesManager::IsCurrent(const FString& SentIdentity, int64 SentEpoch) { SyncIdentity(); return Identity == SentIdentity && Epoch == SentEpoch; }
bool UHorizonPlayerValuesManager::Begin(const FString& UserId)
{
    SyncIdentity();
    if (!AuthManager || !HttpClient || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || AuthManager->GetCurrentUser().AccessToken != HttpClient->GetSessionToken()) LastErrorCode = TEXT("SESSION_REQUIRED");
    else if (AuthManager->GetCurrentUser().UserId != UserId) LastErrorCode = TEXT("SESSION_FORBIDDEN");
    else LastErrorCode.Empty();
    return LastErrorCode.IsEmpty();
}
FHorizonPlayerValuesState UHorizonPlayerValuesManager::GetCurrentState() { SyncIdentity(); return State; }
FHorizonPlayerValuesOffers UHorizonPlayerValuesManager::GetCurrentOffers() { SyncIdentity(); return Offers; }
bool UHorizonPlayerValuesManager::HasServerClock() { SyncIdentity(); return ServerMillis >= 0; }
int64 UHorizonPlayerValuesManager::TimeMillis(const FString& Value)
{
    FDateTime Time; if (!FDateTime::ParseIso8601(*Value, Time)) return -1;
    return Time.ToUnixTimestamp() * 1000 + Time.GetMillisecond();
}
bool UHorizonPlayerValuesManager::AcceptClock(const FString& Time)
{
    const int64 Millis = TimeMillis(Time); if (Millis < 0 || Millis < ServerMillis) return false;
    ServerMillis = Millis; ObservedAt = FPlatformTime::Seconds(); return true;
}
FString UHorizonPlayerValuesManager::CreateOperationId()
{
    SyncIdentity(); const FDateTime Utc = FDateTime::UtcNow();
    const int64 Millis = ServerMillis >= 0 ? ServerMillis + int64((FPlatformTime::Seconds() - ObservedAt) * 1000) : Utc.ToUnixTimestamp() * 1000 + Utc.GetMillisecond();
    return FString::Printf(TEXT("%lld:%s"), Millis, *FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
}
bool UHorizonPlayerValuesManager::ValidOperationId(const FString& Value)
{
    FString Time, Guid; int64 Millis; FGuid Parsed;
    return Value.Split(TEXT(":"), &Time, &Guid) && Time.Len() <= 16 && Time.IsNumeric() && LexTryParseString(Millis, *Time) && Millis > 0 && FGuid::ParseExact(Guid, EGuidFormats::DigitsWithHyphens, Parsed);
}
bool UHorizonPlayerValuesManager::AcceptState(const FHorizonPlayerValuesState& Value, const FString& UserId)
{
    if (Value.UserId != UserId || Value.Revision < Revision || Value.DefinitionVersion < Definition || TimeMillis(Value.ServerTime) < ServerMillis || !AcceptClock(Value.ServerTime)) return false;
    if (Value.DefinitionVersion > Definition) Offers = {};
    Revision = Value.Revision; Definition = Value.DefinitionVersion; State = Value; return true;
}
void UHorizonPlayerValuesManager::GetState(const FString& UserId, FOnPlayerValuesComplete OnComplete) { StateCall(UserId, TEXT(""), nullptr, OnComplete); }
void UHorizonPlayerValuesManager::Purchase(const FString& UserId, const FString& OfferKey, const FString& OperationId, FOnPlayerValuesComplete OnComplete)
{
    if (OfferKey.TrimStartAndEnd().IsEmpty() || !ValidOperationId(OperationId)) { LastErrorCode = TEXT("INVALID_REQUEST"); OnComplete.ExecuteIfBound(false, {}, TEXT("INVALID_REQUEST"), TEXT("Invalid purchase")); return; }
    const auto Body = MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"), UserId); Body->SetStringField(TEXT("offerKey"), OfferKey); Body->SetStringField(TEXT("operationId"), OperationId);
    StateCall(UserId, TEXT("/purchase"), Body, OnComplete);
}
void UHorizonPlayerValuesManager::Adjust(const FString& UserId, const FString& Key, int64 Amount, const FString& Reason, const FString& OperationId, FOnPlayerValuesComplete OnComplete)
{
    if (Key.TrimStartAndEnd().IsEmpty() || Reason.TrimStartAndEnd().IsEmpty() || Amount == 0 || Amount < -MaxSafe || Amount > MaxSafe || !ValidOperationId(OperationId)) { LastErrorCode = TEXT("INVALID_REQUEST"); OnComplete.ExecuteIfBound(false, {}, TEXT("INVALID_REQUEST"), TEXT("Invalid adjustment")); return; }
    const auto Body = MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"), UserId); Body->SetStringField(TEXT("key"), Key); Body->SetField(TEXT("amount"), MakeShared<FJsonValueNumberString>(LexToString(Amount))); Body->SetStringField(TEXT("reason"), Reason); Body->SetStringField(TEXT("operationId"), OperationId);
    StateCall(UserId, TEXT("/adjust"), Body, OnComplete);
}
void UHorizonPlayerValuesManager::StateCall(const FString& UserId, const FString& Route, const TSharedPtr<FJsonObject>& Body, FOnPlayerValuesComplete OnComplete)
{
    if (!Begin(UserId)) { OnComplete.ExecuteIfBound(false, {}, LastErrorCode, LastErrorCode); return; }
    const FString SentIdentity = Identity; const int64 SentEpoch = Epoch; TWeakObjectPtr<UHorizonPlayerValuesManager> Weak(this);
    const auto Complete = FOnHttpResponse::CreateLambda([Weak, UserId, SentIdentity, SentEpoch, OnComplete](const FHorizonNetworkResponse& Response)
    {
        auto* Self = Weak.Get(); if (!Self) return;
        if (!Self->IsCurrent(SentIdentity, SentEpoch)) { OnComplete.ExecuteIfBound(false, {}, TEXT("STALE_RESPONSE"), TEXT("Session or cache changed")); return; }
        auto Json = Response.JsonData;
        if (!Response.bSuccess) { const TSharedPtr<FJsonObject>* ErrorState; Json = Json && Json->TryGetObjectField(TEXT("state"), ErrorState) ? *ErrorState : nullptr; }
        FHorizonPlayerValuesState Value; const bool bPresent = FHorizonPlayerValuesState::TryFromJson(Json, Value);
        if (bPresent && !Self->AcceptState(Value, UserId)) { Self->LastErrorCode = TEXT("STALE_RESPONSE"); OnComplete.ExecuteIfBound(false, {}, Self->LastErrorCode, TEXT("Older balance reply")); return; }
        if (Response.bSuccess && !bPresent) { Self->LastErrorCode = TEXT("INVALID_RESPONSE"); OnComplete.ExecuteIfBound(false, {}, Self->LastErrorCode, TEXT("Invalid state")); return; }
        Self->LastErrorCode = Response.bSuccess ? TEXT("") : Response.GetErrorCodeString();
        OnComplete.ExecuteIfBound(Response.bSuccess, Value, Self->LastErrorCode, Response.ErrorMessage);
    });
    if (Route.IsEmpty()) HttpClient->Get(FString(Endpoint) + TEXT("?userId=") + FGenericPlatformHttp::UrlEncode(UserId), true, Complete);
    else HttpClient->PostJson(Body.ToSharedRef(), FString(Endpoint) + Route, true, Complete);
}
void UHorizonPlayerValuesManager::GetOffers(const FString& UserId, FOnPlayerValuesOffersComplete OnComplete)
{
    if (!Begin(UserId)) { OnComplete.ExecuteIfBound(false, {}, LastErrorCode, LastErrorCode); return; }
    const FString SentIdentity = Identity; const int64 SentEpoch = Epoch; TWeakObjectPtr<UHorizonPlayerValuesManager> Weak(this);
    HttpClient->Get(FString(Endpoint) + TEXT("/offers?userId=") + FGenericPlatformHttp::UrlEncode(UserId), true, FOnHttpResponse::CreateLambda([Weak, SentIdentity, SentEpoch, OnComplete](const FHorizonNetworkResponse& Response)
    {
        auto* Self = Weak.Get(); if (!Self) return;
        if (!Self->IsCurrent(SentIdentity, SentEpoch)) { OnComplete.ExecuteIfBound(false, {}, TEXT("STALE_RESPONSE"), TEXT("Session or cache changed")); return; }
        FHorizonPlayerValuesOffers Value;
        if (!Response.bSuccess || !FHorizonPlayerValuesOffers::TryFromJson(Response.JsonData, Value) || Value.DefinitionVersion < Self->Definition || !Self->AcceptClock(Value.ServerTime))
        { Self->LastErrorCode = Response.bSuccess ? TEXT("STALE_RESPONSE") : Response.GetErrorCodeString(); OnComplete.ExecuteIfBound(false, {}, Self->LastErrorCode, Response.ErrorMessage); return; }
        if (Value.DefinitionVersion > Self->Definition) { Self->Definition = Value.DefinitionVersion; Self->State = {}; }
        Self->Offers = Value; Self->LastErrorCode.Empty(); OnComplete.ExecuteIfBound(true, Value, TEXT(""), TEXT(""));
    }));
}
void UHorizonPlayerValuesManager::RequestState(const FString& UserId)
{ GetState(UserId, FOnPlayerValuesComplete::CreateWeakLambda(this, [this](bool Success, const FHorizonPlayerValuesState& Value, const FString& Code, const FString& Message) { OnValuesResult.Broadcast(Success, Value, Code, Message); })); }
void UHorizonPlayerValuesManager::RequestOffers(const FString& UserId)
{ GetOffers(UserId, FOnPlayerValuesOffersComplete::CreateWeakLambda(this, [this](bool Success, const FHorizonPlayerValuesOffers& Value, const FString& Code, const FString& Message) { OnOffersResult.Broadcast(Success, Value, Code, Message); })); }
void UHorizonPlayerValuesManager::PurchaseOffer(const FString& UserId, const FString& OfferKey, const FString& OperationId)
{ Purchase(UserId, OfferKey, OperationId, FOnPlayerValuesComplete::CreateWeakLambda(this, [this](bool Success, const FHorizonPlayerValuesState& Value, const FString& Code, const FString& Message) { OnValuesResult.Broadcast(Success, Value, Code, Message); })); }
void UHorizonPlayerValuesManager::AdjustValue(const FString& UserId, const FString& Key, int64 Amount, const FString& Reason, const FString& OperationId)
{ Adjust(UserId, Key, Amount, Reason, OperationId, FOnPlayerValuesComplete::CreateWeakLambda(this, [this](bool Success, const FHorizonPlayerValuesState& Value, const FString& Code, const FString& Message) { OnValuesResult.Broadcast(Success, Value, Code, Message); })); }
