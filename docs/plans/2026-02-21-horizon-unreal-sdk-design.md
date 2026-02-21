# horizOn Unreal SDK - Design Document

**Date:** 2026-02-21
**Target:** Unreal Engine 5.5+
**Distribution:** Marketplace-ready Plugin
**Blueprint Support:** Full (C++ and Blueprints)

---

## Overview

The horizOn Unreal SDK provides game developers with a drop-in plugin for integrating horizOn backend services into Unreal Engine 5.5+ projects. It replicates the full feature set of the existing Unity and Godot SDKs with idiomatic UE5 patterns.

## Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| UE Version | 5.5+ only | Latest APIs, smallest support surface |
| Blueprint | Full support | Maximizes audience, standard for marketplace plugins |
| Distribution | Marketplace-ready plugin | Enables Fab/Marketplace distribution |
| Subsystem | UGameInstanceSubsystem | Idiomatic UE5, automatic lifecycle |
| Editor | Full Editor Utility Widget | Config importer + dashboard + test panel |
| Example | Full example level | Map with UMG widget demonstrating all features |

## Plugin Structure

```
Plugins/HorizonSDK/
├── HorizonSDK.uplugin
├── Source/
│   ├── HorizonSDK/                       # Runtime module
│   │   ├── HorizonSDK.Build.cs
│   │   ├── Public/
│   │   │   ├── HorizonSDKModule.h
│   │   │   ├── HorizonSubsystem.h        # Main entry point
│   │   │   ├── HorizonConfig.h           # UDeveloperSettings
│   │   │   ├── HorizonTypes.h            # Enums, structs, delegates
│   │   │   ├── Http/
│   │   │   │   └── HorizonHttpClient.h
│   │   │   ├── Managers/
│   │   │   │   ├── HorizonAuthManager.h
│   │   │   │   ├── HorizonCloudSaveManager.h
│   │   │   │   ├── HorizonLeaderboardManager.h
│   │   │   │   ├── HorizonRemoteConfigManager.h
│   │   │   │   ├── HorizonNewsManager.h
│   │   │   │   ├── HorizonGiftCodeManager.h
│   │   │   │   ├── HorizonFeedbackManager.h
│   │   │   │   └── HorizonUserLogManager.h
│   │   │   ├── Models/
│   │   │   │   ├── HorizonUserData.h
│   │   │   │   ├── HorizonLeaderboardEntry.h
│   │   │   │   ├── HorizonNewsEntry.h
│   │   │   │   └── HorizonNetworkResponse.h
│   │   │   └── HorizonBlueprintLibrary.h
│   │   └── Private/
│   │       └── (matching .cpp files)
│   └── HorizonSDKEditor/                 # Editor module
│       ├── HorizonSDKEditor.Build.cs
│       ├── Public/
│       │   ├── HorizonSDKEditorModule.h
│       │   ├── HorizonConfigImporter.h
│       │   └── HorizonEditorWidget.h
│       └── Private/
│           └── (matching .cpp files)
├── Content/
│   ├── Example/
│   │   ├── Maps/HorizonExampleMap.umap
│   │   └── UI/WBP_HorizonExample.uasset
│   └── Editor/
│       └── WBP_HorizonEditor.uasset
├── Resources/
│   └── Icon128.png
├── Config/
│   └── FilterPlugin.ini
├── Docs/
│   ├── QUICKSTART.md
│   └── API_REFERENCE.md
├── README.md
├── LICENSE
└── CHANGELOG.md
```

## Architecture

### Core Pattern: UGameInstanceSubsystem

```
UHorizonSubsystem (GameInstanceSubsystem)
  ├── UHorizonHttpClient         (networking, retry, rate-limit)
  ├── UHorizonAuthManager        (auth flows, session cache)
  ├── UHorizonLeaderboardManager (scores, rankings)
  ├── UHorizonCloudSaveManager   (JSON + binary)
  ├── UHorizonRemoteConfigManager(typed getters, cache)
  ├── UHorizonNewsManager        (articles, language filter)
  ├── UHorizonGiftCodeManager    (validate, redeem)
  ├── UHorizonFeedbackManager    (bugs, features, device info)
  └── UHorizonUserLogManager     (server-side logs)
```

The subsystem is automatically created by the engine when a GameInstance exists. No manual initialization required. Managers are created as UObject children during `Initialize()`.

### Async Pattern

C++ and Blueprint use different async patterns:

**C++ (delegate callbacks):**
```cpp
Auth->SignUpAnonymous(TEXT("Player"), FOnAuthComplete::CreateLambda([](bool bSuccess) {
    // handle result
}));
```

**Blueprint (latent async action nodes):**
`UBlueprintAsyncActionBase` subclasses for each major operation, providing `OnSuccess` and `OnFailure` exec output pins.

### Configuration

`UHorizonConfig` extends `UDeveloperSettings` -- appears in Project Settings > Plugins > horizOn.

| Setting | Type | Default |
|---------|------|---------|
| ApiKey | FString | "" |
| Hosts | TArray<FString> | [] |
| ConnectionTimeoutSeconds | int32 | 10 |
| MaxRetryAttempts | int32 | 3 |
| RetryDelaySeconds | float | 1.0 |
| LogLevel | EHorizonLogLevel | Info |

### Session Persistence

`UHorizonSessionSave` extends `USaveGame`. Stores anonymous token, last session token, user ID. Saved/loaded via `UGameplayStatics::SaveGameToSlot("HorizonSession", 0)`.

## HTTP Client

`UHorizonHttpClient` wraps `FHttpModule`:

**Headers applied automatically:**
- `X-API-Key: {ApiKey}`
- `Authorization: Bearer {SessionToken}` (when authenticated)
- `Content-Type: application/json` or `application/octet-stream`

**Retry strategy (matches Unity/Godot SDKs):**
- 4xx: Return immediately
- 429: Wait `Retry-After` header duration, then retry
- 5xx: Retry with exponential backoff
- Connection failure: Retry with backoff

**Host selection:**
- Single host: Health check only (`/actuator/health`)
- Multiple hosts: Ping all 3x each, select lowest latency

## Data Models

### FHorizonUserData
| Field | Type |
|-------|------|
| UserId | FString |
| Email | FString |
| DisplayName | FString |
| AuthType | EHorizonAuthType (Anonymous/Email/Google) |
| AccessToken | FString |
| AnonymousToken | FString |
| bIsEmailVerified | bool |
| bIsAnonymous | bool |

### FHorizonLeaderboardEntry
| Field | Type |
|-------|------|
| Position | int32 |
| Username | FString |
| Score | int64 |

### FHorizonNewsEntry
| Field | Type |
|-------|------|
| Id | FString |
| Title | FString |
| Message | FString |
| ReleaseDate | FString |
| LanguageCode | FString |

### FHorizonNetworkResponse
| Field | Type |
|-------|------|
| bSuccess | bool |
| StatusCode | int32 |
| ErrorMessage | FString |
| JsonData | TSharedPtr<FJsonObject> (C++ only) |
| BinaryData | TArray<uint8> |

### Enums
- `EHorizonAuthType`: Anonymous, Email, Google
- `EHorizonConnectionStatus`: Disconnected, Connecting, Connected, Failed
- `EHorizonLogLevel`: Debug, Info, Warning, Error, None

## Feature Managers

All managers are `UObject` subclasses owned by the subsystem. All async methods take delegate callbacks. Blueprint async actions provide latent nodes.

### UHorizonAuthManager
- SignUpAnonymous, SignUpEmail, SignUpGoogle
- SignInEmail, SignInAnonymous, SignInGoogle
- RestoreSession, CheckAuth, SignOut
- ChangeName, ForgotPassword, VerifyEmail
- Events: OnUserSignedIn, OnUserSignedOut

### UHorizonCloudSaveManager
- Save/Load (FString -- JSON text)
- SaveBytes/LoadBytes (TArray<uint8> -- binary)
- SaveObject<T>/LoadObject<T> (serialized via FJsonObjectConverter)

### UHorizonLeaderboardManager
- SubmitScore(int64 Score, FString Metadata)
- GetTop(int32 Limit), GetRank(), GetAround(int32 Limit)
- In-memory TMap cache, cleared on SubmitScore

### UHorizonRemoteConfigManager
- GetConfig(FString Key), GetAllConfigs()
- GetString/GetInt/GetFloat/GetBool with defaults
- In-memory TMap<FString, FString> cache

### UHorizonNewsManager
- LoadNews(int32 Limit, FString LanguageCode)
- GetNewsById(FString Id)
- 5-minute expiry cache

### UHorizonGiftCodeManager
- Redeem(FString Code), Validate(FString Code)

### UHorizonFeedbackManager
- Submit(Title, Category, Message, Email, bIncludeDeviceInfo)
- ReportBug, RequestFeature, SendGeneral helpers
- Auto-collects: UE version, OS, device model, GPU

### UHorizonUserLogManager
- CreateLog(LogLevel, Message, ErrorCode)
- Info, Warn, Error helpers
- Requires authenticated user, Pro tier

## API Endpoints

All endpoints match the Unity/Godot SDKs exactly:

```
Base: https://horizon.pm/api/v1/app/

GET  /actuator/health
POST /user-management/signup
POST /user-management/signin
POST /user-management/check-auth
POST /user-management/change-name
POST /user-management/verify-email
POST /user-management/forgot-password
POST /user-management/reset-password
POST /leaderboard/submit
GET  /leaderboard/top?userId={id}&limit={n}
GET  /leaderboard/rank?userId={id}
GET  /leaderboard/around?userId={id}&limit={n}
POST /cloud-save/save
POST /cloud-save/load
POST /cloud-save/save-bytes
GET  /cloud-save/load-bytes
GET  /remote-config/{key}
GET  /remote-config/all
GET  /news?limit={n}&languageCode={code}
POST /gift-codes/validate
POST /gift-codes/redeem
POST /user-feedback/submit
POST /user-logs/create
```

## Editor Tooling

### HorizonSDKEditor Module

**Menu items (Tools > horizOn):**
1. **Import Config...** -- File picker for horizOn_config.json, writes to Project Settings
2. **SDK Dashboard** -- Opens Editor Utility Widget
3. **Clear Session Cache** -- Deletes saved session data

**Editor Utility Widget (WBP_HorizonEditor):**
- Config panel: Shows API key (masked), hosts, environment
- Connection test: Ping button with per-host latency display
- Quick test buttons for each feature

## Example Level

`Content/Example/Maps/HorizonExampleMap.umap` with `WBP_HorizonExample`:

- Tab-based UMG widget (Auth | Cloud Save | Leaderboard | Config | News | Gift Codes | Feedback)
- Input fields and action buttons per tab
- Result display area
- Self-contained -- works by pressing Play in the example map

## Dependencies

**External:** None (zero third-party dependencies)

**Unreal Engine modules used:**
- HTTP (FHttpModule)
- Json / JsonUtilities (FJsonSerializer, FJsonObjectConverter)
- UMG (for example/editor widgets)
- DeveloperSettings (configuration)
- SaveGame (session persistence)

## Error Handling

HTTP status codes mapped to `EHorizonErrorCode` enum:
- 200-204: Success
- 400: InvalidRequest
- 401: Unauthorized
- 403: Forbidden
- 404: NotFound
- 409: Conflict
- 429: RateLimited
- 5xx: ServerError

Rate limiting: 10 requests/min per client. SDK respects Retry-After header automatically.
