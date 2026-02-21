# CLAUDE.md -- Project Memory for horizOn Unreal SDK

## Project Overview

This repository contains the **horizOn SDK** plugin for Unreal Engine. It is a client-side SDK that connects UE5 games to the horizOn backend-as-a-service platform. The plugin lives entirely under `Plugins/HorizonSDK/`.

## Architecture

The SDK uses a **UGameInstanceSubsystem + UObject managers** architecture:

- **UHorizonSubsystem** is the main entry point. It is a `UGameInstanceSubsystem` that creates and owns all manager objects and the shared HTTP client on initialization.
- Each **Manager** (Auth, CloudSave, Leaderboard, etc.) is a `UObject` created by the subsystem. Managers receive a pointer to the shared `UHorizonHttpClient` (and `UHorizonAuthManager` where authentication is required).
- **UHorizonHttpClient** wraps `FHttpModule` with automatic retry logic, HTTP 429 rate-limit handling, and multi-host ping-based server selection.
- **UHorizonConfig** (`UDeveloperSettings`) stores API key, hosts, timeout, retry, and log level. Editable in Project Settings.
- **UHorizonSessionSave** (`USaveGame`) persists session tokens to disk for automatic session restoration.

## Key Files and Purposes

```
Plugins/HorizonSDK/
  HorizonSDK.uplugin                 -- Plugin descriptor
  Source/
    HorizonSDK/                      -- Runtime module
      HorizonSDK.Build.cs            -- Build rules (dependencies: Core, HTTP, Json, UMG, etc.)
      Public/
        HorizonTypes.h               -- Shared enums, structs, delegates
        HorizonConfig.h              -- UDeveloperSettings config class
        HorizonSessionSave.h         -- USaveGame for session persistence
        HorizonSubsystem.h           -- UGameInstanceSubsystem entry point
        HorizonSDKModule.h           -- Module interface + log category
        HorizonBlueprintLibrary.h    -- Static BP function library
        Http/
          HorizonHttpClient.h        -- HTTP client with retry + ping selection
        Models/
          HorizonUserData.h          -- User data struct
          HorizonLeaderboardEntry.h  -- Leaderboard entry struct
          HorizonNewsEntry.h         -- News entry struct
          HorizonNetworkResponse.h   -- HTTP response wrapper struct
        Managers/
          HorizonAuthManager.h       -- Auth (anonymous, email, Google)
          HorizonCloudSaveManager.h  -- Cloud save (JSON + binary)
          HorizonLeaderboardManager.h -- Leaderboard (submit, top, rank, around)
          HorizonRemoteConfigManager.h -- Remote config (typed getters)
          HorizonNewsManager.h       -- News feed with cache
          HorizonGiftCodeManager.h   -- Gift code validate + redeem
          HorizonFeedbackManager.h   -- Feedback submission
          HorizonUserLogManager.h    -- Server-side user logging
        AsyncActions/
          HorizonAsync_*.h           -- Blueprint async action nodes
        Example/
          HorizonExampleWidget.h     -- UUserWidget demo class
      Private/
        (mirrors Public/ structure with .cpp implementations)

    HorizonSDKEditor/                -- Editor module (editor-only)
      HorizonSDKEditor.Build.cs
      Public/
        HorizonSDKEditorModule.h     -- Editor module with menu registration
      Private/
        HorizonSDKEditorModule.cpp   -- Config importer, dashboard tab, cache clear
        SHorizonEditorWidget.h/cpp   -- Slate dashboard widget
```

## Naming Conventions (UE5 Standard)

- `F` prefix: structs (e.g., `FHorizonNetworkResponse`, `FHorizonUserData`)
- `E` prefix: enums (e.g., `EHorizonAuthType`, `EHorizonConnectionStatus`)
- `U` prefix: UObject-derived classes (e.g., `UHorizonSubsystem`, `UHorizonAuthManager`)
- `S` prefix: Slate widgets (e.g., `SHorizonEditorWidget`)
- `HORIZONSDK_API` macro: export macro for the runtime module
- Log category: `LogHorizonSDK` (declared in `HorizonSDKModule.h`)

## API Endpoint Base

All API requests go through the HTTP client to endpoints under:
```
/api/v1/app/
```

Specific endpoint patterns:
- Auth: `/api/v1/app/auth/signup/anonymous`, `/api/v1/app/auth/signin/email`, etc.
- Cloud Save: `/api/v1/app/cloud-save/save`, `/api/v1/app/cloud-save/load`
- Leaderboard: `/api/v1/app/leaderboard/submit`, `/api/v1/app/leaderboard/top`
- Remote Config: `/api/v1/app/config`, `/api/v1/app/config/{key}`
- News: `/api/v1/app/news`
- Gift Codes: `/api/v1/app/gift-codes/redeem`, `/api/v1/app/gift-codes/validate`
- Feedback: `/api/v1/app/feedback`
- User Logs: `/api/v1/app/user-logs`

## Reference SDKs

Design and implementation plans are in:
- `docs/plans/2026-02-21-horizon-unreal-sdk-design.md`
- `docs/plans/2026-02-21-horizon-unreal-sdk-implementation.md`

## Build Notes

This is a plugin, not a standalone project. It cannot be compiled on its own. To compile:
1. Place the plugin inside an Unreal Engine 5.5+ project's `Plugins/` directory.
2. Build the project normally through the editor or `UnrealBuildTool`.

There are no standalone build or test commands for this repository.

## Module Dependencies

**Runtime (HorizonSDK):** Core, CoreUObject, Engine, HTTP, Json, JsonUtilities, UMG, Slate, SlateCore

**Editor (HorizonSDKEditor):** Core, CoreUObject, Engine, UnrealEd, Slate, SlateCore, ToolMenus, DesktopPlatform, HTTP, InputCore, HorizonSDK
