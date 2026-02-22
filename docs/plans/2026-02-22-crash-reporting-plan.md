# Crash Reporting — Unreal SDK Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Date**: 2026-02-22
**Status**: Pending Implementation
**SDK Version**: 1.0.0 (will become 1.1.0 after this feature)
**Language**: C++ (Unreal Engine 5.5+)
**Related**: `ansible-horizon/docs/plans/2026-02-22-crash-reporting-sdk-logic.md` (universal spec)

---

## 1. Overview

Implement a `UHorizonCrashReportManager` for the horizOn Unreal SDK, following the same patterns as the existing 8 managers. The crash reporting system captures unhandled exceptions, non-fatal errors, and contextual data (breadcrumbs, device info, custom keys), then sends structured reports to the horizOn backend.

**Goal**: Feature parity with Unity and Godot SDK crash reporting.

**Tech Stack**: C++ with UE5 idioms, Blueprint integration via async actions.

---

## 2. File Structure

```
Plugins/HorizonSDK/Source/HorizonSDK/
├── Public/
│   ├── Managers/
│   │   └── HorizonCrashReportManager.h        (NEW)
│   └── AsyncActions/
│       └── HorizonAsync_CrashReport.h          (NEW)
└── Private/
    ├── Managers/
    │   └── HorizonCrashReportManager.cpp        (NEW)
    └── AsyncActions/
        └── HorizonAsync_CrashReport.cpp         (NEW)
```

**Modified files**:
- `Public/HorizonSubsystem.h` — Add CrashReport manager UPROPERTY
- `Private/HorizonSubsystem.cpp` — Create + initialize CrashReport manager
- `Public/HorizonTypes.h` — Add new enums and delegates

---

## Phase 1: Types & Enums

### Task 1: Add crash reporting types to HorizonTypes.h

**Step 1: Add CrashType enum**

```cpp
UENUM(BlueprintType)
enum class EHorizonCrashType : uint8
{
    Crash       UMETA(DisplayName = "Crash"),
    NonFatal    UMETA(DisplayName = "Non-Fatal"),
    Anr         UMETA(DisplayName = "ANR")
};
```

**Step 2: Add CrashReportManager delegates**

```cpp
// C++ delegate for crash report result
DECLARE_DELEGATE_ThreeParams(FOnCrashReportComplete,
    bool /*bSuccess*/, const FString& /*ReportId*/, const FString& /*GroupId*/);

// C++ delegate for session registration result
DECLARE_DELEGATE_OneParam(FOnCrashSessionComplete, bool /*bSuccess*/);
```

**Step 3: Commit**
```
feat(crash): add CrashType enum and crash reporting delegates
```

---

## Phase 2: CrashReportManager

### Task 2: Create HorizonCrashReportManager header

**File**: `Public/Managers/HorizonCrashReportManager.h`

**Class design**:

```cpp
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonCrashReportManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

    // --- Lifecycle ---
    void StartCapture();
    void StopCapture();

    // --- Manual Reporting ---
    void RecordException(
        const FString& Message,
        const FString& StackTrace = TEXT(""),
        const TMap<FString, FString>& ExtraKeys = {},
        FOnCrashReportComplete OnComplete = {});

    void ReportCrash(
        const FString& Message,
        const FString& StackTrace = TEXT(""),
        FOnCrashReportComplete OnComplete = {});

    // --- Context ---
    UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
    void RecordBreadcrumb(const FString& Type, const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
    void Log(const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
    void SetCustomKey(const FString& Key, const FString& Value);

    UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
    void SetUserId(const FString& UserId);

    // --- State ---
    UFUNCTION(BlueprintPure, Category = "horizOn|CrashReport")
    bool IsCapturing() const { return bIsCapturing; }

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

    // Device info (cached once)
    FString CachedPlatform;
    FString CachedOS;
    FString CachedDeviceModel;
    int32 CachedDeviceMemoryMb = 0;

    // Breadcrumb ring buffer
    static constexpr int32 MaxBreadcrumbs = 50;
    struct FBreadcrumb
    {
        FString Timestamp;
        FString Type;
        FString Message;
    };
    TArray<FBreadcrumb> Breadcrumbs;  // Pre-allocated to MaxBreadcrumbs
    int32 BreadcrumbHead = 0;
    int32 BreadcrumbCount = 0;

    // Custom keys
    static constexpr int32 MaxCustomKeys = 10;
    TMap<FString, FString> CustomKeys;

    // Rate limiting
    static constexpr int32 TokensPerMinute = 5;
    static constexpr int32 MaxSessionReports = 20;
    float Tokens = 5.0f;
    float LastRefillTime = 0.0f;
    int32 SessionReportCount = 0;

    // Internal methods
    void CacheDeviceInfo();
    FString GenerateSessionId();
    FString GenerateFingerprint(const FString& StackTrace);
    FString NormalizeFrame(const FString& Frame);
    FString HashSHA256(const FString& Input);
    void RefillTokens();
    bool CheckRateLimit();
    FString ResolveUserId();
    TArray<TSharedRef<FJsonObject>> GetBreadcrumbsJson();
    void AddBreadcrumb(const FString& Type, const FString& Message);

    void RegisterSession();
    void SubmitReport(
        EHorizonCrashType Type,
        const FString& Message,
        const FString& StackTrace,
        const TMap<FString, FString>& ExtraKeys,
        FOnCrashReportComplete OnComplete);

    // Crash handler delegates
    FDelegateHandle ErrorDelegateHandle;
    void OnEngineError();
};
```

**Commit**:
```
feat(crash): add CrashReportManager header with full API
```

---

### Task 3: Implement CrashReportManager core

**File**: `Private/Managers/HorizonCrashReportManager.cpp`

Implement in this order:

**Step 1: Initialize() + CacheDeviceInfo()**

```cpp
void UHorizonCrashReportManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
    HttpClient = InHttpClient;
    AuthManager = InAuthManager;
    CacheDeviceInfo();

    // Pre-allocate breadcrumb buffer
    Breadcrumbs.SetNum(MaxBreadcrumbs);

    // Initialize rate limiter
    Tokens = static_cast<float>(TokensPerMinute);
    LastRefillTime = FPlatformTime::Seconds();
}
```

Device info:
```cpp
CachedPlatform = FPlatformProperties::PlatformName();
CachedOS = FPlatformMisc::GetOSVersion();
CachedDeviceModel = FPlatformMisc::GetDeviceMakeAndModel();
CachedDeviceMemoryMb = static_cast<int32>(FPlatformMemory::GetConstants().TotalPhysical / (1024 * 1024));
```

**Step 2: Session ID generation**

```cpp
FString UHorizonCrashReportManager::GenerateSessionId()
{
    // 16 random bytes → 32-char hex
    TArray<uint8> Bytes;
    Bytes.SetNum(16);
    for (int32 i = 0; i < 16; ++i)
        Bytes[i] = static_cast<uint8>(FMath::RandRange(0, 255));

    return BytesToHex(Bytes.GetData(), Bytes.Num()).ToLower();
}
```

**Step 3: Fingerprint generation**

Follow the universal spec algorithm:
1. Split stack trace by newlines
2. Skip engine-internal frames (starting with `FGenericPlatformMisc`, `FWindowsPlatformStackWalk`, `UnrealEngine`, `CoreUObject`, `Core_`, `Engine_`)
3. Normalize each frame: strip addresses, line numbers, file paths
4. Take top 5 game-code frames
5. Join with `|`, SHA-256 hash

SHA-256 via UE5:
```cpp
FString UHorizonCrashReportManager::HashSHA256(const FString& Input)
{
    FSHAHash Hash;
    FSHA256 Sha256;
    Sha256.Update(reinterpret_cast<const uint8*>(TCHAR_TO_UTF8(*Input)),
                  Input.Len());
    Sha256.Final();
    // Convert to hex string
    ...
}
```

Or simpler using `FMD5` pattern but for SHA-256.

**Step 4: Breadcrumb ring buffer**

Same circular buffer algorithm as Unity/Godot. Use ISO 8601 timestamps:
```cpp
FDateTime::UtcNow().ToIso8601()
```

**Step 5: Rate limiting**

Token bucket with `FPlatformTime::Seconds()` for timing. Same logic as Unity (continuous refill).

**Step 6: Custom keys**

`TMap<FString, FString>` with max 10 check.

**Step 7: RegisterSession()**

```cpp
void UHorizonCrashReportManager::RegisterSession()
{
    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("sessionId"), SessionId);
    Body->SetStringField(TEXT("appVersion"), FApp::GetBuildVersion());
    Body->SetStringField(TEXT("platform"), CachedPlatform);
    Body->SetStringField(TEXT("userId"), ResolveUserId());

    HttpClient->PostJson(Body, TEXT("api/v1/app/crash-reports/session"), false,
        FOnHttpResponse::CreateLambda([](const FHorizonNetworkResponse& Response)
        {
            if (Response.bSuccess)
                UE_LOG(LogHorizonSDK, Log, TEXT("Crash session registered."));
            else
                UE_LOG(LogHorizonSDK, Warning, TEXT("Crash session registration failed: %s"), *Response.ErrorMessage);
        }));
}
```

**Step 8: SubmitReport()**

Build JSON body with all fields (type, message, stackTrace, fingerprint, device info, breadcrumbs as JSON array, customKeys as JSON object). POST to `api/v1/app/crash-reports/create`.

Parse response for `id` and `groupId`.

**Step 9: StartCapture() / StopCapture()**

Hook into UE5 crash handlers:
```cpp
void UHorizonCrashReportManager::StartCapture()
{
    if (bIsCapturing) return;
    bIsCapturing = true;

    SessionId = GenerateSessionId();
    SessionReportCount = 0;
    Tokens = static_cast<float>(TokensPerMinute);

    RegisterSession();

    // Hook engine error handler
    ErrorDelegateHandle = FCoreDelegates::OnHandleSystemError.AddUObject(
        this, &UHorizonCrashReportManager::OnEngineError);

    UE_LOG(LogHorizonSDK, Log, TEXT("Crash capture started (session: %s)"), *SessionId);
}

void UHorizonCrashReportManager::StopCapture()
{
    if (!bIsCapturing) return;
    bIsCapturing = false;

    FCoreDelegates::OnHandleSystemError.Remove(ErrorDelegateHandle);

    UE_LOG(LogHorizonSDK, Log, TEXT("Crash capture stopped."));
}
```

**Step 10: Public API wrappers**

```cpp
void UHorizonCrashReportManager::RecordException(
    const FString& Message, const FString& StackTrace,
    const TMap<FString, FString>& ExtraKeys, FOnCrashReportComplete OnComplete)
{
    SubmitReport(EHorizonCrashType::NonFatal, Message, StackTrace, ExtraKeys, OnComplete);
}

void UHorizonCrashReportManager::ReportCrash(
    const FString& Message, const FString& StackTrace,
    FOnCrashReportComplete OnComplete)
{
    SubmitReport(EHorizonCrashType::Crash, Message, StackTrace, {}, OnComplete);
}
```

**Commit**:
```
feat(crash): implement CrashReportManager core logic
```

---

### Task 4: Register manager in HorizonSubsystem

**Modify**: `Public/HorizonSubsystem.h`

Add:
```cpp
UPROPERTY(BlueprintReadOnly, Category = "horizOn")
UHorizonCrashReportManager* CrashReport = nullptr;
```

Add forward declaration:
```cpp
class UHorizonCrashReportManager;
```

**Modify**: `Private/HorizonSubsystem.cpp`

In `Initialize()`, after UserLogs creation:
```cpp
CrashReport = NewObject<UHorizonCrashReportManager>(this);
CrashReport->Initialize(HttpClient, Auth);
```

Add include:
```cpp
#include "Managers/HorizonCrashReportManager.h"
```

**Commit**:
```
feat(crash): register CrashReportManager in subsystem
```

---

## Phase 3: Blueprint Integration

### Task 5: Create async action for Blueprint

**File**: `Public/AsyncActions/HorizonAsync_CrashReport.h`

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnCrashReportAsyncSuccess, const FString&, ReportId, const FString&, GroupId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnCrashReportAsyncFailure, const FString&, ErrorMessage);

UCLASS()
class HORIZONSDK_API UHorizonAsync_CrashReport : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnCrashReportAsyncSuccess OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnCrashReportAsyncFailure OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true",
        WorldContext = "WorldContextObject",
        DisplayName = "Record Exception"),
        Category = "horizOn|CrashReport")
    static UHorizonAsync_CrashReport* RecordException(
        const UObject* WorldContextObject,
        const FString& Message,
        const FString& StackTrace);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true",
        WorldContext = "WorldContextObject",
        DisplayName = "Report Crash"),
        Category = "horizOn|CrashReport")
    static UHorizonAsync_CrashReport* ReportCrash(
        const UObject* WorldContextObject,
        const FString& Message,
        const FString& StackTrace);

    virtual void Activate() override;

private:
    enum class ECrashOp { RecordException, ReportCrash };

    TWeakObjectPtr<const UObject> WorldContext;
    ECrashOp Operation;
    FString ParamMessage;
    FString ParamStackTrace;

    void HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId);
};
```

**Implementation**: Follow exact same pattern as `HorizonAsync_Feedback`:
- Factory methods create NewObject, capture params, RegisterWithGameInstance
- `Activate()` gets subsystem, calls manager method with CreateUObject callback
- `HandleResult()` broadcasts OnSuccess/OnFailure, calls SetReadyToDestroy()

**Commit**:
```
feat(crash): add Blueprint async actions for crash reporting
```

---

## Phase 4: Blueprint Helpers

### Task 6: Add Blueprint convenience functions

**Modify**: `Public/HorizonBlueprintLibrary.h`

Add:
```cpp
UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport",
    meta = (WorldContext = "WorldContextObject"))
static void HorizonRecordBreadcrumb(
    const UObject* WorldContextObject,
    const FString& Type,
    const FString& Message);

UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport",
    meta = (WorldContext = "WorldContextObject"))
static void HorizonSetCrashCustomKey(
    const UObject* WorldContextObject,
    const FString& Key,
    const FString& Value);

UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport",
    meta = (WorldContext = "WorldContextObject"))
static void HorizonStartCrashCapture(const UObject* WorldContextObject);

UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport",
    meta = (WorldContext = "WorldContextObject"))
static void HorizonStopCrashCapture(const UObject* WorldContextObject);
```

Each function gets subsystem → calls manager method.

**Commit**:
```
feat(crash): add Blueprint helper functions for crash reporting
```

---

## Phase 5: Testing & Verification

### Task 7: Manual testing

1. Verify crash report submission from C++ code
2. Verify crash report submission from Blueprint async action
3. Verify session registration fires on `StartCapture()`
4. Verify breadcrumb recording and ring buffer behavior
5. Verify rate limiting (submit > 5 reports quickly, confirm drops)
6. Verify custom keys (set 10 keys, try to set 11th)
7. Verify fingerprint consistency (same crash → same fingerprint)
8. Verify `FCoreDelegates::OnHandleSystemError` hook captures crashes

### Task 8: Update documentation

- Update `README.md` with crash reporting section
- Update `CHANGELOG.md` with crash reporting entry
- Add crash reporting example to `HorizonExampleWidget`

**Commit**:
```
docs(crash): add crash reporting documentation and examples
```

---

## 3. Constants Reference

| Constant | Value | Purpose |
|----------|-------|---------|
| `MaxBreadcrumbs` | 50 | Ring buffer size |
| `MaxCustomKeys` | 10 | Max key-value pairs |
| `TokensPerMinute` | 5 | Rate limit burst |
| `MaxSessionReports` | 20 | Per-session hard cap |
| `SdkVersion` | `"unreal-1.0.0"` | Sent with reports |

---

## 4. API Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `api/v1/app/crash-reports/session` | POST | Register crash session |
| `api/v1/app/crash-reports/create` | POST | Submit crash report |

---

## 5. Not In Scope (Future)

- **Offline persistence**: Queue failed reports to disk for retry on next launch
- **ANR detection**: Monitor main thread responsiveness
- **Auto-breadcrumbs**: Wire into other manager events automatically
- **Symbolication**: Parse .pdb files for readable stack traces
- **Minidump capture**: Capture UE5 minidumps for detailed analysis
