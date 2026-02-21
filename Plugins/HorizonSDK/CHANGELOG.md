# Changelog

All notable changes to the horizOn SDK for Unreal Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-02-21

### Added

- **Plugin scaffold** -- HorizonSDK and HorizonSDKEditor modules with UE 5.5+ support.
- **Core types** -- Shared enums, structs, and delegates (`HorizonTypes.h`).
- **Data models** -- `FHorizonUserData`, `FHorizonLeaderboardEntry`, `FHorizonNewsEntry`, `FHorizonNetworkResponse`.
- **Configuration** -- `UHorizonConfig` (UDeveloperSettings) with API key, hosts, timeout, retry, and log level.
- **Session persistence** -- `UHorizonSessionSave` (USaveGame) for offline session caching.
- **HTTP client** -- `UHorizonHttpClient` with retry logic, rate-limit handling, and ping-based host selection.
- **Subsystem** -- `UHorizonSubsystem` (UGameInstanceSubsystem) as the main entry point.
- **Auth Manager** -- Anonymous, email, and Google sign-up / sign-in with session restore.
- **Cloud Save Manager** -- JSON and binary save/load operations.
- **Leaderboard Manager** -- Score submission, top/rank/around queries with caching.
- **Remote Config Manager** -- Typed config retrieval (string, int, float, bool) with caching.
- **News Manager** -- News feed with language filtering and 5-minute TTL cache.
- **Gift Code Manager** -- Code validation and redemption.
- **Feedback Manager** -- Bug reports, feature requests, and general feedback with device info.
- **User Log Manager** -- Server-side structured logging (info, warning, error).
- **Blueprint Library** -- `UHorizonBlueprintLibrary` for convenient Blueprint access.
- **Blueprint Async Actions** -- Async nodes for all SDK operations (Connect, Auth, CloudSave, Leaderboard, RemoteConfig, News, GiftCode, Feedback).
- **Editor tooling** -- Tools > horizOn menu with config import, SDK dashboard, and cache clearing.
- **Editor dashboard** -- Slate widget displaying config summary and connection test.
- **Example widget** -- `UHorizonExampleWidget` base class for quick SDK testing from UMG.
