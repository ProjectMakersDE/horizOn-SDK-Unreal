# Crash Reporting — Unreal SDK Design

**Date**: 2026-02-23
**Status**: Approved
**Supersedes**: `2026-02-22-crash-reporting-plan.md` (incomplete draft)
**Related**: `ansible-horizon/docs/plans/2026-02-22-crash-reporting-sdk-logic.md` (universal spec)

---

## 1. Overview

Add a `UHorizonCrashManager` to the horizOn Unreal SDK. The manager captures fatal crashes, non-fatal errors, and contextual data (breadcrumbs, device info, custom keys), then sends structured reports to the horizOn backend. Feature parity with Unity and Godot SDK crash reporting.

---

## 2. Architecture Decisions

### Auth-Dependent Manager

CrashManager receives both `HttpClient` and `AuthManager`. This enables automatic user ID resolution from the signed-in user, with `SetUserId()` as override. Same pattern as Leaderboard, CloudSave, etc.

### Fully Automatic Crash Capture

Two capture hooks:

1. **Fatal crashes**: `FCoreDelegates::OnHandleSystemError` — fires on engine-level system errors
2. **UE_LOG errors**: Custom `FOutputDevice` subclass registered with `GLog` — captures `ELogVerbosity::Error` and `Fatal` log messages as NON_FATAL reports

Manual reporting via `RecordException()` and `ReportCrash()` always available.

### Auto-Breadcrumbs from Existing Managers (Tier 1)

CrashManager subscribes to dynamic multicast delegates on other managers:

| Manager Event | Breadcrumb Type | Message Pattern |
|--------------|----------------|-----------------|
| `Auth::OnUserSignedIn` | `"user"` | `"User signed in"` |
| `Auth::OnUserSignedOut` | `"user"` | `"User signed out"` |
| `Subsystem::OnConnected` | `"network"` | `"Connected to server"` |
| `Subsystem::OnConnectionFailed` | `"network"` | `"Connection failed: {error}"` |

The subsystem wires these up after all managers are initialized.

---

## 3. File Structure

### New Files

```
Plugins/HorizonSDK/Source/HorizonSDK/
  Public/
    Managers/HorizonCrashManager.h
    AsyncActions/HorizonAsync_Crash.h
  Private/
    Managers/HorizonCrashManager.cpp
    AsyncActions/HorizonAsync_Crash.cpp
```

### Modified Files

- `Public/HorizonTypes.h` — Add `EHorizonCrashType` enum and crash delegates
- `Public/HorizonSubsystem.h` — Add `Crashes` UPROPERTY + forward declaration
- `Private/HorizonSubsystem.cpp` — Create + initialize CrashManager, wire auto-breadcrumbs
- `Public/HorizonBlueprintLibrary.h` — Add crash helper functions
- `Private/HorizonBlueprintLibrary.cpp` — Implement crash helper functions

---

## 4. Crash Types

```cpp
UENUM(BlueprintType)
enum class EHorizonCrashType : uint8
{
    Crash       UMETA(DisplayName = "Crash"),
    NonFatal    UMETA(DisplayName = "Non-Fatal"),
    Anr         UMETA(DisplayName = "ANR")
};
```

---

## 5. CrashManager API

### Public Methods

| Method | Purpose |
|--------|---------|
| `Initialize(HttpClient, AuthManager)` | Set up dependencies, cache device info, pre-allocate buffers |
| `StartCapture()` | Generate session ID, register session, hook crash handlers |
| `StopCapture()` | Unhook crash handlers |
| `RecordException(Message, StackTrace, ExtraKeys, OnComplete)` | Submit NON_FATAL report |
| `ReportCrash(Message, StackTrace, OnComplete)` | Submit CRASH report |
| `RecordBreadcrumb(Type, Message)` | Add typed breadcrumb to ring buffer |
| `Log(Message)` | Shorthand for `"log"` type breadcrumb |
| `SetCustomKey(Key, Value)` | Set persistent key-value pair (max 10) |
| `SetUserId(UserId)` | Override user ID for crash attribution |
| `IsCapturing()` | Query capture state |

### Events (Dynamic Multicast)

```cpp
FOnHorizonCrashReported         // (ReportId, GroupId) — successful submission
FOnHorizonCrashReportFailed     // (ErrorMessage) — failed or rate limited
FOnHorizonCrashSessionRegistered // () — session registered
FOnHorizonBreadcrumbRecorded    // (Message) — breadcrumb added
```

### C++ Delegates

```cpp
DECLARE_DELEGATE_ThreeParams(FOnCrashReportComplete,
    bool, const FString&, const FString&);   // bSuccess, ReportId, GroupId
DECLARE_DELEGATE_OneParam(FOnCrashSessionComplete, bool);  // bSuccess
```

---

## 6. Breadcrumb Ring Buffer

- Pre-allocated `TArray<FBreadcrumb>` with 50 slots
- Circular write via `BreadcrumbHead` index
- `BreadcrumbCount` tracks total ever added (unbounded)
- Retrieval outputs chronological order (oldest first), handling wrap-around
- Timestamps: `FDateTime::UtcNow().ToIso8601()`

### Breadcrumb Struct

Internal struct (not USTRUCT — only serialized to JSON for reports):

```cpp
struct FBreadcrumb
{
    FString Timestamp;
    FString Type;
    FString Message;
};
```

---

## 7. Fingerprint Generation

1. Split stack trace by newlines
2. Filter out engine-internal frames: `FGenericPlatformMisc`, `FWindowsPlatformStackWalk`, `UnrealEngine`, `CoreUObject`, `Core_`, `Engine_`
3. Normalize each frame: strip file paths, memory addresses (`0x...`), line numbers (`:42`), lambda markers (`<...>`), collapse whitespace
4. Take top 5 game-code frames
5. Concatenate with `|` separator
6. SHA-256 hash → 64-char hex string

Edge cases:
- Empty stack trace → hash `"no_stack_trace"`
- Fewer than 5 game frames → use all available
- No game frames → hash full normalized trace as fallback

SHA-256 via UE5's `FSHA256` class.

---

## 8. Rate Limiting (Token Bucket)

| Constant | Value |
|----------|-------|
| `TokensPerMinute` | 5 |
| `MaxSessionReports` | 20 |
| `RefillInterval` | 60 seconds |

Discrete refill: every 60 seconds elapsed, add up to 5 tokens (capped at 5). Session hard cap at 20 total reports. Dropped reports log a warning and emit `OnCrashReportFailed`.

Timing via `FPlatformTime::Seconds()`.

---

## 9. Custom Keys

- `TMap<FString, FString>`, max 10 entries
- `SetCustomKey(Key, Value)` — insert or update; reject new keys if at limit
- Merge at report time: persistent keys + per-report extra keys (extra overrides persistent)

---

## 10. User ID Resolution

Priority chain:
1. Explicit override via `SetUserId()`
2. `AuthManager->GetCurrentUser().UserId` if signed in
3. Empty string fallback

---

## 11. Device Info (Cached at Init)

| Field | Unreal Source |
|-------|--------------|
| `platform` | `FPlatformProperties::PlatformName()` |
| `os` | `FPlatformMisc::GetOSVersion()` |
| `deviceModel` | `FPlatformMisc::GetDeviceMakeAndModel()` |
| `deviceMemoryMb` | `FPlatformMemory::GetConstants().TotalPhysical / 1MB` |

---

## 12. Session Tracking

- Session ID: 32-char hex (16 random bytes via `FMath::RandRange`)
- Registration: `POST /api/v1/app/crash-reports/session` — fire-and-forget
- New session on each `StartCapture()` call

---

## 13. API Endpoints

### Session Registration

```
POST /api/v1/app/crash-reports/session
Body: { sessionId, appVersion, platform, userId }
Response 201: { id, createdAt }
```

### Crash Report Submission

```
POST /api/v1/app/crash-reports/create
Body: { type, message, stackTrace, fingerprint, appVersion, sdkVersion,
        platform, os, deviceModel, deviceMemoryMb, sessionId, userId,
        breadcrumbs[], customKeys{} }
Response 201: { id, groupId, createdAt }
```

### Error Handling

| Status | SDK Action |
|--------|------------|
| 201 | Emit success event |
| 400 | Log error, emit failure |
| 403 | Log "not available for FREE tier", emit failure |
| 429 | Log warning, emit failure (no retry) |
| 5xx | HttpClient handles retry automatically |

---

## 14. Blueprint Integration

### Async Actions

- `UHorizonAsync_CrashRecordException` — Record Exception node
- `UHorizonAsync_CrashReportCrash` — Report Crash node

Both follow the standard factory pattern with `OnSuccess`/`OnFailure` output pins.

### Blueprint Library Helpers

- `HorizonStartCrashCapture(WorldContext)`
- `HorizonStopCrashCapture(WorldContext)`
- `HorizonRecordBreadcrumb(WorldContext, Type, Message)`
- `HorizonSetCrashCustomKey(WorldContext, Key, Value)`

---

## 15. Automatic Crash Capture (UE_LOG)

A lightweight `FOutputDevice` subclass (`FHorizonCrashOutputDevice`) registered with `GLog`:

- Captures `ELogVerbosity::Error` → NON_FATAL report
- Captures `ELogVerbosity::Fatal` → CRASH report
- Ignores other log levels
- Uses the log message as crash message; no stack trace from log capture (UE doesn't provide one via FOutputDevice)
- Owned by CrashManager, created in `StartCapture()`, destroyed in `StopCapture()`

This is separate from `FCoreDelegates::OnHandleSystemError` which handles actual engine crashes.

---

## 16. Subsystem Integration

```cpp
// In UHorizonSubsystem::Initialize():
Crashes = NewObject<UHorizonCrashManager>(this);
Crashes->Initialize(HttpClient, Auth);

// Wire auto-breadcrumbs after all managers exist:
Auth->OnUserSignedIn.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedIn);
Auth->OnUserSignedOut.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoUserSignedOut);
OnConnected.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnected);
OnConnectionFailed.AddUniqueDynamic(Crashes, &UHorizonCrashManager::OnAutoConnectionFailed);
```

The CrashManager exposes `UFUNCTION()` handlers for these auto-breadcrumb events.

---

## 17. Not In Scope (Future)

- **Offline persistence**: Queue failed reports to disk for retry on next launch
- **ANR detection**: Monitor main thread responsiveness
- **Symbolication**: Parse .pdb files for readable stack traces
- **Minidump capture**: Capture UE5 minidumps for detailed analysis
- **Auto-breadcrumbs from CloudSave/Leaderboard/etc.**: Currently only Auth + Connection events; can extend later
