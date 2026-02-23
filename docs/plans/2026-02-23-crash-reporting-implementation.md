# Crash Reporting — Unreal SDK Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a `UHorizonCrashManager` to the horizOn Unreal SDK that captures crashes, non-fatal errors, breadcrumbs, and device info, then sends structured reports to the horizOn backend.

**Architecture:** Auth-dependent UObject manager following the existing Subsystem + Managers pattern. Ring buffer breadcrumbs, token-bucket rate limiting, SHA-256 fingerprinting. Automatic crash capture via `FCoreDelegates::OnHandleSystemError` and a custom `FOutputDevice`. Blueprint integration via async actions and blueprint library helpers.

**Tech Stack:** C++ (UE5.5+), `FHttpModule`, `FSHA256`, `FCoreDelegates`, `FOutputDevice`, `UBlueprintAsyncActionBase`

**Design doc:** `docs/plans/2026-02-23-crash-reporting-design.md`

**Universal spec:** `ansible-horizon/docs/plans/2026-02-22-crash-reporting-sdk-logic.md`

---

## Phase 1: Types & Delegates

### Task 1: Add crash reporting types to HorizonTypes.h

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonTypes.h:52-67`

**Step 1: Add EHorizonCrashType enum**

After the `EHorizonErrorCode` enum (after line 52), add:

```cpp
UENUM(BlueprintType)
enum class EHorizonCrashType : uint8
{
	Crash       UMETA(DisplayName = "Crash"),
	NonFatal    UMETA(DisplayName = "Non-Fatal"),
	Anr         UMETA(DisplayName = "ANR")
};
```

**Step 2: Add crash event delegates**

After the existing dynamic multicast delegates (after line 60), add:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHorizonCrashReported, const FString&, ReportId, const FString&, GroupId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonCrashReportFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonCrashSessionRegistered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonBreadcrumbRecorded, const FString&, Message);
```

**Step 3: Add C++ crash delegates**

After the existing C++ delegates (after line 67), add:

```cpp
DECLARE_DELEGATE_ThreeParams(FOnCrashReportComplete, bool /*bSuccess*/, const FString& /*ReportId*/, const FString& /*GroupId*/);
DECLARE_DELEGATE_OneParam(FOnCrashSessionComplete, bool /*bSuccess*/);
```

**Step 4: Commit**

```
feat(crash): add CrashType enum and crash reporting delegates
```

---

## Phase 2: CrashManager Header

### Task 2: Create HorizonCrashManager header

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonCrashManager.h`

**Step 1: Write the full header**

```cpp
// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Misc/OutputDevice.h"
#include "HorizonCrashManager.generated.h"

/**
 * Crash Reporting Manager for the horizOn SDK.
 *
 * Captures fatal crashes, non-fatal errors, and contextual data
 * (breadcrumbs, device info, custom keys), then sends structured
 * reports to the horizOn backend.
 *
 * Call StartCapture() to begin automatic crash capture and session
 * registration. Call StopCapture() to unhook crash handlers.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonCrashManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	// --- Lifecycle ---

	/** Hook crash handlers and register a new session with the backend. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void StartCapture();

	/** Unhook crash handlers. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void StopCapture();

	// --- Manual Reporting ---

	/** Report a non-fatal caught exception (NON_FATAL type). */
	void RecordException(
		const FString& Message,
		const FString& StackTrace = TEXT(""),
		const TMap<FString, FString>& ExtraKeys = {},
		FOnCrashReportComplete OnComplete = {});

	/** Report a fatal crash (CRASH type). */
	void ReportCrash(
		const FString& Message,
		const FString& StackTrace = TEXT(""),
		FOnCrashReportComplete OnComplete = {});

	// --- Context ---

	/** Add a typed breadcrumb to the ring buffer. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void RecordBreadcrumb(const FString& Type, const FString& Message);

	/** Shorthand: add a "log" type breadcrumb. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void Log(const FString& Message);

	/** Set a persistent custom key-value pair (max 10). */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void SetCustomKey(const FString& Key, const FString& Value);

	/** Override user ID for crash attribution. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void SetUserId(const FString& InUserId);

	// --- State ---

	/** Returns true if crash capture is currently active. */
	UFUNCTION(BlueprintPure, Category = "horizOn|CrashReport")
	bool IsCapturing() const { return bIsCapturing; }

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonCrashReported OnCrashReported;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonCrashReportFailed OnCrashReportFailed;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonCrashSessionRegistered OnSessionRegistered;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonBreadcrumbRecorded OnBreadcrumbRecorded;

	// --- Auto-breadcrumb handlers (called by Subsystem wiring) ---

	UFUNCTION()
	void OnAutoUserSignedIn();

	UFUNCTION()
	void OnAutoUserSignedOut();

	UFUNCTION()
	void OnAutoConnected();

	UFUNCTION()
	void OnAutoConnectionFailed(const FString& ErrorMessage);

private:
	// Dependencies
	UPROPERTY()
	UHorizonHttpClient* HttpClient = nullptr;

	UPROPERTY()
	UHorizonAuthManager* AuthManager = nullptr;

	// State
	bool bIsCapturing = false;
	FString SessionId;
	FString UserIdOverride;

	// Device info (cached once at init)
	FString CachedPlatform;
	FString CachedOS;
	FString CachedDeviceModel;
	int32 CachedDeviceMemoryMb = 0;

	// SDK version sent with reports
	static const FString SdkVersion;

	// Breadcrumb ring buffer
	static constexpr int32 MaxBreadcrumbs = 50;
	struct FBreadcrumb
	{
		FString Timestamp;
		FString Type;
		FString Message;
	};
	TArray<FBreadcrumb> Breadcrumbs;
	int32 BreadcrumbHead = 0;
	int32 BreadcrumbCount = 0;

	// Custom keys
	static constexpr int32 MaxCustomKeys = 10;
	TMap<FString, FString> CustomKeys;

	// Rate limiting (token bucket)
	static constexpr int32 TokensPerMinute = 5;
	static constexpr int32 MaxSessionReports = 20;
	static constexpr double RefillIntervalSeconds = 60.0;
	double Tokens = 5.0;
	double LastRefillTime = 0.0;
	int32 SessionReportCount = 0;

	// Internal methods
	void CacheDeviceInfo();
	FString GenerateSessionId() const;
	FString GenerateFingerprint(const FString& StackTrace) const;
	FString NormalizeFrame(const FString& Frame) const;
	bool IsEngineFrame(const FString& Frame) const;
	FString HashSHA256(const FString& Input) const;
	void RefillTokens();
	bool ConsumeRateLimitToken();
	FString ResolveUserId() const;
	TArray<TSharedRef<FJsonObject>> CollectBreadcrumbsJson() const;
	void AddBreadcrumb(const FString& Type, const FString& Message);
	void RegisterSession();
	void SubmitReport(
		EHorizonCrashType Type,
		const FString& Message,
		const FString& StackTrace,
		const TMap<FString, FString>& ExtraKeys,
		FOnCrashReportComplete OnComplete);
	static FString CrashTypeToString(EHorizonCrashType Type);

	// Crash handler delegates
	FDelegateHandle SystemErrorHandle;

	// Custom FOutputDevice for UE_LOG capture
	class FHorizonCrashOutputDevice : public FOutputDevice
	{
	public:
		FHorizonCrashOutputDevice(UHorizonCrashManager* InOwner);
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;
	private:
		TWeakObjectPtr<UHorizonCrashManager> Owner;
	};

	FHorizonCrashOutputDevice* OutputDevice = nullptr;
	void OnEngineError();
	void OnLogMessage(const FString& Message, ELogVerbosity::Type Verbosity);
};
```

**Step 2: Commit**

```
feat(crash): add CrashManager header with full API surface
```

---

## Phase 3: CrashManager Implementation

### Task 3: Implement CrashManager initialization and device info

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp`

**Step 1: Write initialization and device info caching**

```cpp
// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonCrashManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GenericPlatform/GenericPlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/SecureHash.h"

const FString UHorizonCrashManager::SdkVersion = TEXT("unreal-1.0.0");

// ============================================================
// Initialization
// ============================================================

void UHorizonCrashManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;

	CacheDeviceInfo();

	// Pre-allocate breadcrumb ring buffer
	Breadcrumbs.SetNum(MaxBreadcrumbs);

	// Initialize rate limiter
	Tokens = static_cast<double>(TokensPerMinute);
	LastRefillTime = FPlatformTime::Seconds();

	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonCrashManager initialized."));
}

void UHorizonCrashManager::CacheDeviceInfo()
{
	CachedPlatform = FPlatformProperties::PlatformName();
	CachedOS = FPlatformMisc::GetOSVersion();
	CachedDeviceModel = FPlatformMisc::GetDeviceMakeAndModel();
	CachedDeviceMemoryMb = static_cast<int32>(FPlatformMemory::GetConstants().TotalPhysical / (1024 * 1024));
}
```

**Step 2: Commit (partial — continue in next tasks)**

Do NOT commit yet. Continue adding to this file in subsequent tasks.

---

### Task 4: Implement session ID, user ID resolution, and CrashType helpers

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp` (append)

**Step 1: Add session ID generation, user ID resolution, and type conversion**

```cpp
// ============================================================
// Session ID Generation
// ============================================================

FString UHorizonCrashManager::GenerateSessionId() const
{
	TArray<uint8> Bytes;
	Bytes.SetNum(16);
	for (int32 i = 0; i < 16; ++i)
	{
		Bytes[i] = static_cast<uint8>(FMath::RandRange(0, 255));
	}
	return BytesToHex(Bytes.GetData(), Bytes.Num()).ToLower();
}

// ============================================================
// User ID Resolution
// ============================================================

FString UHorizonCrashManager::ResolveUserId() const
{
	// 1. Explicit override
	if (!UserIdOverride.IsEmpty())
	{
		return UserIdOverride;
	}

	// 2. Authenticated user
	if (AuthManager && AuthManager->IsSignedIn())
	{
		return AuthManager->GetCurrentUser().UserId;
	}

	// 3. Fallback
	return TEXT("");
}

void UHorizonCrashManager::SetUserId(const FString& InUserId)
{
	UserIdOverride = InUserId;
}

// ============================================================
// Crash Type Helpers
// ============================================================

FString UHorizonCrashManager::CrashTypeToString(EHorizonCrashType Type)
{
	switch (Type)
	{
	case EHorizonCrashType::Crash:    return TEXT("CRASH");
	case EHorizonCrashType::NonFatal: return TEXT("NON_FATAL");
	case EHorizonCrashType::Anr:      return TEXT("ANR");
	default:                          return TEXT("CRASH");
	}
}
```

---

### Task 5: Implement breadcrumb ring buffer

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp` (append)

**Step 1: Add breadcrumb logic**

```cpp
// ============================================================
// Breadcrumb Ring Buffer
// ============================================================

void UHorizonCrashManager::AddBreadcrumb(const FString& Type, const FString& Message)
{
	FBreadcrumb& Entry = Breadcrumbs[BreadcrumbHead];
	Entry.Timestamp = FDateTime::UtcNow().ToIso8601();
	Entry.Type = Type;
	Entry.Message = Message;

	BreadcrumbHead = (BreadcrumbHead + 1) % MaxBreadcrumbs;
	BreadcrumbCount++;
}

void UHorizonCrashManager::RecordBreadcrumb(const FString& Type, const FString& Message)
{
	AddBreadcrumb(Type, Message);
	OnBreadcrumbRecorded.Broadcast(Message);
}

void UHorizonCrashManager::Log(const FString& Message)
{
	RecordBreadcrumb(TEXT("log"), Message);
}

TArray<TSharedRef<FJsonObject>> UHorizonCrashManager::CollectBreadcrumbsJson() const
{
	TArray<TSharedRef<FJsonObject>> Result;

	const int32 Count = FMath::Min(BreadcrumbCount, MaxBreadcrumbs);
	if (Count == 0)
	{
		return Result;
	}

	Result.Reserve(Count);

	if (BreadcrumbCount <= MaxBreadcrumbs)
	{
		// Buffer hasn't wrapped — read linearly from 0
		for (int32 i = 0; i < Count; ++i)
		{
			TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("timestamp"), Breadcrumbs[i].Timestamp);
			Obj->SetStringField(TEXT("type"), Breadcrumbs[i].Type);
			Obj->SetStringField(TEXT("message"), Breadcrumbs[i].Message);
			Result.Add(Obj);
		}
	}
	else
	{
		// Buffer has wrapped — start from BreadcrumbHead (oldest entry)
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Idx = (BreadcrumbHead + i) % MaxBreadcrumbs;
			TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("timestamp"), Breadcrumbs[Idx].Timestamp);
			Obj->SetStringField(TEXT("type"), Breadcrumbs[Idx].Type);
			Obj->SetStringField(TEXT("message"), Breadcrumbs[Idx].Message);
			Result.Add(Obj);
		}
	}

	return Result;
}
```

---

### Task 6: Implement custom keys and rate limiting

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp` (append)

**Step 1: Add custom keys and rate limiting**

```cpp
// ============================================================
// Custom Keys
// ============================================================

void UHorizonCrashManager::SetCustomKey(const FString& Key, const FString& Value)
{
	if (Key.IsEmpty())
	{
		return;
	}

	if (CustomKeys.Num() >= MaxCustomKeys && !CustomKeys.Contains(Key))
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::SetCustomKey -- Max custom keys (%d) reached. Ignoring key: %s"), MaxCustomKeys, *Key);
		return;
	}

	CustomKeys.Add(Key, Value);
}

// ============================================================
// Rate Limiting (Token Bucket)
// ============================================================

void UHorizonCrashManager::RefillTokens()
{
	const double Now = FPlatformTime::Seconds();
	const double Elapsed = Now - LastRefillTime;

	if (Elapsed >= RefillIntervalSeconds)
	{
		const int32 Refills = static_cast<int32>(Elapsed / RefillIntervalSeconds);
		Tokens = FMath::Min(Tokens + Refills * static_cast<double>(TokensPerMinute), static_cast<double>(TokensPerMinute));
		LastRefillTime = Now;
	}
}

bool UHorizonCrashManager::ConsumeRateLimitToken()
{
	RefillTokens();

	if (Tokens < 1.0)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager -- Rate limited: per-minute limit reached. Report dropped."));
		OnCrashReportFailed.Broadcast(TEXT("Rate limited: per-minute limit reached."));
		return false;
	}

	if (SessionReportCount >= MaxSessionReports)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager -- Rate limited: session limit (%d) reached. Report dropped."), MaxSessionReports);
		OnCrashReportFailed.Broadcast(TEXT("Rate limited: session limit reached."));
		return false;
	}

	Tokens -= 1.0;
	SessionReportCount++;
	return true;
}
```

---

### Task 7: Implement fingerprint generation

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp` (append)

**Step 1: Add fingerprint generation with SHA-256**

```cpp
// ============================================================
// Fingerprint Generation
// ============================================================

bool UHorizonCrashManager::IsEngineFrame(const FString& Frame) const
{
	// Skip known UE5 engine-internal frame prefixes
	static const TArray<FString> EngineFramePrefixes = {
		TEXT("FGenericPlatformMisc"),
		TEXT("FWindowsPlatformStackWalk"),
		TEXT("FWindowsPlatformMisc"),
		TEXT("FLinuxPlatformStackWalk"),
		TEXT("FLinuxPlatformMisc"),
		TEXT("FMacPlatformStackWalk"),
		TEXT("FMacPlatformMisc"),
		TEXT("UnrealEngine"),
		TEXT("CoreUObject"),
		TEXT("Core_"),
		TEXT("Engine_"),
		TEXT("FDebug::"),
		TEXT("FOutputDevice::"),
		TEXT("raise"),
		TEXT("abort"),
		TEXT("CommonUnixCrashHandler"),
	};

	for (const FString& Prefix : EngineFramePrefixes)
	{
		if (Frame.StartsWith(Prefix))
		{
			return true;
		}
	}
	return false;
}

FString UHorizonCrashManager::NormalizeFrame(const FString& Frame) const
{
	FString Result = Frame.TrimStartAndEnd();

	// Strip file paths: " (at ...)" or " in ..." or "[...]" file refs
	{
		int32 AtIdx;
		if (Result.FindChar(TEXT('('), AtIdx))
		{
			Result = Result.Left(AtIdx).TrimEnd();
		}
	}

	// Strip memory addresses: 0x followed by hex chars
	{
		FRegexPattern AddrPattern(TEXT("0x[0-9a-fA-F]+"));
		FRegexMatcher Matcher(AddrPattern, Result);
		while (Matcher.FindNext())
		{
			Result.ReplaceInline(*Matcher.GetCaptureGroup(0), TEXT(""));
		}
	}

	// Strip line numbers: :digits at end of tokens
	{
		FRegexPattern LinePattern(TEXT(":\\d+"));
		FRegexMatcher Matcher(LinePattern, Result);
		while (Matcher.FindNext())
		{
			Result.ReplaceInline(*Matcher.GetCaptureGroup(0), TEXT(""));
		}
	}

	// Strip lambda/generic markers: <...>
	{
		FRegexPattern LambdaPattern(TEXT("<[^>]*>"));
		FRegexMatcher Matcher(LambdaPattern, Result);
		while (Matcher.FindNext())
		{
			Result.ReplaceInline(*Matcher.GetCaptureGroup(0), TEXT(""));
		}
	}

	// Collapse whitespace
	while (Result.Contains(TEXT("  ")))
	{
		Result.ReplaceInline(TEXT("  "), TEXT(" "));
	}

	return Result.TrimStartAndEnd();
}

FString UHorizonCrashManager::HashSHA256(const FString& Input) const
{
	FSHA256Signature Signature;
	const FTCHARToUTF8 Utf8(*Input);
	FSHA256::HashBuffer(Utf8.Get(), Utf8.Length(), Signature.Hash);
	return Signature.ToString();
}

FString UHorizonCrashManager::GenerateFingerprint(const FString& StackTrace) const
{
	if (StackTrace.IsEmpty())
	{
		return HashSHA256(TEXT("no_stack_trace"));
	}

	TArray<FString> Lines;
	StackTrace.ParseIntoArrayLines(Lines);

	// Collect up to 5 normalized game-code frames
	static constexpr int32 MaxFrames = 5;
	TArray<FString> GameFrames;

	for (const FString& Line : Lines)
	{
		if (GameFrames.Num() >= MaxFrames)
		{
			break;
		}

		FString Trimmed = Line.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			continue;
		}

		if (IsEngineFrame(Trimmed))
		{
			continue;
		}

		FString Normalized = NormalizeFrame(Trimmed);
		if (!Normalized.IsEmpty())
		{
			GameFrames.Add(Normalized);
		}
	}

	// Fallback: if no game frames found, use all normalized frames
	if (GameFrames.IsEmpty())
	{
		for (const FString& Line : Lines)
		{
			FString Normalized = NormalizeFrame(Line);
			if (!Normalized.IsEmpty())
			{
				GameFrames.Add(Normalized);
				if (GameFrames.Num() >= MaxFrames)
				{
					break;
				}
			}
		}
	}

	if (GameFrames.IsEmpty())
	{
		return HashSHA256(TEXT("no_stack_trace"));
	}

	return HashSHA256(FString::Join(GameFrames, TEXT("|")));
}
```

---

### Task 8: Implement session registration and crash report submission

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp` (append)

**Step 1: Add session registration (fire-and-forget)**

```cpp
// ============================================================
// Session Registration
// ============================================================

void UHorizonCrashManager::RegisterSession()
{
	if (!HttpClient)
	{
		return;
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("sessionId"), SessionId);
	Body->SetStringField(TEXT("appVersion"), FApp::GetBuildVersion());
	Body->SetStringField(TEXT("platform"), CachedPlatform);
	Body->SetStringField(TEXT("userId"), ResolveUserId());

	TWeakObjectPtr<UHorizonCrashManager> WeakSelf(this);

	HttpClient->PostJson(Body, TEXT("api/v1/app/crash-reports/session"), false,
		FOnHttpResponse::CreateLambda([WeakSelf](const FHorizonNetworkResponse& Response)
		{
			UHorizonCrashManager* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			if (Response.bSuccess)
			{
				UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager -- Session registered: %s"), *Self->SessionId);
				Self->OnSessionRegistered.Broadcast();
			}
			else
			{
				UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager -- Session registration failed: %s"), *Response.ErrorMessage);
			}
		}));
}
```

**Step 2: Add crash report submission**

```cpp
// ============================================================
// Crash Report Submission
// ============================================================

void UHorizonCrashManager::SubmitReport(
	EHorizonCrashType Type,
	const FString& Message,
	const FString& StackTrace,
	const TMap<FString, FString>& ExtraKeys,
	FOnCrashReportComplete OnComplete)
{
	if (Message.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::SubmitReport -- Empty message, ignoring."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	if (!ConsumeRateLimitToken())
	{
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	if (!HttpClient)
	{
		UE_LOG(LogHorizonSDK, Error, TEXT("CrashManager::SubmitReport -- HttpClient is null."));
		OnCrashReportFailed.Broadcast(TEXT("HttpClient not initialized."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	// Build request body
	const FString Fingerprint = GenerateFingerprint(StackTrace);

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), CrashTypeToString(Type));
	Body->SetStringField(TEXT("message"), Message);
	Body->SetStringField(TEXT("fingerprint"), Fingerprint);
	Body->SetStringField(TEXT("appVersion"), FApp::GetBuildVersion());
	Body->SetStringField(TEXT("sdkVersion"), SdkVersion);
	Body->SetStringField(TEXT("platform"), CachedPlatform);
	Body->SetStringField(TEXT("os"), CachedOS);
	Body->SetStringField(TEXT("deviceModel"), CachedDeviceModel);
	Body->SetNumberField(TEXT("deviceMemoryMb"), CachedDeviceMemoryMb);
	Body->SetStringField(TEXT("sessionId"), SessionId);
	Body->SetStringField(TEXT("userId"), ResolveUserId());

	if (!StackTrace.IsEmpty())
	{
		Body->SetStringField(TEXT("stackTrace"), StackTrace);
	}

	// Breadcrumbs array
	TArray<TSharedPtr<FJsonValue>> BreadcrumbValues;
	for (const TSharedRef<FJsonObject>& Bc : CollectBreadcrumbsJson())
	{
		BreadcrumbValues.Add(MakeShared<FJsonValueObject>(Bc));
	}
	Body->SetArrayField(TEXT("breadcrumbs"), BreadcrumbValues);

	// Custom keys: merge persistent + extra (extra overrides)
	TSharedRef<FJsonObject> KeysObj = MakeShared<FJsonObject>();
	for (const auto& Pair : CustomKeys)
	{
		KeysObj->SetStringField(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : ExtraKeys)
	{
		KeysObj->SetStringField(Pair.Key, Pair.Value);
	}
	Body->SetObjectField(TEXT("customKeys"), KeysObj);

	TWeakObjectPtr<UHorizonCrashManager> WeakSelf(this);
	FOnCrashReportComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/crash-reports/create"), false,
		FOnHttpResponse::CreateLambda([WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
		{
			UHorizonCrashManager* Self = WeakSelf.Get();
			if (!Self)
			{
				return;
			}

			if (Response.bSuccess && Response.JsonData.IsValid())
			{
				const FString ReportId = Response.JsonData->GetStringField(TEXT("id"));
				const FString GroupId = Response.JsonData->GetStringField(TEXT("groupId"));

				UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager -- Report submitted: %s (group: %s)"), *ReportId, *GroupId);
				Self->OnCrashReported.Broadcast(ReportId, GroupId);
				CapturedOnComplete.ExecuteIfBound(true, ReportId, GroupId);
			}
			else
			{
				FString ErrorMsg = Response.ErrorMessage;
				if (Response.StatusCode == 403)
				{
					ErrorMsg = TEXT("Crash reporting not available for FREE accounts.");
				}

				UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager -- Report failed: %s"), *ErrorMsg);
				Self->OnCrashReportFailed.Broadcast(ErrorMsg);
				CapturedOnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
			}
		}));
}
```

---

### Task 9: Implement StartCapture, StopCapture, and automatic crash hooks

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCrashManager.cpp` (append)

**Step 1: Add FOutputDevice subclass implementation**

```cpp
// ============================================================
// FOutputDevice for UE_LOG capture
// ============================================================

UHorizonCrashManager::FHorizonCrashOutputDevice::FHorizonCrashOutputDevice(UHorizonCrashManager* InOwner)
	: Owner(InOwner)
{
}

void UHorizonCrashManager::FHorizonCrashOutputDevice::Serialize(
	const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	UHorizonCrashManager* Mgr = Owner.Get();
	if (!Mgr || !Mgr->bIsCapturing)
	{
		return;
	}

	// Only capture Error and Fatal log messages
	if (Verbosity == ELogVerbosity::Error || Verbosity == ELogVerbosity::Fatal)
	{
		// Avoid recursive capture: skip our own log category
		if (Category == TEXT("LogHorizonSDK"))
		{
			return;
		}

		Mgr->OnLogMessage(FString(V), Verbosity);
	}
}
```

**Step 2: Add OnLogMessage, OnEngineError, StartCapture, StopCapture**

```cpp
// ============================================================
// Crash Capture Handlers
// ============================================================

void UHorizonCrashManager::OnLogMessage(const FString& Message, ELogVerbosity::Type Verbosity)
{
	const EHorizonCrashType Type = (Verbosity == ELogVerbosity::Fatal)
		? EHorizonCrashType::Crash
		: EHorizonCrashType::NonFatal;

	SubmitReport(Type, Message, TEXT(""), {}, {});
}

void UHorizonCrashManager::OnEngineError()
{
	// Capture whatever context is available from the crash context
	const FString Message = TEXT("Engine system error");
	FString StackTrace;

	// Try to get stack trace from the current crash context
	TCHAR StackBuffer[4096];
	StackBuffer[0] = TEXT('\0');
	FPlatformStackWalk::StackWalkAndDump(StackBuffer, UE_ARRAY_COUNT(StackBuffer), 2);
	StackTrace = FString(StackBuffer);

	SubmitReport(EHorizonCrashType::Crash, Message, StackTrace, {}, {});
}

// ============================================================
// Lifecycle
// ============================================================

void UHorizonCrashManager::StartCapture()
{
	if (bIsCapturing)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::StartCapture -- Already capturing."));
		return;
	}

	bIsCapturing = true;
	SessionId = GenerateSessionId();
	SessionReportCount = 0;
	Tokens = static_cast<double>(TokensPerMinute);
	LastRefillTime = FPlatformTime::Seconds();

	// Register session (fire-and-forget)
	RegisterSession();

	// Hook FCoreDelegates::OnHandleSystemError
	SystemErrorHandle = FCoreDelegates::OnHandleSystemError.AddUObject(
		this, &UHorizonCrashManager::OnEngineError);

	// Register custom output device for UE_LOG capture
	OutputDevice = new FHorizonCrashOutputDevice(this);
	GLog->AddOutputDevice(OutputDevice);

	UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager -- Capture started (session: %s)"), *SessionId);
}

void UHorizonCrashManager::StopCapture()
{
	if (!bIsCapturing)
	{
		return;
	}

	bIsCapturing = false;

	// Unhook system error
	FCoreDelegates::OnHandleSystemError.Remove(SystemErrorHandle);
	SystemErrorHandle.Reset();

	// Unregister output device
	if (OutputDevice)
	{
		GLog->RemoveOutputDevice(OutputDevice);
		delete OutputDevice;
		OutputDevice = nullptr;
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager -- Capture stopped."));
}
```

**Step 3: Add public API wrappers**

```cpp
// ============================================================
// Public API
// ============================================================

void UHorizonCrashManager::RecordException(
	const FString& Message,
	const FString& StackTrace,
	const TMap<FString, FString>& ExtraKeys,
	FOnCrashReportComplete OnComplete)
{
	SubmitReport(EHorizonCrashType::NonFatal, Message, StackTrace, ExtraKeys, OnComplete);
}

void UHorizonCrashManager::ReportCrash(
	const FString& Message,
	const FString& StackTrace,
	FOnCrashReportComplete OnComplete)
{
	SubmitReport(EHorizonCrashType::Crash, Message, StackTrace, {}, OnComplete);
}
```

**Step 4: Add auto-breadcrumb handlers**

```cpp
// ============================================================
// Auto-Breadcrumb Handlers
// ============================================================

void UHorizonCrashManager::OnAutoUserSignedIn()
{
	AddBreadcrumb(TEXT("user"), TEXT("User signed in"));
}

void UHorizonCrashManager::OnAutoUserSignedOut()
{
	AddBreadcrumb(TEXT("user"), TEXT("User signed out"));
}

void UHorizonCrashManager::OnAutoConnected()
{
	AddBreadcrumb(TEXT("network"), TEXT("Connected to server"));
}

void UHorizonCrashManager::OnAutoConnectionFailed(const FString& ErrorMessage)
{
	AddBreadcrumb(TEXT("network"), FString::Printf(TEXT("Connection failed: %s"), *ErrorMessage));
}
```

**Step 5: Commit**

```
feat(crash): implement CrashManager with full crash reporting logic
```

---

## Phase 4: Subsystem Integration

### Task 10: Register CrashManager in HorizonSubsystem

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSubsystem.h:10-18` (forward declarations) and `:42-64` (properties)
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSubsystem.cpp:1-54`

**Step 1: Add forward declaration to HorizonSubsystem.h**

After line 18 (`class UHorizonUserLogManager;`), add:

```cpp
class UHorizonCrashManager;
```

**Step 2: Add UPROPERTY to HorizonSubsystem.h**

After the `UserLogs` property (after line 64), add:

```cpp
	UPROPERTY(BlueprintReadOnly, Category = "horizOn")
	UHorizonCrashManager* Crashes;
```

**Step 3: Add include to HorizonSubsystem.cpp**

After line 13 (`#include "Managers/HorizonUserLogManager.h"`), add:

```cpp
#include "Managers/HorizonCrashManager.h"
```

**Step 4: Add initialization in HorizonSubsystem.cpp**

After line 51 (`UserLogs->Initialize(HttpClient, Auth);`), add:

```cpp
	// Create crash manager (auth-dependent)
	Crashes = NewObject<UHorizonCrashManager>(this);
	Crashes->Initialize(HttpClient, Auth);

	// Wire auto-breadcrumbs from other managers
	Auth->OnUserSignedIn.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedIn);
	Auth->OnUserSignedOut.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedOut);
	OnConnected.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnected);
	OnConnectionFailed.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnectionFailed);
```

**Step 5: Add cleanup in Deinitialize()**

In `Deinitialize()`, before `if (IsConnected())`, add:

```cpp
	// Stop crash capture before shutting down
	if (Crashes && Crashes->IsCapturing())
	{
		Crashes->StopCapture();
	}
```

**Step 6: Commit**

```
feat(crash): register CrashManager in subsystem with auto-breadcrumbs
```

---

## Phase 5: Blueprint Integration

### Task 11: Create async actions for Blueprint

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/AsyncActions/HorizonAsync_Crash.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/AsyncActions/HorizonAsync_Crash.cpp`

**Step 1: Write the header**

```cpp
// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_Crash.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrashReportAsyncSuccess, const FString&, ReportId, const FString&, GroupId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrashReportAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Record Exception (NON_FATAL).
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CrashRecordException : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncFailure OnFailure;

	/** Record a non-fatal exception. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Record Exception"), Category = "horizOn|CrashReport")
	static UHorizonAsync_CrashRecordException* RecordException(const UObject* WorldContextObject, const FString& Message, const FString& StackTrace);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString ParamMessage;
	FString ParamStackTrace;

	void HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId);
};

/**
 * Async Blueprint node: Report Crash (CRASH).
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CrashReportCrash : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncFailure OnFailure;

	/** Report a fatal crash. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Report Crash"), Category = "horizOn|CrashReport")
	static UHorizonAsync_CrashReportCrash* ReportCrash(const UObject* WorldContextObject, const FString& Message, const FString& StackTrace);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString ParamMessage;
	FString ParamStackTrace;

	void HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId);
};
```

**Step 2: Write the implementation**

```cpp
// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "AsyncActions/HorizonAsync_Crash.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonCrashManager.h"

// ============================================================
// UHorizonAsync_CrashRecordException
// ============================================================

UHorizonAsync_CrashRecordException* UHorizonAsync_CrashRecordException::RecordException(
	const UObject* WorldContextObject, const FString& Message, const FString& StackTrace)
{
	UHorizonAsync_CrashRecordException* Action = NewObject<UHorizonAsync_CrashRecordException>();
	Action->WorldContext = WorldContextObject;
	Action->ParamMessage = Message;
	Action->ParamStackTrace = StackTrace;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CrashRecordException::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Crashes)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CrashManager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Crashes->RecordException(
		ParamMessage,
		ParamStackTrace,
		{},
		FOnCrashReportComplete::CreateUObject(this, &UHorizonAsync_CrashRecordException::HandleResult));
}

void UHorizonAsync_CrashRecordException::HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(ReportId, GroupId);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to submit crash report."));
	}
	SetReadyToDestroy();
}

// ============================================================
// UHorizonAsync_CrashReportCrash
// ============================================================

UHorizonAsync_CrashReportCrash* UHorizonAsync_CrashReportCrash::ReportCrash(
	const UObject* WorldContextObject, const FString& Message, const FString& StackTrace)
{
	UHorizonAsync_CrashReportCrash* Action = NewObject<UHorizonAsync_CrashReportCrash>();
	Action->WorldContext = WorldContextObject;
	Action->ParamMessage = Message;
	Action->ParamStackTrace = StackTrace;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UHorizonAsync_CrashReportCrash::Activate()
{
	UHorizonSubsystem* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Crashes)
	{
		OnFailure.Broadcast(TEXT("horizOn Subsystem or CrashManager not found."));
		SetReadyToDestroy();
		return;
	}

	Subsystem->Crashes->ReportCrash(
		ParamMessage,
		ParamStackTrace,
		FOnCrashReportComplete::CreateUObject(this, &UHorizonAsync_CrashReportCrash::HandleResult));
}

void UHorizonAsync_CrashReportCrash::HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(ReportId, GroupId);
	}
	else
	{
		OnFailure.Broadcast(TEXT("Failed to submit crash report."));
	}
	SetReadyToDestroy();
}
```

**Step 3: Commit**

```
feat(crash): add Blueprint async actions for crash reporting
```

---

### Task 12: Add Blueprint helper functions

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonBlueprintLibrary.h:23-38`
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonBlueprintLibrary.cpp`

**Step 1: Add function declarations to header**

After line 38 (`static FHorizonUserData GetHorizonCurrentUser(...)`), add:

```cpp
	/** Start crash capture and register a session. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonStartCrashCapture(const UObject* WorldContextObject);

	/** Stop crash capture. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonStopCrashCapture(const UObject* WorldContextObject);

	/** Record a breadcrumb with a type and message. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonRecordBreadcrumb(const UObject* WorldContextObject, const FString& Type, const FString& Message);

	/** Set a custom key-value pair for crash reports (max 10). */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport", meta = (WorldContext = "WorldContextObject"))
	static void HorizonSetCrashCustomKey(const UObject* WorldContextObject, const FString& Key, const FString& Value);
```

**Step 2: Add implementations to .cpp**

Add include at the top of `HorizonBlueprintLibrary.cpp`:

```cpp
#include "Managers/HorizonCrashManager.h"
```

Then add implementations at the end of the file:

```cpp
void UHorizonBlueprintLibrary::HorizonStartCrashCapture(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->StartCapture();
	}
}

void UHorizonBlueprintLibrary::HorizonStopCrashCapture(const UObject* WorldContextObject)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->StopCapture();
	}
}

void UHorizonBlueprintLibrary::HorizonRecordBreadcrumb(const UObject* WorldContextObject, const FString& Type, const FString& Message)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->RecordBreadcrumb(Type, Message);
	}
}

void UHorizonBlueprintLibrary::HorizonSetCrashCustomKey(const UObject* WorldContextObject, const FString& Key, const FString& Value)
{
	UHorizonSubsystem* Subsystem = GetHorizonSubsystem(WorldContextObject);
	if (Subsystem && Subsystem->Crashes)
	{
		Subsystem->Crashes->SetCustomKey(Key, Value);
	}
}
```

**Step 3: Commit**

```
feat(crash): add Blueprint helper functions for crash reporting
```

---

## Phase 6: Cleanup

### Task 13: Remove superseded plan file

**Files:**
- Delete: `docs/plans/2026-02-22-crash-reporting-plan.md`

The old incomplete plan is superseded by the design doc and this implementation plan.

**Step 1: Remove the old file**

```bash
git rm docs/plans/2026-02-22-crash-reporting-plan.md
```

**Step 2: Commit**

```
chore(crash): remove superseded crash reporting plan
```

---

## Summary

| Phase | Tasks | New Files | Modified Files |
|-------|-------|-----------|----------------|
| 1: Types | Task 1 | — | `HorizonTypes.h` |
| 2: Header | Task 2 | `HorizonCrashManager.h` | — |
| 3: Implementation | Tasks 3-9 | `HorizonCrashManager.cpp` | — |
| 4: Integration | Task 10 | — | `HorizonSubsystem.h`, `HorizonSubsystem.cpp` |
| 5: Blueprint | Tasks 11-12 | `HorizonAsync_Crash.h`, `HorizonAsync_Crash.cpp` | `HorizonBlueprintLibrary.h`, `HorizonBlueprintLibrary.cpp` |
| 6: Cleanup | Task 13 | — | Remove old plan |

**Total**: 4 new files, 5 modified files, 1 deleted file, 7 commits.
