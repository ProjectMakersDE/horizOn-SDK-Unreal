// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.
#include "Managers/HorizonStatisticsManager.h"
#include "Transport/HorizonStatisticsTransportContract.h"
#include "Dom/JsonObject.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "Managers/HorizonAchievementsManager.h"
#include "Dom/JsonValue.h"
#include "GenericPlatform/GenericPlatformHttp.h"

namespace { int64 StatisticsNow() { const auto Now = FDateTime::UtcNow(); return Now.ToUnixTimestamp() * 1000 + Now.GetMillisecond(); } }

void UHorizonStatisticsManager::Initialize(UHorizonHttpClient* Http, UHorizonAuthManager* Auth, UHorizonPlayerValuesManager* Values, UHorizonAchievementsManager* Achievements)
{
	HttpClient = Http; AuthManager = Auth; PlayerValues = Values; AchievementManager = Achievements;
	Auth->OnUserSignedIn.AddUniqueDynamic(this, &UHorizonStatisticsManager::OnSignedIn);
	Auth->OnUserSignedOut.AddUniqueDynamic(this, &UHorizonStatisticsManager::OnSignedOut);
	SyncScope();
}
UHorizonStatisticsManager::FScope UHorizonStatisticsManager::Scope() const
{
	return {AuthManager ? AuthManager->GetCurrentUser().UserId : FString(), HttpClient ? HttpClient->GetSessionToken() : FString(),
		HttpClient ? HttpClient->GetActiveHost() : FString(), HttpClient ? HttpClient->GetApiKey() : FString()};
}
void UHorizonStatisticsManager::SyncScope()
{
	const auto Active = Scope();
	if (!(Active == LastScope)) { ClearCache(); LastScope = Active; }
}
bool UHorizonStatisticsManager::HasSession() const
{
	return HttpClient && AuthManager && AuthManager->IsSignedIn() && !HttpClient->GetSessionToken().IsEmpty()
		&& AuthManager->GetCurrentUser().AccessToken == HttpClient->GetSessionToken();
}
bool UHorizonStatisticsManager::SameScope(const FScope& Expected, int64 Epoch)
{
	SyncScope();
	if (!(Scope() == Expected)) { LastErrorCode = TEXT("SESSION_CHANGED"); return false; }
	if (Epoch != CacheEpoch) { LastErrorCode = TEXT("STATE_CONFLICT"); return false; }
	return true;
}
void UHorizonStatisticsManager::ClearCache()
{
	Current = FHorizonStatisticsSnapshot(); Catalog = FHorizonStatisticsCatalog(); MaxRevision = -1; LastServerTime = 0; ClockOffset = 0;
	++CacheEpoch; LastErrorCode.Empty(); LastOperationId.Empty();
}
void UHorizonStatisticsManager::OnSignedIn() { ClearCache(); }
void UHorizonStatisticsManager::OnSignedOut() { ClearCache(); }
FHorizonStatisticsSnapshot UHorizonStatisticsManager::GetCurrentStatistics() { SyncScope(); return Current; }
FHorizonStatisticsCatalog UHorizonStatisticsManager::GetCurrentDefinitions() { SyncScope(); return Catalog; }
FString UHorizonStatisticsManager::CreateOperationId()
{
	SyncScope();
	auto Uuid = FGuid::NewGuid();
	Uuid.B = (Uuid.B & 0xffff0fffU) | 0x00004000U;
	Uuid.C = (Uuid.C & 0x3fffffffU) | 0x80000000U;
	return FString::Printf(TEXT("%lld:%s"), StatisticsNow() + ClockOffset, *Uuid.ToString(EGuidFormats::DigitsWithHyphensLower));
}
void UHorizonStatisticsManager::Fail(const FString& Code, FOnStatisticsLoaded Complete, const FString& Message)
{
	LastErrorCode = Code; Complete.ExecuteIfBound(false, FHorizonStatisticsSnapshot(), Code, Message);
}
bool UHorizonStatisticsManager::AcceptCatalog(const FHorizonStatisticsCatalog& Incoming)
{
	if (!Incoming.bValid || Incoming.DefinitionVersion < Catalog.DefinitionVersion) return false;
	if (!Incoming.bDefinitionsIncluded) return Catalog.bValid && Incoming.DefinitionVersion == Catalog.DefinitionVersion;
	if (Incoming.DefinitionVersion != Catalog.DefinitionVersion) Current = FHorizonStatisticsSnapshot();
	Catalog = Incoming; return true;
}
bool UHorizonStatisticsManager::AcceptSnapshot(FHorizonStatisticsSnapshot& Incoming)
{
	LastErrorCode = TEXT("INVALID_RESPONSE");
	if (!Incoming.bValid || Incoming.UserId != Scope().User) return false;
	if (Incoming.Revision < MaxRevision || Incoming.DefinitionVersion < Catalog.DefinitionVersion || Incoming.OperationWindow.ServerTimeUnixMillis < LastServerTime)
	{ LastErrorCode = TEXT("STATE_CONFLICT"); return false; }
	FHorizonStatisticsCatalog Next;
	if (Incoming.bDefinitionsIncluded) { Next.DefinitionVersion = Incoming.DefinitionVersion; Next.Definitions = Incoming.Definitions; Next.bValid = Next.bDefinitionsIncluded = true; }
	else if (Catalog.bValid && Incoming.DefinitionVersion == Catalog.DefinitionVersion) Next = Catalog;
	else return false;
	if (Incoming.Values.Num() != Next.Definitions.Num()) return false;
	for (const auto& Definition : Next.Definitions)
		if (!Incoming.Values.ContainsByPredicate([&Definition](const FHorizonStatisticValue& Value) { return Value.Key == Definition.Key; })) return false;
	Catalog = Next; Incoming.Definitions = Catalog.Definitions; Incoming.bDefinitionsIncluded = true;
	Current = Incoming; MaxRevision = Incoming.Revision; LastServerTime = Incoming.OperationWindow.ServerTimeUnixMillis;
	ClockOffset = LastServerTime - StatisticsNow(); LastErrorCode.Empty(); return true;
}
void UHorizonStatisticsManager::HandleSnapshot(const FHorizonNetworkResponse& Response, const FScope& Expected, int64 Epoch, const FString& OperationId, FOnStatisticsLoaded Complete)
{
	if (!SameScope(Expected, Epoch)) { Fail(LastErrorCode, Complete); return; }
	if (!Response.bSuccess)
	{
		if (Response.StatusCode == 401) ClearCache();
		Fail(Response.GetErrorCodeString(), Complete, Response.ErrorMessage); return;
	}
	auto Snapshot = FHorizonStatisticsSnapshot::FromJson(Response.JsonData);
	if (!OperationId.IsEmpty() && Snapshot.OperationId != OperationId) { Fail(TEXT("INVALID_RESPONSE"), Complete); return; }
	if (!AcceptSnapshot(Snapshot)) { Fail(LastErrorCode, Complete); return; }
	Complete.ExecuteIfBound(true, Snapshot, TEXT(""), TEXT(""));
}
void UHorizonStatisticsManager::GetStatistics(FOnStatisticsLoaded Complete)
{
	SyncScope(); if (!HasSession()) { Fail(TEXT("SESSION_REQUIRED"), Complete); return; }
	const auto Expected = Scope(); const int64 Epoch = CacheEpoch;
	FString Endpoint = TEXT("/api/v1/app/statistics?userId=") + FGenericPlatformHttp::UrlEncode(Expected.User);
	if (Catalog.bValid) Endpoint += FString::Printf(TEXT("&definitionVersion=%lld"), Catalog.DefinitionVersion);
	TWeakObjectPtr<UHorizonStatisticsManager> Weak(this);
	HttpClient->Get(Endpoint, true, FOnHttpResponse::CreateLambda([Weak, Expected, Epoch, Complete](const FHorizonNetworkResponse& Response)
	{ if (auto* Self = Weak.Get()) Self->HandleSnapshot(Response, Expected, Epoch, TEXT(""), Complete); }));
}
void UHorizonStatisticsManager::GetDefinitions(FOnStatisticsDefinitionsLoaded Complete)
{
	SyncScope();
	if (!HasSession()) { LastErrorCode = TEXT("SESSION_REQUIRED"); Complete.ExecuteIfBound(false, FHorizonStatisticsCatalog(), LastErrorCode, TEXT("Player session required")); return; }
	const auto Expected = Scope(); const int64 Epoch = CacheEpoch;
	FString Endpoint = TEXT("/api/v1/app/statistics/definitions");
	if (Catalog.bValid) Endpoint += FString::Printf(TEXT("?definitionVersion=%lld"), Catalog.DefinitionVersion);
	TWeakObjectPtr<UHorizonStatisticsManager> Weak(this);
	HttpClient->Get(Endpoint, true, FOnHttpResponse::CreateLambda([Weak, Expected, Epoch, Complete](const FHorizonNetworkResponse& Response)
	{
		auto* Self = Weak.Get(); if (!Self) return;
		if (!Self->SameScope(Expected, Epoch)) { Complete.ExecuteIfBound(false, FHorizonStatisticsCatalog(), Self->LastErrorCode, TEXT("Request context changed")); return; }
		if (!Response.bSuccess)
		{
			if (Response.StatusCode == 401) Self->ClearCache();
			Self->LastErrorCode = Response.GetErrorCodeString(); Complete.ExecuteIfBound(false, FHorizonStatisticsCatalog(), Self->LastErrorCode, Response.ErrorMessage); return;
		}
		if (!Self->AcceptCatalog(FHorizonStatisticsCatalog::FromJson(Response.JsonData)))
		{ Self->LastErrorCode = TEXT("INVALID_RESPONSE"); Complete.ExecuteIfBound(false, FHorizonStatisticsCatalog(), Self->LastErrorCode, TEXT("Invalid or stale catalog")); return; }
		Self->LastErrorCode.Empty(); Complete.ExecuteIfBound(true, Self->Catalog, TEXT(""), TEXT(""));
	}));
}
void UHorizonStatisticsManager::UpdateStatistics(const TArray<FHorizonStatisticUpdate>& Updates, const FString& OperationId, FOnStatisticsLoaded Complete)
{
	SyncScope(); if (!HasSession()) { Fail(TEXT("SESSION_REQUIRED"), Complete); return; }
	std::vector<std::pair<std::string, int64_t>> Validation;
	for (const auto& Update : Updates) Validation.emplace_back(TCHAR_TO_UTF8(*Update.Key), Update.Value);
	if (!HorizonTransportContract::IsStatisticsOperationId(TCHAR_TO_UTF8(*OperationId)) || !HorizonTransportContract::IsStatisticsBatch(Validation))
	{ Fail(TEXT("INVALID_REQUEST"), Complete); return; }
	const auto Expected = Scope(); const int64 Epoch = CacheEpoch; LastOperationId = OperationId;
	auto Body = MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"), Expected.User); Body->SetStringField(TEXT("operationId"), OperationId);
	TArray<TSharedPtr<FJsonValue>> JsonUpdates;
	for (const auto& Update : Updates)
	{
		auto Entry = MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("key"), Update.Key); Entry->SetNumberField(TEXT("value"), static_cast<double>(Update.Value));
		JsonUpdates.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Body->SetArrayField(TEXT("updates"), JsonUpdates);
	// An uncertain response may already have atomically unlocked and credited rewards.
	if (PlayerValues) PlayerValues->ClearCache();
	if (AchievementManager) AchievementManager->ClearCache();
	TWeakObjectPtr<UHorizonStatisticsManager> Weak(this);
	HttpClient->PostJson(Body, TEXT("/api/v1/app/statistics"), true, FOnHttpResponse::CreateLambda([Weak, Expected, Epoch, OperationId, Complete](const FHorizonNetworkResponse& Response)
	{ if (auto* Self = Weak.Get()) Self->HandleSnapshot(Response, Expected, Epoch, OperationId, Complete); }));
}
