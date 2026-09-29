<p align="center">
  <a href="https://horizon.pm">
    <img src="https://horizon.pm/media/images/og-image.png" alt="horizOn - Game Backend & Live-Ops Dashboard" />
  </a>
</p>

# horizOn SDK for Unreal Engine

[![Unreal Engine 5.5+](https://img.shields.io/badge/Unreal_Engine-5.5%2B-blue?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.6.0-orange)](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/releases)

Official Unreal Engine SDK for **horizOn** Backend-as-a-Service by [ProjectMakers](https://projectmakers.de).

## Features

| Feature | Manager | Description |
|---------|---------|-------------|
| 🔐 **Authentication** | `UHorizonAuthManager` | Anonymous, email, Google, and Apple sign-up / sign-in with session caching |
| 🏆 **Leaderboards** | `UHorizonLeaderboardManager` | Submit scores, retrieve top entries, rank, and surrounding entries |
| ☁️ **Cloud Save** | `UHorizonCloudSaveManager` | Save and load player data in JSON or binary format |
| ⚙️ **Remote Config** | `UHorizonRemoteConfigManager` | Typed key-value retrieval (string, int, float, bool) with caching |
| 🌐 **Localization** | `UHorizonLocalizationManager` | Translated strings in 15 languages with an active language and caching |
| 📰 **News** | `UHorizonNewsManager` | In-game news feed with language filtering and TTL cache |
| 🎁 **Gift Codes** | `UHorizonGiftCodeManager` | Validate and redeem promotional codes, cosmetic unlocks via `grants` |
| 🧑‍🎤 **Player Profile** | `UHorizonPlayerProfileManager` | Avatar, frame and badges per player, cosmetic unlocks, shown in leaderboards |
| ✅ **Validated Actions** | `UHorizonValidatedActionsManager` | Server-checked runs: ticket with seed, validated submit with input log hash, rule codes, server-owned currency and loot, input log upload as evidence |
| 💬 **Feedback** | `UHorizonFeedbackManager` | Submit bug reports, feature requests, and general feedback |
| 📊 **User Logs** | `UHorizonUserLogManager` | Server-side structured logging for analytics and debugging |
| 💥 **Crash Reporting** | `UHorizonCrashManager` | Crash capture, exception tracking, breadcrumbs |
| ✉️ **Email Sending** | `UHorizonEmailSendingManager` | Transactional emails with templates, scheduling, multi-language support |

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

// Listen for the connection result (OnConnected must be a UFUNCTION)
Horizon->OnConnected.AddDynamic(this, &AMyActor::OnConnected);

// Connect to the backend
Horizon->ConnectToServer();

// In AMyActor::OnConnected(): sign up anonymously
Horizon->Auth->SignUpAnonymous(TEXT("Player1"),
    FOnAuthComplete::CreateLambda([](bool bSuccess)
    {
        // Handle result
    }));
```

### Blueprints

1. Use the **"Connect to horizOn Server"** async node to connect to the server.
2. Use **"Sign Up Anonymous"**, **"Sign In Email"**, or **"Sign In With Apple (Native)"** to authenticate.
3. Call any feature node (**"Submit Leaderboard Score"**, **"Cloud Save Data"**, **"Load News"**, etc.).

All async nodes expose **On Success** and **On Failure** execution pins for easy error handling.

## API Reference

### Connection

```cpp
UHorizonSubsystem* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();

// Connect to server
Horizon->ConnectToServer();

// Check status
Horizon->IsConnected();          // Returns true if connected
Horizon->GetConnectionStatus();  // Returns EHorizonConnectionStatus

// Disconnect and clear session state
Horizon->Disconnect();
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

// Apple sign-in (drop-in: native sheet on iOS, system browser elsewhere)
Horizon->Auth->SignInWithApple(FOnAuthComplete::CreateLambda([](bool bSuccess)
{
    // On success: Horizon->Auth->GetCurrentUser() returns FHorizonUserData with
    // AppleUserId and bIsPrivateRelayEmail populated.
}));

// Apple sign-in with a token you already obtained yourself
Horizon->Auth->SignInApple(TEXT("eyJraWQiOi..."),
    FOnAuthComplete::CreateLambda([](bool bSuccess) { }));

// Apple sign-up — first-time user, optionally pass profile data Apple sends only on first login
Horizon->Auth->SignUpApple(TEXT("eyJraWQiOi..."), TEXT("Jane"), TEXT("Doe"), TEXT("janed"),
    FOnAuthComplete::CreateLambda([](bool bSuccess) { }));

// Check authentication state
if (Horizon->Auth->IsSignedIn())
{
    FHorizonUserData User = Horizon->Auth->GetCurrentUser();
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

// Get top players (second argument: use the local cache)
Horizon->Leaderboard->GetTop(10, true,
    FOnLeaderboardEntriesComplete::CreateLambda([](bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries) { }));

// Get your rank
Horizon->Leaderboard->GetRank(true,
    FOnLeaderboardRankComplete::CreateLambda([](bool bSuccess, const FHorizonLeaderboardEntry& Entry) { }));

// Get players around your rank
Horizon->Leaderboard->GetAround(5, true,
    FOnLeaderboardEntriesComplete::CreateLambda([](bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries) { }));
```

A board can accept validated runs only (`FHorizonLeaderboardBoard::bValidatedOnly`, from
`ListBoards`). `SubmitScore` to such a board fails with the code `VALIDATED_SUBMIT_REQUIRED`
and writes nothing; the SDK does not retry it. The `OnComplete` signature is unchanged, read
the code with `Horizon->Leaderboard->GetLastSubmitErrorCode()` and submit through
[Validated Actions](#validated-actions) instead. A player banned from a board gets
`PLAYER_BANNED` from `SubmitScore` (same way, nothing written, not retried).

Every entry of top, around and rank carries the player's profile in `Entry.Profile`
(`AvatarId`, `FrameId`, `Badges`). Empty values mean "not set"; treat IDs your game
does not know as "not set" as well and fall back to a default avatar.

```cpp
for (const FHorizonLeaderboardEntry& Entry : Entries)
{
    const FString Avatar = Entry.Profile.HasAvatar() ? Entry.Profile.AvatarId : TEXT("avatar.default");
    UE_LOG(LogTemp, Log, TEXT("#%d %s %lld (%s, %d badges)"),
        Entry.Position, *Entry.Username, Entry.Score, *Avatar, Entry.Profile.Badges.Num());
}
```

### Cloud Saves

```cpp
// Save JSON object
FString SaveData = TEXT("{\"level\": 5, \"coins\": 1000}");
Horizon->CloudSave->Save(SaveData,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Load JSON object
Horizon->CloudSave->Load(
    FOnStringComplete::CreateLambda([](bool bSuccess, const FString& Data) { }));

// Save binary data
TArray<uint8> Bytes = /* your data */;
Horizon->CloudSave->SaveBytes(Bytes,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Load binary data
Horizon->CloudSave->LoadBytes(
    FOnBinaryComplete::CreateLambda([](bool bSuccess, const TArray<uint8>& Data) { }));
```

### Remote Config

```cpp
// Get a raw value (Key, bUseCache, callback)
Horizon->RemoteConfig->GetConfig(TEXT("game_version"), true,
    FOnConfigComplete::CreateLambda([](bool bSuccess, const FString& Value) { }));

// Get typed values with defaults (Key, DefaultValue, bUseCache, callback)
Horizon->RemoteConfig->GetInt(TEXT("max_level"), 100, true,
    TDelegate<void(bool, int32)>::CreateLambda([](bool bSuccess, int32 Value) { }));

Horizon->RemoteConfig->GetFloat(TEXT("difficulty"), 1.0f, true,
    TDelegate<void(bool, float)>::CreateLambda([](bool bSuccess, float Value) { }));

Horizon->RemoteConfig->GetBool(TEXT("maintenance_mode"), false, true,
    TDelegate<void(bool, bool)>::CreateLambda([](bool bSuccess, bool bValue) { }));

// Get all configs
Horizon->RemoteConfig->GetAllConfigs(true,
    FOnAllConfigsComplete::CreateLambda([](bool bSuccess, const TMap<FString, FString>& Configs) { }));
```

### Localization

```cpp
// The active language defaults to the system language (if one of the 15
// supported codes: en, de, es, fr, it, pt, nl, pl, ru, ja, zh, ar, ko, tr, id),
// otherwise "en". Change it at runtime (clears the translation cache):
Horizon->Localization->SetLanguage(TEXT("de"));

// Get a single translation. An empty language uses the active language.
// The server falls back to English when a key has no translation.
Horizon->Localization->GetLocalization(TEXT("welcome_message"), TEXT(""),
    FOnLocalizationComplete::CreateLambda([](bool bSuccess, const FString& Value) { }));

// Get all translations for a language
Horizon->Localization->GetAllLocalizations(TEXT("de"),
    FOnAllLocalizationsComplete::CreateLambda([](bool bSuccess, const TMap<FString, FString>& Translations) { }));

// List the available language codes
Horizon->Localization->GetAvailableLanguages(
    FOnLanguagesComplete::CreateLambda([](bool bSuccess, const TArray<FString>& Languages) { }));
```

### News

```cpp
// Limit, language code, bUseCache, callback
Horizon->News->LoadNews(20, TEXT("en"), true,
    FOnNewsComplete::CreateLambda([](bool bSuccess, const TArray<FHorizonNewsEntry>& Entries)
    {
        for (const auto& Entry : Entries)
        {
            UE_LOG(LogTemp, Log, TEXT("%s: %s"), *Entry.Title, *Entry.Message);
        }
    }));
```

### Gift Codes

`Redeem` needs a signed-in player and sends the player session (`Authorization: Bearer`). The server only redeems codes for the player who owns that session.

```cpp
// Validate
Horizon->GiftCodes->Validate(TEXT("ABCD-1234"),
    FOnGiftCodeValidateComplete::CreateLambda([](bool bRequestSuccess, bool bValid) { }));

// Redeem
Horizon->GiftCodes->Redeem(TEXT("ABCD-1234"),
    FOnGiftCodeRedeemComplete::CreateLambda([Horizon](bool bSuccess, const FString& GiftData, const FString& Message)
    {
        if (bSuccess)
        {
            // Parse GiftData for rewards.
            // Cosmetics from the code's "grants" are unlocked on the server:
            const TArray<FString>& Unlocked = Horizon->GiftCodes->GetLastGrantedUnlocks();
            // The cached player profile was cleared; GetProfile now shows the unlocks.
        }
    }));
```

A code whose `giftData` contains `grants` (for example `{"grants": ["badge.supporter"]}`)
unlocks those cosmetics for the player. `GetLastGrantedUnlocks()` returns them after a
successful redeem (empty when the code had no grants); in Blueprints use
**"Get Horizon Last Granted Unlocks"**.

### Player Profile

Each player has a small profile that leaderboards show next to name and score: an
avatar, an optional frame and up to three badges. The catalog of cosmetics (IDs with
type `avatar`, `frame` or `badge`, free or locked) is maintained per API key in the
horizOn Dashboard. The server stores IDs only; your game maps them to its own assets.
Locked cosmetics need an unlock, granted by a gift code with `grants` or in the Dashboard.

Both calls need a signed-in player and send the player session (`Authorization: Bearer`).

```cpp
// Load profile, unlocks and the catalog (with an availability flag per entry)
Horizon->PlayerProfile->GetProfile(
    FOnPlayerProfileComplete::CreateLambda([](bool bSuccess, const FHorizonPlayerProfileResult& Result,
        const FString& ErrorCode, const FString& ErrorMessage)
    {
        if (!bSuccess) { return; }
        for (const FHorizonCosmetic& Avatar : Result.GetCosmetics(TEXT("avatar")))
        {
            // Build the picker: Avatar.Id, Avatar.bLocked, Avatar.bAvailable
        }
    }));

// Replace the whole profile (pass the current values for slots you keep)
Horizon->PlayerProfile->SetProfile(
    TEXT("avatar.zombie_07"),
    TEXT(""),                          // empty clears the frame
    { TEXT("badge.supporter") },       // at most 3, empty array clears the badges
    FOnPlayerProfileComplete::CreateLambda([](bool bSuccess, const FHorizonPlayerProfileResult& Result,
        const FString& ErrorCode, const FString& ErrorMessage)
    {
        if (!bSuccess && ErrorCode == TEXT("COSMETIC_LOCKED"))
        {
            // The player does not own this cosmetic yet
        }
    }));

// Last result, cached until sign-in, sign-out or a gift code that granted unlocks
if (Horizon->PlayerProfile->HasCurrentProfile())
{
    const FHorizonPlayerProfileResult& Current = Horizon->PlayerProfile->GetCurrentProfile();
    bool bCanUse = Current.IsAvailable(TEXT("frame.gold"));
}
```

`ErrorCode` is the server code: `INVALID_COSMETIC_ID`, `INVALID_BADGES` (more than 3 or
listed twice), `COSMETIC_NOT_FOUND`, `COSMETIC_TYPE_MISMATCH`, `COSMETIC_LOCKED`,
`SESSION_REQUIRED`, `SESSION_FORBIDDEN`, `PLAYER_NOT_FOUND`. Without a signed-in player
the SDK fails locally with `SESSION_REQUIRED` and sends nothing; more than 3 badges or a
malformed ID fail locally too. Without a server code the SDK reports the HTTP mapping
(`RATE_LIMITED`, `CONNECTION_FAILED`, `SERVER_ERROR`, ...). Cosmetic IDs are 1 to 32
characters: lowercase letters, digits, `.`, `_`, `-`.

#### Blueprints

- **"Get Player Profile"**: Loads profile, unlocks and catalog (On Success: Result, On Failure: Error Code, Error Message)
- **"Set Player Profile"**: Replaces avatar, frame and badges
- **"Get Horizon Current Player Profile"**: The last cached result
- **"Get Horizon Cosmetics Of Type"** / **"Is Horizon Cosmetic Available"**: Picker helpers
- **"Get Horizon Last Granted Unlocks"**: Unlocks from the last redeemed gift code

### Validated Actions

Validated Actions let the server check a run before its result counts. A run starts with a
single use ticket and a server seed. Seed all randomness of the run with that seed, record the
player's inputs as bytes, and submit the result together with the SHA-256 of that input log.
The server checks the rules of your API key (maximum and minimum score, minimum duration,
score per second, stages; set in the horizOn Dashboard) before anything is written and answers
a rejection with a machine readable code. Rule values never reach the client.

Both calls need a signed-in player and send the player session (`Authorization: Bearer`).

```cpp
#include "Managers/HorizonValidatedActionsManager.h"

// 1. Start a run (bound to the board "weekly"; an empty key starts an unbound run)
Horizon->ValidatedActions->StartRun(TEXT("weekly"),
    FOnValidatedRunStarted::CreateLambda([](bool bSuccess, const FHorizonValidatedRun& Run,
        const FString& ErrorCode, const FString& ErrorMessage)
    {
        if (!bSuccess) { return; } // for example RUN_RATE_LIMITED
        FRandomStream Random(Run.Seed); // all randomness of the run from the server seed
    }));

// 2. Play, append every input to TArray<uint8> InputLog, then submit the current run
Horizon->ValidatedActions->SubmitValidated(18250, InputLog, TEXT(""), TEXT(""), {},
    FOnValidatedSubmitComplete::CreateLambda([](bool bSuccess, const FHorizonValidatedSubmitResult& Result,
        const FString& ErrorCode, const FString& ErrorMessage)
    {
        if (!bSuccess)
        {
            // Rule codes such as DURATION_TOO_SHORT or SCORE_ABOVE_MAX, ticket codes such as TICKET_EXPIRED
            UE_LOG(LogTemp, Warning, TEXT("Run rejected: %s"), *ErrorCode);
            return;
        }
        UE_LOG(LogTemp, Log, TEXT("Best %lld, rank %lld, %lld s"), Result.BestScore, Result.Rank, Result.DurationSeconds);
    }));

// With a hash computed elsewhere (64 hex characters)
const FString Hash = UHorizonValidatedActionsManager::ComputeInputLogHash(InputLog);
Horizon->ValidatedActions->SubmitValidatedWithHash(18250, Hash, TEXT("wave_10"), TEXT("weekly"), {}, OnComplete);
```

- `SubmitValidated` parameters: `Score` (ignored by the server for a run without board),
  `InputLog`, `Stage` (for stage rules, empty when none), `LeaderboardKey` (empty uses the
  ticket's board), `Earned` (`TArray<FHorizonEarnedValue>`, see
  [Server-owned state](#server-owned-state); leave it empty unless the rules define values).
- The manager keeps the started run: `GetCurrentRun()`, `HasActiveRun()`, `DiscardRun()`.
  A ticket is single use: after an accepted run, the ticket codes (`TICKET_*`), every rule
  or value rejection (422) and 403 `SCORE_LIMIT_REACHED` the current run is cleared. The
  server checks the board and the request before it touches the ticket, so the run stays on
  `LEADERBOARD_NOT_FOUND`, `LEADERBOARD_MISMATCH`, `SCORE_REQUIRED` and
  `PLAYER_NAME_REQUIRED` (fix the call and submit again), and also on network errors, 401,
  429 and 5xx. Sign-out clears the run.
- 403 `PLAYER_BANNED`: the player is banned from the board. The server checks this before it
  touches the ticket, so the run stays (the ticket works again after an unban). Drop it with
  `DiscardRun()` or start a run without board.
- `GetLastErrorCode()` returns the code of the last failed call.
- An accepted run with a board clears the leaderboard cache, like `SubmitScore`.

Error codes: `SESSION_REQUIRED`, `NO_ACTIVE_RUN` (no started run) and
`INVALID_INPUT_LOG_HASH` fail locally without a request. Server codes: `TICKET_INVALID`,
`TICKET_EXPIRED`, `TICKET_FOREIGN`, `TICKET_CONSUMED`, `LEADERBOARD_MISMATCH`,
`STAGE_REQUIRED`, `STAGE_UNKNOWN`, `SCORE_ABOVE_MAX`, `SCORE_BELOW_MIN`,
`STAGE_SCORE_ABOVE_MAX`, `STAGE_SCORE_BELOW_MIN`, `DURATION_TOO_SHORT`,
`SCORE_RATE_TOO_HIGH`, `UNKNOWN_VALUE_KEY`, `DUPLICATE_VALUE_KEY`, `EARNED_ABOVE_MAX`,
`EARNED_BELOW_MIN`, `INSUFFICIENT_BALANCE`, `SCORE_LIMIT_REACHED`, `PLAYER_BANNED`, `SCORE_REQUIRED`, `PLAYER_NAME_REQUIRED`,
`LEADERBOARD_NOT_FOUND`, `PLAYER_NOT_FOUND`, `SESSION_FORBIDDEN`,
`VALIDATED_ACTIONS_UNAVAILABLE`, `RUN_RATE_LIMITED` and `RUN_CAPACITY_REACHED`. The two run
limits are not retried automatically (the wait can be up to an hour). A backend without the
feature (for example a Simple Server) gives `NOT_SUPPORTED`.

#### Server-owned state

The rules of your API key can define values (currency, loot counters) under `values`, for
example `"gold": {"maxPerRun": 500, "dailyCap": 5000}`. Only the server writes them. A run
earns (positive) or spends (negative) them through `Earned` of the submit; the server checks
the key (`UNKNOWN_VALUE_KEY`, also when the rules define no values), duplicates
(`DUPLICATE_VALUE_KEY`), `maxPerRun` / `minPerRun` (`EARNED_ABOVE_MAX`, `EARNED_BELOW_MIN`)
and the balance of a spend (`INSUFFICIENT_BALANCE`). These are 422 rejections: the ticket is
used up. The daily cap and `maxBalance` clamp a credit without rejecting the run.

```cpp
// Earn 250 gold and spend one chest key in the same run
TArray<FHorizonEarnedValue> Earned;
Earned.Add(FHorizonEarnedValue(TEXT("gold"), 250));
Earned.Add(FHorizonEarnedValue(TEXT("chest.key"), -1));

Horizon->ValidatedActions->SubmitValidated(18250, InputLog, TEXT(""), TEXT(""), Earned,
    FOnValidatedSubmitComplete::CreateLambda([](bool bSuccess, const FHorizonValidatedSubmitResult& Result,
        const FString& ErrorCode, const FString& ErrorMessage)
    {
        if (!bSuccess) { return; } // for example INSUFFICIENT_BALANCE
        const FHorizonPlayerStateValue* Key = Result.State.FindValue(TEXT("chest.key"));
        if (Key && Key->IsFullyCredited())
        {
            // The spend was applied: open the chest
        }
        UE_LOG(LogTemp, Log, TEXT("Gold: %lld"), Result.State.GetBalance(TEXT("gold")));
    }));

// Read the state at any time (for example at game start)
Horizon->ValidatedActions->GetState(FOnPlayerStateLoaded::CreateLambda(
    [](bool bSuccess, const FHorizonPlayerState& State, const FString& ErrorCode, const FString& ErrorMessage)
    {
        for (const FHorizonPlayerStateValue& Value : State.Values)
        {
            // "250 / 5000 today"; DailyCap 0 means no cap
            UE_LOG(LogTemp, Log, TEXT("%s: %lld (%lld / %lld today)"), *Value.Key, Value.Balance, Value.EarnedToday, Value.DailyCap);
        }
    }));
```

- `FHorizonPlayerState`: `Day` (current UTC day), `Values` (every key of the rules, sorted,
  balance 0 when never earned), `GetBalance(Key)`, `FindValue(Key)`. `UserId` is set by
  `GetState` only.
- `FHorizonPlayerStateValue`: `Key`, `Balance`, `EarnedToday`, `DailyCap` (0 = no cap),
  `Requested` and `Credited` (int64). `Requested` / `Credited` are set in a submit result for
  the values the run touched and 0 otherwise. `Credited < Requested` for a credit means the
  daily cap or `maxBalance` clamped it. A spend is either applied in full or `Credited` is 0
  (a parallel run of the same player used the balance first): grant a purchase only when
  `IsFullyCredited()`.
- `Result.State` is empty when the rules define no values.
- The manager caches the last known state: `GetCurrentState()`, `HasState()`,
  `GetBalance(Key)`, updated by `GetState` and every accepted run with a state, cleared on
  sign-out. `OnStateChanged` (Blueprint assignable) fires on every change.
- There is no method that writes the state. Support corrects balances in the horizOn
  Dashboard.

**Cloud save as a mirror.** The cloud save stays a blob the client writes. Keep server-owned
values there only as a copy:

1. After every accepted run copy `Result.State` into your save data (for display and an
   offline start).
2. At game start call `GetState` and overwrite the copy with it, never the other way round.
3. Never send a value from the cloud save back as a balance. Balances change only through
   `Earned` of a validated run.
4. Values earned offline are sent as `Earned` with the next validated run; the per run and
   daily limits apply as usual.

`AHorizonValidatedActionsExample::BuildCloudSaveMirror(State)` shows one compact format
(`{"day":"2026-09-29","balances":{"gold":1250}}`).

#### Evidence

The server can ask for the input log of an accepted run, for example when the run is flagged
or lands in the board's top N (`evidenceTopN`, set per board in the horizOn Dashboard). The
result then carries `Result.Evidence` with `bRequired = true`, `RunId`, `UploadBefore` (24 h)
and `MaxBytes` (32,768). You can review and download the logs in the horizOn Dashboard and
replay them with the run's seed.

- After `SubmitValidated` the SDK uploads the same bytes by itself
  (`PUT /api/v1/app/validated-actions/runs/{runId}/evidence`, the log as standard base64).
  Turn this off with `Horizon->ValidatedActions->bAutoUploadEvidence = false`.
- After `SubmitValidatedWithHash` the SDK does not know the bytes. Call `UploadEvidence`
  yourself with the exact bytes of the submitted hash:

```cpp
if (Result.Evidence.bRequired)
{
    Horizon->ValidatedActions->UploadEvidence(Result.Evidence.RunId, InputLog,
        FOnEvidenceUploaded::CreateLambda([](bool bSuccess, const FHorizonEvidenceUploadResult& Upload,
            const FString& ErrorCode, const FString& ErrorMessage)
        {
            if (!bSuccess && UHorizonValidatedActionsManager::IsEvidenceUploadRetryable(ErrorCode))
            {
                // EVIDENCE_HASH_MISMATCH (send the correct bytes) or a network error: try again later
            }
        }));
}
```

- Every upload, automatic or manual, fires `OnEvidenceUploaded(RunId, Bytes)` or
  `OnEvidenceUploadFailed(RunId, ErrorCode, ErrorMessage)` (Blueprint assignable).
  `GetLastEvidenceErrorCode()` holds the last upload error. An upload never changes the
  submit result or `GetLastErrorCode()`: the run counts either way.
- Upload codes: `EVIDENCE_HASH_MISMATCH` (422, the request stays open, send the correct bytes
  again), `EVIDENCE_NOT_REQUESTED` (404), `EVIDENCE_ALREADY_UPLOADED` (409),
  `EVIDENCE_EXPIRED` (410, past `UploadBefore`), `EVIDENCE_TOO_LARGE` (413, also checked
  locally against `MaxBytes` of the evidence request), `EVIDENCE_INVALID_ENCODING` (400).
  Local codes: `SESSION_REQUIRED`, `INVALID_RUN_ID`, `EMPTY_INPUT_LOG`.
- Send an upload again only when `IsEvidenceUploadRetryable(ErrorCode)` is true:
  `EVIDENCE_HASH_MISMATCH` and network errors (`CONNECTION_FAILED`). All other codes are
  final. The SDK itself never repeats an upload beyond the HTTP client's network retries.
- The automatic upload starts after the submit callback ran, with `Result.Evidence.RunId`
  (falling back to `Result.RunId`).

#### Blueprints

- **"Start Validated Run"**: Starts a run (On Success: Run, On Failure: Error Code, Error Message)
- **"Submit Validated Run"**: Hashes the input log and submits the current run (On Success: Result, with `State`)
- **"Submit Validated Run With Hash"**: Same with a ready SHA-256 hash
- **"Get Validated Player State"**: Loads the server-owned values (On Success: State)
- **"Upload Validated Run Evidence"**: Uploads the input log of an accepted run (On Success: Result with `RunId`, `Status`, `Bytes`)
- **"Compute Input Log Hash"**, **"Has Active Run"**, **"Get Last Error Code"**, **"Discard Run"**,
  **"Has State"**, **"Get Balance"**, **"Is Evidence Upload Retryable"**, **"Get Last Evidence Error Code"**,
  the property **Auto Upload Evidence** and the events **On State Changed**, **On Evidence Uploaded**,
  **On Evidence Upload Failed** on `ValidatedActions`
- **"Get Horizon Current Validated Run"**: The current run
- **"Get Horizon Validated Player State"**: The cached state
- **"Get Horizon Validated Balance"**, **"Find Horizon Validated Value"**,
  **"Is Horizon Value Fully Credited"**: Helpers for a state and its values

### Feedback

```cpp
// Bug report
Horizon->Feedback->ReportBug(TEXT("Crash on level 5"), TEXT("Game crashes when opening inventory"),
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// Feature request
Horizon->Feedback->RequestFeature(TEXT("Dark mode"), TEXT("Please add dark mode option"),
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));

// General feedback with email and device info (Title, Category, Message, Email, bIncludeDeviceInfo, callback)
Horizon->Feedback->Submit(TEXT("Title"), TEXT("GENERAL"), TEXT("Message"),
    TEXT("email@example.com"), true,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error) { }));
```

### User Logs

```cpp
// Message, callback, optional error code
Horizon->UserLogs->Info(TEXT("Tutorial completed"),
    FOnUserLogComplete::CreateLambda([](bool bSuccess, const FString& LogId, const FString& CreatedAt) { }));
Horizon->UserLogs->Warn(TEXT("Low memory detected"), FOnUserLogComplete());
Horizon->UserLogs->Error(TEXT("Failed to load asset"), FOnUserLogComplete(), TEXT("ERR_001"));
```

### Crash Reporting

Track crashes, non-fatal exceptions, and breadcrumbs to monitor game stability. The `UHorizonCrashManager` can automatically capture engine-level crashes when capture is active.

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

Use these Blueprint nodes (async nodes are marked):

- **"Record Exception"** (async) - Reports a non-fatal exception
- **"Report Crash"** (async) - Reports a fatal crash
- **"Horizon Record Breadcrumb"** - Adds context breadcrumb
- **"Horizon Start Crash Capture"** - Begins automatic capture
- **"Horizon Stop Crash Capture"** - Stops automatic capture
- **"Horizon Set Crash Custom Key"** - Sets report metadata

#### Limits

| Parameter | Limit |
|-----------|-------|
| Reports per minute | 5 |
| Reports per session | 20 |
| Breadcrumbs (ring buffer) | 50 |
| Custom keys | 10 |

### Email Sending

Send transactional emails to registered players. Create multi-language HTML templates with variable placeholders in the horizOn Dashboard, then trigger immediate or scheduled email delivery from your game using the SDK. Emails are sent through your own SMTP server -- horizOn handles the queue, rendering, and scheduling while you keep full control over branding and deliverability.

#### C++

```cpp
// Get subsystem
auto* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();

// Send immediate email
TMap<FString, FString> Variables;
Variables.Add("username", "John");

Horizon->EmailSending->SendEmail(
    TEXT("user-uuid"), TEXT("welcome"), Variables, TEXT("en"),
    FOnSendEmailComplete::CreateLambda([](bool bSuccess, const FSendEmailResponse& Response) {
        if (bSuccess)
            UE_LOG(LogTemp, Log, TEXT("Email queued: %s"), *Response.Id);
    })
);

// Schedule email for later
Horizon->EmailSending->SendEmail(
    TEXT("user-uuid"), TEXT("reminder"), Variables, TEXT("en"),
    TEXT("2026-04-12T09:00:00Z"),
    FOnSendEmailComplete::CreateLambda([](bool bSuccess, const FSendEmailResponse& Response) {
        if (bSuccess)
            UE_LOG(LogTemp, Log, TEXT("Scheduled: %s"), *Response.ScheduledAt);
    })
);

// Check status
Horizon->EmailSending->GetEmailStatus(
    EmailId,
    FOnEmailStatusComplete::CreateLambda([](bool bSuccess, const FEmailStatusResponse& Response) {
        if (bSuccess)
            UE_LOG(LogTemp, Log, TEXT("Status: %s"), *Response.Status);
    })
);

// Cancel a scheduled email
Horizon->EmailSending->CancelEmail(
    EmailId,
    FOnCancelEmailComplete::CreateLambda([](bool bSuccess, const FCancelEmailResponse& Response) {
        if (bSuccess)
            UE_LOG(LogTemp, Log, TEXT("Cancelled: %s"), *Response.Message);
    })
);
```

#### Blueprints

Use the async nodes for Blueprint integration:

- **"Send Email"** - Send a transactional email to a user via a template
- **"Cancel Email"** - Cancel a pending or scheduled email
- **"Get Email Status"** - Get the current status of an email

All async nodes expose **On Success** and **On Failure** execution pins.

## Events / Delegates

### C++ Delegates

```cpp
// Connection
Horizon->OnConnected.AddDynamic(this, &AMyActor::OnConnected);
Horizon->OnConnectionFailed.AddDynamic(this, &AMyActor::OnConnectionFailed);

// Authentication
Horizon->Auth->OnUserSignedIn.AddDynamic(this, &AMyActor::OnSignedIn);
Horizon->Auth->OnUserSignedOut.AddDynamic(this, &AMyActor::OnSignedOut);

// Crash Reporting
Horizon->Crashes->OnCrashReported.AddDynamic(this, &AMyActor::OnCrashReported);
Horizon->Crashes->OnCrashReportFailed.AddDynamic(this, &AMyActor::OnCrashReportFailed);
Horizon->Crashes->OnSessionRegistered.AddDynamic(this, &AMyActor::OnSessionRegistered);
Horizon->Crashes->OnBreadcrumbRecorded.AddDynamic(this, &AMyActor::OnBreadcrumbRecorded);
```

### Blueprint Events

All async nodes expose **On Success** and **On Failure** execution pins. For event-driven patterns, bind to the event dispatchers on the subsystem (On Connected, On Connection Failed), the Auth manager (On User Signed In, On User Signed Out) and the Crashes manager.

## Configuration Options

Open **Project Settings > Plugins > horizOn SDK** to configure:

| Option | Default | Description |
|--------|---------|-------------|
| API Key | - | Your horizOn API key |
| Backend Hosts | empty | Backend server URL(s), for example `https://horizon.pm`. Must be set (or imported) before connecting. Single host skips ping; multiple hosts use latency-based selection. |
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

Validated Actions are cloud only: a Simple Server does not have them, the SDK reports `NOT_SUPPORTED`.

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
│   │   │   ├── Managers/        Feature managers (incl. CrashManager)
│   │   │   ├── AsyncActions/    Blueprint async nodes
│   │   │   ├── Example/         Example widget (monolithic test UI)
│   │   │   └── Examples/        Per-feature example actors + Hello horizOn
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

- 📖 **Documentation**: [horizon.pm/quickstart](https://horizon.pm/quickstart)
- 💬 **Discord**: [discord.gg/horizOn](https://discord.gg/JFmaXtguku)
- 🐛 **Issues**: [GitHub Issues](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues)

## License

MIT License - Copyright (c) [ProjectMakers](https://projectmakers.de)

See [LICENSE](LICENSE) for details.
