<p align="center">
  <a href="https://horizon.pm">
    <img src="https://horizon.pm/media/images/og-image.png" alt="horizOn - Game Backend & Live-Ops Dashboard" />
  </a>
</p>

# horizOn SDK for Unreal Engine

[![Unreal Engine 5.5+](https://img.shields.io/badge/Unreal_Engine-5.5%2B-blue?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.1.0-orange)](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/releases)

Official Unreal Engine SDK for **horizOn** Backend-as-a-Service by [ProjectMakers](https://projectmakers.de).

## Features

| Feature | Manager | Description |
|---------|---------|-------------|
| 🔐 **Authentication** | `UHorizonAuthManager` | Anonymous, email, and Google sign-up / sign-in with session caching |
| 🏆 **Leaderboards** | `UHorizonLeaderboardManager` | Submit scores, retrieve top entries, rank, and surrounding entries |
| ☁️ **Cloud Save** | `UHorizonCloudSaveManager` | Save and load player data in JSON or binary format |
| ⚙️ **Remote Config** | `UHorizonRemoteConfigManager` | Typed key-value retrieval (string, int, float, bool) with caching |
| 📰 **News** | `UHorizonNewsManager` | In-game news feed with language filtering and TTL cache |
| 🎁 **Gift Codes** | `UHorizonGiftCodeManager` | Validate and redeem promotional codes |
| 💬 **Feedback** | `UHorizonFeedbackManager` | Submit bug reports, feature requests, and general feedback |
| 📊 **User Logs** | `UHorizonUserLogManager` | Server-side structured logging for analytics and debugging |
| 💥 **Crash Reporting** | `UHorizonCrashReportManager` | Crash capture, exception tracking, breadcrumbs |

## Requirements

- Unreal Engine **5.5** or later
- A horizOn project with an API key (obtain from the [horizOn dashboard](https://horizon.pm))

## Installation

1. Download the latest release ZIP from [Releases](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/releases).
2. Extract `HorizonSDK/` into your project's `Plugins/` directory.
3. Open your project in Unreal Editor.
4. Go to **Edit > Plugins**, search for "horizOn SDK", and enable it.
5. Restart the editor when prompted.

## Quick Start

> **[Quickstart Guide on horizon.pm](https://horizon.pm/quickstart#unreal)** - Interactive setup guide with step-by-step instructions.

### C++

```cpp
#include "HorizonSubsystem.h"

// Get the subsystem from any Actor or UObject with world context
UHorizonSubsystem* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();

// Connect to the backend
Horizon->ConnectToServer();

// Listen for connection result
Horizon->OnConnected.AddDynamic(this, &AMyActor::OnConnected);

// Sign up anonymously
Horizon->Auth->SignUpAnonymous(TEXT("Player1"),
    FOnAuthComplete::CreateLambda([](bool bSuccess)
    {
        // Handle result
    }));
```

### Blueprints

1. Use the **"horizOn Connect"** async node to connect to the server.
2. Use **"horizOn Sign Up Anonymous"** or **"horizOn Sign In Email"** to authenticate.
3. Call any feature node (Submit Score, Save Data, Load News, etc.).

All async nodes expose **On Success** and **On Failure** execution pins for easy error handling.

## API Reference

### Connection

```cpp
UHorizonSubsystem* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();

// Connect to server
Horizon->ConnectToServer();

// Check status
Horizon->IsConnected();    // Returns true if connected
Horizon->GetActiveHost();  // Returns current server URL

// Disconnect
Horizon->DisconnectFromServer();
```

### Authentication

```cpp
// Anonymous sign-up
Horizon->Auth->SignUpAnonymous(TEXT("PlayerName"),
    FOnAuthComplete::CreateLambda([](bool bSuccess) { }));

// Email sign-up
Horizon->Auth->SignUpEmail(TEXT("user@example.com"), TEXT("password"), TEXT("DisplayName"),
    FOnAuthComplete::CreateLambda([](bool bSuccess) { }));

// Email sign-in
Horizon->Auth->SignInEmail(TEXT("user@example.com"), TEXT("password"),
    FOnAuthComplete::CreateLambda([](bool bSuccess) { }));

// Check authentication state
if (Horizon->Auth->IsSignedIn())
{
    FHorizonUser User = Horizon->Auth->GetCurrentUser();
    UE_LOG(LogTemp, Log, TEXT("Welcome, %s!"), *User.DisplayName);
}

// Sign out
Horizon->Auth->SignOut();
```

### Leaderboards

```cpp
// Submit score
Horizon->Leaderboard->SubmitScore(12500,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Get top players
Horizon->Leaderboard->GetTop(10,
    FOnLeaderboardComplete::CreateLambda([](bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries) { }));

// Get your rank
Horizon->Leaderboard->GetRank(
    FOnLeaderboardEntryComplete::CreateLambda([](bool bSuccess, const FHorizonLeaderboardEntry& Entry) { }));

// Get players around your rank
Horizon->Leaderboard->GetAround(5,
    FOnLeaderboardComplete::CreateLambda([](bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries) { }));
```

### Cloud Saves

```cpp
// Save JSON object
FString SaveData = TEXT("{\"level\": 5, \"coins\": 1000}");
Horizon->CloudSave->SaveData(SaveData,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Load JSON object
Horizon->CloudSave->LoadData(
    FOnCloudSaveComplete::CreateLambda([](bool bSuccess, const FString& Data) { }));

// Save binary data
TArray<uint8> Bytes = /* your data */;
Horizon->CloudSave->SaveBytes(Bytes,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Load binary data
Horizon->CloudSave->LoadBytes(
    FOnCloudSaveBytesComplete::CreateLambda([](bool bSuccess, const TArray<uint8>& Data) { }));
```

### Remote Config

```cpp
// Get typed values with defaults
Horizon->RemoteConfig->GetConfig(TEXT("game_version"),
    FOnStringComplete::CreateLambda([](bool bSuccess, const FString& Value) { }));

Horizon->RemoteConfig->GetInt(TEXT("max_level"), 100,
    FOnIntComplete::CreateLambda([](bool bSuccess, int32 Value) { }));

Horizon->RemoteConfig->GetFloat(TEXT("difficulty"), 1.0f,
    FOnFloatComplete::CreateLambda([](bool bSuccess, float Value) { }));

Horizon->RemoteConfig->GetBool(TEXT("maintenance_mode"), false,
    FOnBoolComplete::CreateLambda([](bool bSuccess, bool bValue) { }));

// Get all configs
Horizon->RemoteConfig->GetAllConfigs(
    FOnConfigMapComplete::CreateLambda([](bool bSuccess, const TMap<FString, FString>& Configs) { }));
```

### News

```cpp
Horizon->News->LoadNews(20, TEXT("en"),
    FOnNewsComplete::CreateLambda([](bool bSuccess, const TArray<FHorizonNewsEntry>& Entries)
    {
        for (const auto& Entry : Entries)
        {
            UE_LOG(LogTemp, Log, TEXT("%s: %s"), *Entry.Title, *Entry.Message);
        }
    }));
```

### Gift Codes

```cpp
// Validate
Horizon->GiftCodes->Validate(TEXT("ABCD-1234"),
    FOnBoolComplete::CreateLambda([](bool bSuccess, bool bValid) { }));

// Redeem
Horizon->GiftCodes->Redeem(TEXT("ABCD-1234"),
    FOnGiftCodeComplete::CreateLambda([](bool bSuccess, const FHorizonGiftCodeResult& Result)
    {
        if (bSuccess && Result.bSuccess)
        {
            // Parse Result.GiftData for rewards
        }
    }));
```

### Feedback

```cpp
// Bug report
Horizon->Feedback->ReportBug(TEXT("Crash on level 5"), TEXT("Game crashes when opening inventory"),
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Feature request
Horizon->Feedback->RequestFeature(TEXT("Dark mode"), TEXT("Please add dark mode option"),
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// General feedback with email and device info
Horizon->Feedback->Submit(TEXT("Title"), TEXT("Message"), TEXT("GENERAL"),
    TEXT("email@example.com"), true,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));
```

### User Logs

```cpp
Horizon->UserLogs->Info(TEXT("Tutorial completed"));
Horizon->UserLogs->Warn(TEXT("Low memory detected"));
Horizon->UserLogs->Error(TEXT("Failed to load asset"), TEXT("ERR_001"));
```

### Crash Reporting

Track crashes, non-fatal exceptions, and breadcrumbs to monitor game stability. The `UHorizonCrashReportManager` can automatically capture engine-level crashes when capture is active.

#### C++

```cpp
// Start automatic crash capture (call once on game start)
// Hooks into FCoreDelegates::OnHandleSystemError
Horizon->Crashes->StartCapture();

// Record breadcrumbs for context leading up to issues
Horizon->Crashes->RecordBreadcrumb(TEXT("navigation"), TEXT("Entered level 5"));
Horizon->Crashes->RecordBreadcrumb(TEXT("user_action"), TEXT("Opened inventory"));
Horizon->Crashes->Log(TEXT("Player picked up item"));

// Set custom metadata included in all reports
Horizon->Crashes->SetCustomKey(TEXT("level"), TEXT("5"));
Horizon->Crashes->SetCustomKey(TEXT("build"), TEXT("1.2.3"));

// Override user ID (defaults to authenticated user)
Horizon->Crashes->SetUserId(UserId);

// Manually record a non-fatal exception
Horizon->Crashes->RecordException(TEXT("Failed to load texture"), TEXT("stack trace here"),
    TMap<FString, FString>(),
    FOnCrashReportComplete::CreateLambda([](bool bSuccess, const FString& ReportId, const FString& GroupId)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Exception recorded: %s"), *ReportId);
        }
    }));

// Report a fatal crash
Horizon->Crashes->ReportCrash(TEXT("Unexpected null reference"), TEXT("stack trace"),
    FOnCrashReportComplete::CreateLambda([](bool bSuccess, const FString& ReportId, const FString& GroupId) { }));

// Stop capture when done
Horizon->Crashes->StopCapture();

// Check capture state
bool bCapturing = Horizon->Crashes->IsCapturing();
```

#### Blueprints

Use the async nodes for Blueprint integration:

- **"horizOn Record Exception"** - Reports a non-fatal exception
- **"horizOn Report Crash"** - Reports a fatal crash
- **"horizOn Record Breadcrumb"** - Adds context breadcrumb
- **"horizOn Start Crash Capture"** - Begins automatic capture
- **"horizOn Stop Crash Capture"** - Stops automatic capture
- **"horizOn Set Crash Custom Key"** - Sets report metadata

#### Limits

| Parameter | Limit |
|-----------|-------|
| Reports per minute | 5 |
| Reports per session | 20 |
| Breadcrumbs (ring buffer) | 50 |
| Custom keys | 10 |

## Events / Delegates

### C++ Delegates

```cpp
// Connection
Horizon->OnConnected.AddDynamic(this, &AMyActor::OnConnected);
Horizon->OnDisconnected.AddDynamic(this, &AMyActor::OnDisconnected);

// Authentication
Horizon->Auth->OnSignInComplete.AddDynamic(this, &AMyActor::OnSignedIn);
Horizon->Auth->OnSignInFailed.AddDynamic(this, &AMyActor::OnSignInFailed);

// Leaderboard
Horizon->Leaderboard->OnScoreSubmitted.AddDynamic(this, &AMyActor::OnScoreSubmitted);

// Cloud Save
Horizon->CloudSave->OnDataSaved.AddDynamic(this, &AMyActor::OnDataSaved);
Horizon->CloudSave->OnDataLoaded.AddDynamic(this, &AMyActor::OnDataLoaded);

// Crash Reporting
Horizon->Crashes->OnCrashReported.AddDynamic(this, &AMyActor::OnCrashReported);
Horizon->Crashes->OnCrashReportFailed.AddDynamic(this, &AMyActor::OnCrashReportFailed);
Horizon->Crashes->OnSessionRegistered.AddDynamic(this, &AMyActor::OnSessionRegistered);
```

### Blueprint Events

All async nodes expose **On Success** and **On Failure** execution pins. For event-driven patterns, use the **Event Dispatchers** exposed on each manager component.

## Configuration Options

Open **Project Settings > Plugins > horizOn SDK** to configure:

| Option | Default | Description |
|--------|---------|-------------|
| API Key | - | Your horizOn API key |
| Backend Hosts | `["https://horizon.pm"]` | Backend server URL(s). Single host skips ping; multiple hosts use latency-based selection. |
| Connection Timeout | 10 | HTTP request timeout in seconds |
| Max Retries | 3 | Retry count for failed requests |
| Retry Delay | 1.0 | Delay between retries in seconds |
| Log Level | INFO | DEBUG, INFO, WARNING, ERROR, NONE |

Alternatively, use **Tools > horizOn > Import Config...** to import a JSON config file from the horizOn dashboard.

## Rate Limiting

**Limit**: 10 requests per minute per client. Requests exceeding this limit receive an HTTP 429 response and are automatically retried after the cooldown period.

| Do | Don't |
|----|-------|
| Load all configs at startup | Fetch configs repeatedly |
| Cache leaderboard data | Refresh every frame |
| Save on level complete | Save on every action |
| Submit scores on improvement | Submit every score |
| Start crash capture once | Start/stop capture repeatedly |

## Error Handling

```cpp
// Use completion callbacks to handle errors
Horizon->Leaderboard->SubmitScore(1000,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error)
    {
        if (!bSuccess)
        {
            UE_LOG(LogTemp, Error, TEXT("Submit failed: %s"), *Error);
        }
    }));

// Auth with error handling
Horizon->Auth->SignInEmail(TEXT("user@example.com"), TEXT("password"),
    FOnAuthComplete::CreateLambda([](bool bSuccess)
    {
        if (!bSuccess)
        {
            UE_LOG(LogTemp, Error, TEXT("Sign-in failed"));
        }
    }));
```

### Common HTTP Status Codes

| Code | Meaning | Action |
|------|---------|--------|
| 400 | Bad Request | Check parameters |
| 401 | Unauthorized | Re-authenticate |
| 403 | Forbidden | Check tier/permissions |
| 429 | Rate Limited | Wait and retry (automatic) |

## Self-Hosted Option

The horizOn SDKs work with both the **managed horizOn BaaS** and the **free, open-source [horizOn Simple Server](https://github.com/ProjectMakersDE/horizOn-simpleServer)**.

Simple Server is a lightweight PHP backend with no dependencies — perfect as a starting point if you want full control over your infrastructure. It supports core features like leaderboards, cloud saves, remote config, news, gift codes, feedback, and crash reporting.

To connect to your own server, set the **Backend Hosts** in Project Settings > Plugins > horizOn SDK to your server URL.

> **Note:** Simple Server is a starting point, not a full replacement. For the complete experience with dashboard, user authentication, multi-region deployment, and more, use [horizOn BaaS](https://horizon.pm).

## Project Structure

```
Plugins/HorizonSDK/
├── HorizonSDK.uplugin
├── Source/
│   ├── HorizonSDK/              (Runtime module)
│   │   ├── Public/
│   │   │   ├── Http/            HTTP client
│   │   │   ├── Models/          Data structs
│   │   │   ├── Managers/        Feature managers (incl. CrashReportManager)
│   │   │   ├── AsyncActions/    Blueprint async nodes
│   │   │   └── Example/         Example widget
│   │   └── Private/
│   └── HorizonSDKEditor/        (Editor module)
│       ├── Public/
│       └── Private/
├── Docs/
└── Config/
```

## Documentation

- **[Quickstart Guide](https://horizon.pm/quickstart#unreal)** - Interactive setup
- **[Plugin README](Plugins/HorizonSDK/README.md)** - Plugin-specific documentation
- **[horizOn Docs](https://horizon.pm/docs)** - Full API documentation

## Support

- 📖 **Documentation**: [docs.horizon.pm](https://docs.horizon.pm)
- 💬 **Discord**: [discord.gg/horizOn](https://discord.gg/JFmaXtguku)
- 🐛 **Issues**: [GitHub Issues](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues)

## License

MIT License - Copyright (c) [ProjectMakers](https://projectmakers.de)

See [LICENSE](LICENSE) for details.
