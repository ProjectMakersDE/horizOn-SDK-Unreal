// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonCloudSaveManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Transport/HorizonCloudSaveTransportContract.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonCloudSaveManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonCloudSaveManager initialized."));
}

// ============================================================
// JSON Save / Load
// ============================================================

void UHorizonCloudSaveManager::Save(const FString& Data, FOnRequestComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Save -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);
	Body->SetStringField(TEXT("saveData"), Data);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/cloud-save/save"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::Save -- Data saved successfully."));
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Save -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonCloudSaveManager::SaveObject(const TMap<FString, FString>& Data, FOnRequestComplete OnComplete)
{
	TSharedRef<FJsonObject> ObjectJson = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Data)
	{
		ObjectJson->SetStringField(Pair.Key, Pair.Value);
	}

	FString SerializedData;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SerializedData);
	if (!FJsonSerializer::Serialize(ObjectJson, Writer))
	{
		OnComplete.ExecuteIfBound(false, TEXT("Failed to serialize cloud save object."));
		return;
	}

	Save(SerializedData, OnComplete);
}

void UHorizonCloudSaveManager::Load(FOnStringComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Load -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT(""));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnStringComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/cloud-save/load"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Load -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
					return;
				}

				bool bFound = false;
				FString SaveData;

				if (Response.JsonData.IsValid())
				{
					bFound = Response.JsonData->GetBoolField(TEXT("found"));
					if (bFound)
					{
						SaveData = Response.JsonData->GetStringField(TEXT("saveData"));
					}
				}

				if (bFound)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::Load -- Data loaded successfully."));
					CapturedOnComplete.ExecuteIfBound(true, SaveData);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::Load -- No save data found."));
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""));
				}
			}
		));
}

// ============================================================
// Binary Save / Load
// ============================================================

void UHorizonCloudSaveManager::SaveBytes(const TArray<uint8>& Data, FOnRequestComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::SaveBytes -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TEXT("User is not signed in."));
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	const FString Endpoint = FString::Printf(TEXT("api/v1/app/cloud-save/save?userId=%s"), *UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnRequestComplete CapturedOnComplete = OnComplete;

	HttpClient->PostBinary(Endpoint, Data, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::SaveBytes -- Binary data saved successfully."));
					CapturedOnComplete.ExecuteIfBound(true, TEXT(""));
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::SaveBytes -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, Response.ErrorMessage);
				}
			}
		));
}

void UHorizonCloudSaveManager::LoadBytes(FOnBinaryComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadBytes -- User is not signed in."));
		OnComplete.ExecuteIfBound(false, TArray<uint8>());
		return;
	}

	const FString UserId = AuthManager->GetCurrentUser().UserId;
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnBinaryComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJsonForBinary(Body, TEXT("api/v1/app/cloud-save/load"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.StatusCode == 204)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::LoadBytes -- No binary save data found."));
					CapturedOnComplete.ExecuteIfBound(false, TArray<uint8>());
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::LoadBytes -- Binary data loaded (%d bytes)."), Response.BinaryData.Num());
					CapturedOnComplete.ExecuteIfBound(true, Response.BinaryData);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadBytes -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<uint8>());
				}
			}
		));
}

// ============================================================
// Revision-checked saves (several devices)
// ============================================================

// FJsonObject::TryGetBoolField and TryGetStringField convert numbers and strings. A confirmation
// or a found flag must be a real JSON boolean, saveData a real JSON string.
static bool TryGetCloudSaveBool(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, bool& OutValue)
{
	const TSharedPtr<FJsonValue> Value = Json.IsValid() ? Json->TryGetField(Field) : nullptr;
	if (!Value.IsValid() || Value->Type != EJson::Boolean)
	{
		return false;
	}
	OutValue = Value->AsBool();
	return true;
}

static bool TryGetCloudSaveString(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, FString& OutValue)
{
	const TSharedPtr<FJsonValue> Value = Json.IsValid() ? Json->TryGetField(Field) : nullptr;
	if (!Value.IsValid() || Value->Type != EJson::String)
	{
		return false;
	}
	OutValue = Value->AsString();
	return true;
}

static FHorizonCloudSaveSnapshot CloudSaveSnapshotFrom(const FHorizonNetworkResponse& Response)
{
	FHorizonCloudSaveSnapshot Snapshot;
	Snapshot.StatusCode = Response.StatusCode;
	if (!Response.bSuccess)
	{
		Snapshot.ErrorMessage = Response.ErrorMessage;
		return Snapshot;
	}

	Snapshot.bSuccess = true;
	Snapshot.Revision = HorizonTransportContract::ParseCloudSaveRevision(
		TCHAR_TO_UTF8(*Response.GetHeader(UTF8_TO_TCHAR(HorizonTransportContract::CloudSaveRevisionHeader))));
	return Snapshot;
}

static FHorizonCloudSaveWriteResult CloudSaveWriteResultFrom(const FHorizonNetworkResponse& Response)
{
	FHorizonCloudSaveWriteResult Result;
	Result.StatusCode = Response.StatusCode;

	// Saved only when the server confirms the write with "success": true.
	bool bServerSuccess = false;
	if (Response.bSuccess && TryGetCloudSaveBool(Response.JsonData, TEXT("success"), bServerSuccess) && bServerSuccess)
	{
		double Revision = 0.0;
		double DataSizeBytes = 0.0;
		const bool bHasRevision = Response.JsonData->TryGetNumberField(TEXT("revision"), Revision);
		Response.JsonData->TryGetNumberField(TEXT("dataSizeBytes"), DataSizeBytes);
		Result.Status = EHorizonCloudSaveWriteStatus::Saved;
		Result.Revision = HorizonTransportContract::CloudSaveRevisionFromSave(bHasRevision, Revision);
		Result.DataSizeBytes = static_cast<int64>(DataSizeBytes);
	}
	else if (Response.StatusCode == 409)
	{
		// Never retried: the game decides after loading a new snapshot.
		Result.Status = EHorizonCloudSaveWriteStatus::Conflict;
		Result.ErrorMessage = TEXT("Cloud save changed on another device (HTTP 409). Nothing was written. Load a new snapshot before saving again.");
	}
	else if (Response.bSuccess)
	{
		Result.ErrorMessage = TEXT("Cloud save response did not confirm the write.");
	}
	else
	{
		Result.ErrorMessage = Response.ErrorMessage.IsEmpty() ? FString(TEXT("Cloud save failed.")) : Response.ErrorMessage;
	}
	return Result;
}

static FString CheckConditionalSave(bool bDataMissing, int64 ExpectedRevision, const UHorizonAuthManager* AuthManager)
{
	if (bDataMissing)
	{
		return TEXT("Save data is required.");
	}
	if (ExpectedRevision < 0)
	{
		return TEXT("ExpectedRevision must be 0 (empty slot) or a revision from a snapshot or save.");
	}
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		return TEXT("User is not signed in.");
	}
	return FString();
}

static TMap<FString, FString> CloudSaveIfMatch(int64 ExpectedRevision)
{
	const auto Header = HorizonTransportContract::BuildCloudSaveIfMatch(ExpectedRevision);
	TMap<FString, FString> Headers;
	Headers.Add(UTF8_TO_TCHAR(Header.first.c_str()), UTF8_TO_TCHAR(Header.second.c_str()));
	return Headers;
}

static void LogCloudSaveWrite(const TCHAR* Operation, const FHorizonCloudSaveWriteResult& Result)
{
	switch (Result.Status)
	{
	case EHorizonCloudSaveWriteStatus::Saved:
		UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::%s -- Saved (%lld bytes, revision %lld)."), Operation, Result.DataSizeBytes, Result.Revision);
		break;
	case EHorizonCloudSaveWriteStatus::Conflict:
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::%s -- Conflict: %s"), Operation, *Result.ErrorMessage);
		break;
	default:
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::%s -- Failed: %s"), Operation, *Result.ErrorMessage);
		break;
	}
}

void UHorizonCloudSaveManager::LoadSnapshot(FOnCloudSaveSnapshotComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadSnapshot -- User is not signed in."));
		FHorizonCloudSaveSnapshot Snapshot;
		Snapshot.ErrorMessage = TEXT("User is not signed in.");
		OnComplete.ExecuteIfBound(Snapshot);
		return;
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), AuthManager->GetCurrentUser().UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnCloudSaveSnapshotComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/cloud-save/load"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				FHorizonCloudSaveSnapshot Snapshot = CloudSaveSnapshotFrom(Response);
				if (Snapshot.bSuccess)
				{
					// An unreadable answer must not look like an empty slot: saving at its revision
					// would overwrite the stored save.
					const bool bReadable = TryGetCloudSaveBool(Response.JsonData, TEXT("found"), Snapshot.bFound)
						&& (!Snapshot.bFound || TryGetCloudSaveString(Response.JsonData, TEXT("saveData"), Snapshot.Data));
					if (!bReadable)
					{
						Snapshot = FHorizonCloudSaveSnapshot();
						Snapshot.StatusCode = Response.StatusCode;
						Snapshot.ErrorMessage = TEXT("Cloud save load returned an unreadable response.");
					}
				}

				if (Snapshot.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::LoadSnapshot -- found=%d, revision %lld."), Snapshot.bFound ? 1 : 0, Snapshot.Revision);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadSnapshot -- Failed: %s"), *Snapshot.ErrorMessage);
				}
				CapturedOnComplete.ExecuteIfBound(Snapshot);
			}
		));
}

void UHorizonCloudSaveManager::LoadBytesSnapshot(FOnCloudSaveSnapshotComplete OnComplete)
{
	if (!AuthManager || !AuthManager->IsSignedIn())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadBytesSnapshot -- User is not signed in."));
		FHorizonCloudSaveSnapshot Snapshot;
		Snapshot.ErrorMessage = TEXT("User is not signed in.");
		OnComplete.ExecuteIfBound(Snapshot);
		return;
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), AuthManager->GetCurrentUser().UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnCloudSaveSnapshotComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJsonForBinary(Body, TEXT("api/v1/app/cloud-save/load"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				// HTTP 204 is an empty slot (revision 0), the client reports it as success.
				FHorizonCloudSaveSnapshot Snapshot = CloudSaveSnapshotFrom(Response);
				if (Snapshot.bSuccess && Response.StatusCode != 204)
				{
					Snapshot.bFound = true;
					Snapshot.BinaryData = Response.BinaryData;
				}

				if (Snapshot.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CloudSave::LoadBytesSnapshot -- %d bytes, revision %lld."), Snapshot.BinaryData.Num(), Snapshot.Revision);
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::LoadBytesSnapshot -- Failed: %s"), *Snapshot.ErrorMessage);
				}
				CapturedOnComplete.ExecuteIfBound(Snapshot);
			}
		));
}

void UHorizonCloudSaveManager::Save(const FString& Data, int64 ExpectedRevision, FOnCloudSaveWriteComplete OnComplete)
{
	const FString LocalError = CheckConditionalSave(Data.IsEmpty(), ExpectedRevision, AuthManager);
	if (!LocalError.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::Save -- %s"), *LocalError);
		FHorizonCloudSaveWriteResult Result;
		Result.ErrorMessage = LocalError;
		OnComplete.ExecuteIfBound(Result);
		return;
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("userId"), AuthManager->GetCurrentUser().UserId);
	Body->SetStringField(TEXT("saveData"), Data);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnCloudSaveWriteComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/cloud-save/save"), true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				const FHorizonCloudSaveWriteResult Result = CloudSaveWriteResultFrom(Response);
				LogCloudSaveWrite(TEXT("Save"), Result);
				CapturedOnComplete.ExecuteIfBound(Result);
			}
		),
		CloudSaveIfMatch(ExpectedRevision));
}

void UHorizonCloudSaveManager::SaveBytes(const TArray<uint8>& Data, int64 ExpectedRevision, FOnCloudSaveWriteComplete OnComplete)
{
	const FString LocalError = CheckConditionalSave(Data.Num() == 0, ExpectedRevision, AuthManager);
	if (!LocalError.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CloudSave::SaveBytes -- %s"), *LocalError);
		FHorizonCloudSaveWriteResult Result;
		Result.ErrorMessage = LocalError;
		OnComplete.ExecuteIfBound(Result);
		return;
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/cloud-save/save?userId=%s"), *AuthManager->GetCurrentUser().UserId);

	TWeakObjectPtr<UHorizonCloudSaveManager> WeakSelf(this);
	FOnCloudSaveWriteComplete CapturedOnComplete = OnComplete;

	HttpClient->PostBinary(Endpoint, Data, true,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				const FHorizonCloudSaveWriteResult Result = CloudSaveWriteResultFrom(Response);
				LogCloudSaveWrite(TEXT("SaveBytes"), Result);
				CapturedOnComplete.ExecuteIfBound(Result);
			}
		),
		CloudSaveIfMatch(ExpectedRevision));
}
