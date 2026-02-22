# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains the **horizOn SDK** plugin for Unreal Engine 5.5+. It is a client-side SDK that connects UE5 games to the horizOn backend-as-a-service platform. All plugin code lives under `Plugins/HorizonSDK/`.

## Build Notes

This is a plugin, not a standalone project. It cannot be compiled on its own. To compile:
1. Place the plugin inside an Unreal Engine 5.5+ project's `Plugins/` directory.
2. Build the project normally through the editor or `UnrealBuildTool`.

There are no standalone build, lint, or test commands for this repository.

## Architecture

### Subsystem + Managers Pattern

The SDK uses a **UGameInstanceSubsystem + UObject managers** architecture:

- **UHorizonSubsystem** (`HorizonSubsystem.h/cpp`) is the single entry point. It creates and owns the HTTP client and all managers in `Initialize()`.
- **UHorizonHttpClient** (`Http/HorizonHttpClient.h/cpp`) wraps `FHttpModule` with automatic retry, HTTP 429 rate-limit handling, and multi-host ping-based server selection. All managers share this single instance.
- **UHorizonConfig** (`HorizonConfig.h/cpp`) is a `UDeveloperSettings` (editable in Project Settings). Access the CDO anywhere via `UHorizonConfig::Get()`.
- **UHorizonSessionSave** (`HorizonSessionSave.h/cpp`) is a `USaveGame` that persists session tokens to disk for automatic session restoration.

### Manager Initialization Pattern

Managers fall into two categories based on whether they need auth context:

- **Auth-independent** managers receive only `HttpClient`: `RemoteConfig`, `News`
- **Auth-dependent** managers receive both `HttpClient` and `Auth`: `CloudSave`, `Leaderboard`, `GiftCodes`, `Feedback`, `UserLogs`
- **Auth manager** itself receives only `HttpClient` (it IS the auth provider)

See `HorizonSubsystem.cpp::Initialize()` for the exact wiring.

### Two-Tier Delegate System

The SDK exposes functionality through two parallel API surfaces:

1. **C++ delegates** (in `HorizonTypes.h`): `FOnAuthComplete`, `FOnRequestComplete`, `FOnStringComplete`, `FOnBinaryComplete` -- used by managers directly. These are single-cast `DECLARE_DELEGATE` types.
2. **Blueprint async actions** (in `AsyncActions/`): Each `UBlueprintAsyncActionBase` subclass wraps manager calls with `FOnAuthAsyncSuccess`/`FOnAuthAsyncFailure`-style multi-cast delegates exposed as output pins. The async action's `Activate()` calls the appropriate manager method, and the completion callback broadcasts to the Blueprint pins.

### Response Flow

All HTTP responses flow through `FHorizonNetworkResponse` (`Models/HorizonNetworkResponse.h`), which wraps: `bSuccess`, `StatusCode`, `ErrorMessage`, `ErrorCode` (mapped from HTTP status via `StatusToErrorCode()`), `JsonData` (C++ only), and `BinaryData`.

### Adding a New Manager

1. Create `UHorizonNewManager : UObject` in `Managers/` with `Initialize(UHorizonHttpClient*, [UHorizonAuthManager*])`.
2. Add a `UPROPERTY(BlueprintReadOnly)` pointer in `UHorizonSubsystem.h`.
3. `NewObject` + `Initialize()` it in `UHorizonSubsystem::Initialize()`.
4. Create `UHorizonAsync_New : UBlueprintAsyncActionBase` in `AsyncActions/` with static factory `UFUNCTION`s using `meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject")`.
5. (Optional) Add static helper functions in `UHorizonBlueprintLibrary`.

### Module Structure

The plugin has two modules defined in `HorizonSDK.uplugin`:

- **HorizonSDK** (Runtime): depends on Core, CoreUObject, Engine, HTTP, Json, JsonUtilities, UMG, Slate, SlateCore, RHI
- **HorizonSDKEditor** (Editor): depends on Core, CoreUObject, Engine, UnrealEd, Slate, SlateCore, ToolMenus, DesktopPlatform, HTTP, InputCore, HorizonSDK

## Naming Conventions (UE5 Standard)

- `F` prefix: structs (`FHorizonNetworkResponse`, `FHorizonUserData`)
- `E` prefix: enums (`EHorizonAuthType`, `EHorizonConnectionStatus`)
- `U` prefix: UObject-derived classes (`UHorizonSubsystem`, `UHorizonAuthManager`)
- `S` prefix: Slate widgets (`SHorizonEditorWidget`)
- `HORIZONSDK_API`: export macro for the runtime module
- `LogHorizonSDK`: log category (declared in `HorizonSDKModule.h`)

## API Endpoint Base

All API requests go to endpoints under `/api/v1/app/`. Endpoint paths follow the pattern: `/api/v1/app/{feature}/{action}` (e.g., `/api/v1/app/auth/signup/anonymous`, `/api/v1/app/leaderboard/top`).

## Reference Documents

- `docs/plans/2026-02-21-horizon-unreal-sdk-design.md` -- Design decisions
- `docs/plans/2026-02-21-horizon-unreal-sdk-implementation.md` -- Implementation plan

## Release Pipeline

Releases are fully automated via semantic-release on push to `main`:

1. `@semantic-release/commit-analyzer` determines version bump from commit types
2. `@semantic-release/exec` auto-updates `VersionName` in `HorizonSDK.uplugin` and the version badge in `README.md`
3. `@semantic-release/git` commits the updated files with `chore(release): X.Y.Z [skip ci]`
4. `@semantic-release/github` creates a GitHub release
5. A second CI job zips `Plugins/HorizonSDK/` and uploads it as a release asset
6. A repository dispatch triggers changelog sync to the `horizOn-Changelog` repo

**Do not manually edit** `CHANGELOG.md`, the `VersionName` in `.uplugin`, or the version badge in `README.md` -- these are managed by the release pipeline.

## Commit Conventions

This repository uses **Conventional Commits** with **semantic-release** for automated versioning.

### Format

```
<type>(<scope>): <subject>
```

### Types

| Type | Description | Release |
|------|-------------|---------|
| `feat` | New feature | Minor (0.x.0) |
| `fix` | Bug fix | Patch (0.0.x) |
| `perf` | Performance improvement | Patch (0.0.x) |
| `docs` | Documentation only | No release |
| `chore` | Maintenance / tooling | No release |
| `refactor` | Code restructuring | No release |
| `style` | Code style / formatting | No release |
| `test` | Test additions / changes | No release |

Add `BREAKING CHANGE:` in the footer or `!` after the type for a major version bump.

## CRITICAL: No AI Attribution in Commits

- Never add `Co-Authored-By` lines mentioning Claude, Anthropic, or AI
- Never mention "Claude", "Claude Code", "AI-generated", "AI-assisted", or similar in commit messages, PR descriptions, or changelogs
- This applies to all git operations: commits, PRs, tags, release notes
