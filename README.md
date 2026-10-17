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
| 🔄 **Realtime** | `UHorizonRealtimeManager` | Optional Cloud invalidations for inbox, matches and chat; HTTP content reload |
| ⚙️ **Remote Config** | `UHorizonRemoteConfigManager` | Typed key-value retrieval (string, int, float, bool) with caching |
| 🌐 **Localization** | `UHorizonLocalizationManager` | Translated strings in 15 languages with an active language and caching |
| 📰 **News** | `UHorizonNewsManager` | In-game news feed with language filtering and TTL cache |
| 🎁 **Gift Codes** | `UHorizonGiftCodeManager` | Validate and redeem codes, cosmetic unlocks and replay-safe player values |
| 🧑‍🎤 **Player Profile** | `UHorizonPlayerProfileManager` | Avatar, frame and badges per player, cosmetic unlocks, shown in leaderboards |
| ✅ **Validated Actions** | `UHorizonValidatedActionsManager` | Server-checked runs: ticket with seed, validated submit with input log hash, rule codes, server-owned currency and loot, input log upload as evidence |
| 📬 **Inbox** | `UHorizonInboxManager` | Explicit snapshots, read/remove actions and entry-bound reward claims with full balance snapshots |
| 👥 **Friends** | `UHorizonFriendsManager` | Explicit revision-bound social snapshots, exact public lookup and original request IDs |
| 📊 **Statistics** | `UHorizonStatisticsManager` | Explicit atomic session batches with sum/max/min/last counters, optional UTC resets and safe retries |
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

Anonymous signup omits client tokens and stores the token issued by the server. It signs in with that token when signup returns no session. The deprecated token argument is ignored. Signup makes exactly one transport attempt: a lost response or HTTP error must be reconciled before any new identity is created. Use `SignInAnonymous` for an existing account. `RestoreAnonymousSession` reuses the cached token if its session has expired.

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

Sign in first. All save/load requests carry the player session. Binary loads POST a JSON `userId` body with `Accept: application/octet-stream`. HTTP 204 completes with `false` and empty bytes when no save exists.

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

**Saving from several devices.** The plain save methods overwrite whatever is stored. When a player plays on more than one device, load a snapshot with its revision and save against that revision. The server writes only while the save is still at that revision. Otherwise it answers HTTP 409, nothing is written and the result is `Conflict`. The SDK never retries or overwrites a conflict: load again, merge or let the player choose, then save with the new revision.

```cpp
Horizon->CloudSave->LoadSnapshot(FOnCloudSaveSnapshotComplete::CreateLambda(  // or LoadBytesSnapshot
    [Horizon, NewJson](const FHorizonCloudSaveSnapshot& Snapshot)
    {
        if (!Snapshot.bSuccess) { return; }
        // Build NewJson from Snapshot.Data: only the snapshot's data belongs to its revision.
        Horizon->CloudSave->Save(NewJson, Snapshot.Revision,  // or SaveBytes(Bytes, Revision, ...)
            FOnCloudSaveWriteComplete::CreateLambda([](const FHorizonCloudSaveWriteResult& Result)
            {
                if (Result.IsConflict()) { /* another device saved first: load a new snapshot and decide */ }
                else if (Result.IsSaved()) { /* pass Result.Revision to the next save */ }
            }));
    }));
```

`Revision` is `0` for an empty slot. It is `-1` when the server sends no revision, for example a self-hosted simpleServer: the state is then unknown, not empty, and the server does not detect conflicts. Blueprint nodes: **Cloud Load Data Snapshot**, **Cloud Load Bytes Snapshot**, **Cloud Save Data At Revision** and **Cloud Save Bytes At Revision** (pins Saved, Conflict, Failure).

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

Use `PrepareRedemption(Code, callback)` (Blueprint: `PrepareRedemptionCode` / `OnPrepared`) to read a V2 intent. Persist the complete `FHorizonGiftCodeIntent` in a SaveGame before `RedeemPrepared(Intent, callback)` (Blueprint: `RedeemPreparedCode` / `OnRedeemResult`). The feature example includes explicit redeem/retry actions and verified scoped storage. A timeout or restart reuses the exact stored tuple; never prepare or advance automatically.

Campaign UUID, expected count and config revision define the operation; counts/revisions are canonical Int64 decimal strings. Replay returns current Player Values, `bAlreadyApplied/bReplayed=true`, empty grants and no original giftData. Replace balances, never add grants. V1 `Redeem` and arbitrary-ID methods return `UPGRADE_REQUIRED`. `MIGRATION_REQUIRED`, `COUNT_AHEAD`, `CAMPAIGN_CHANGED`, `COUNTER_EXHAUSTED` and other domain errors require reconciliation. Player/project changes fence responses and cannot retarget persisted intents; reusing a visible code cannot redirect the immutable campaign UUID.


### Friends (Develop, TASK-1009)

```cpp
#include "Managers/HorizonFriendsManager.h"
#include "Models/HorizonFriends.h"
Horizon->Friends->GetFriends(UserId,
    FOnFriendsComplete::CreateLambda([](bool Ok, const FHorizonFriendsSnapshot& Snapshot,
        const FString& Code, const FString& Message) { /* Replace the social UI from Snapshot. */ }));
Horizon->Friends->FindPlayer(UserId, ExactUuidOrHCode,
    FOnFriendLookupComplete::CreateLambda([](bool Ok, const FHorizonFriendLookup& Player,
        const FString& Code, const FString& Message) { /* Exact public identity only. */ }));
// Explicit player action after reviewing the current incoming row. Persist this tuple first.
Horizon->Friends->AcceptRequest(UserId, TargetUserId, ReviewedRevision, OriginalRequestId,
    FOnFriendsComplete::CreateLambda([](bool Ok, const FHorizonFriendsSnapshot& Snapshot,
        const FString& Code, const FString& Message) { /* Conflict: explicitly refresh and reconcile. */ }));
```

`SendRequest`, `RemoveFriend`, `BlockPlayer` and `UnblockPlayer` take `(UserId, TargetUserId, ExpectedRevision, FOnFriendsComplete)`. `AcceptRequest`, `DeclineRequest` and `CancelRequest` additionally require the original RequestId immediately before the callback. Revisions are explicit safe integers, never defaults. Native delegates use CreateLambda/CreateUObject, not BindDynamic. Blueprint entry points are RequestFriends, FindFriendPlayer, SendFriendRequest, AcceptFriendRequest, DeclineFriendRequest, CancelFriendRequest, RemoveFriendPlayer, BlockFriendPlayer and UnblockFriendPlayer, with typed OnFriendsResult/OnLookupResult events.

`FHorizonFriendsSnapshot` contains UserId, Revision, FriendCode, Friends, Incoming, Outgoing, Blocked, Limits and ServerTime. Requests expose UserId, RequestId, CreatedAt and Display. Public Display contains Name and Profile.AvatarId/FrameId/Badges; unavailable cosmetics/name become empty placeholders. Friends expose Since, LastActiveAt and bHasLastActiveAt. A null last login remains unknown; it is never online presence. Blocked rows expose no profile/activity. Exact lookup accepts a full UUID or canonical H-code, never name/email searches. Above-tier retained friendships stay visible and removable after downgrade.

`GetCurrentFriends()` returns only the current host/project/player/session context. Cache clears retain accepted revision and server-time floors. HTTP generations and cache epochs fence late responses, including host/project/session A→B→A changes. One operation is active per context; overlapping callers get REQUEST_IN_PROGRESS. Feature errors preserve their codes; authorization loss clears private state. Optional hosted realtime accepts friends:<userId>; HTTP reloads the content. simpleServer supports the REST contract and reports NOT_SUPPORTED for realtime.

`AHorizonFriendsExample` requires an existing restored session and performs no startup mutation, signup, polling or automatic relationship repair. Explicit StartAction persists the project/player/action/target/revision/request-ID tuple before HTTP, validates a new action against the current authorized row or exact lookup, and blocks conflicting pending work. RetrySavedAction sends that same tuple without replacing its revision or request ID. A lost response or conflict requires an explicit LoadFriends and player reconciliation. AcknowledgePendingAfterRefresh only records that user decision; it cannot cancel an older in-flight server action or guarantee its outcome. A later action still needs its own reviewed current revision and request ID. Unblock never restores a friendship/request. Acknowledgement is bound to the exact local intent ID and persisted send serial that were present when refresh began; another actor or later retry invalidates it. Success callbacks also match that fence before deleting a save. The sample never manufactures a fresh revision or accepts a replacement request automatically.

`Tests/run_native_friends_test.py` runs Horizon.Friends.NativeTransport and StrictModels against the strict artificial fixture on 127.0.0.1:18815 after a native plugin build. The fixture checks unchanged original tuples after a lost response and a new incoming request, all seven actions, public lookup privacy, null login, downgrade retention, safe integer tokens and cache/session/project races and two real sample Actor instances sharing one durable intent. It is SDK wiring evidence; live Develop, standalone game recipes and release acceptance are separate gates.

### Achievements (Develop, TASK-1008)

```cpp
#include "Managers/HorizonAchievementsManager.h"
#include "Models/HorizonAchievements.h"
Horizon->Achievements->GetAchievements(UserId,
    FOnAchievementsComplete::CreateLambda([](bool Ok, const FHorizonAchievementsSnapshot& Snapshot,
        const FString& Code, const FString& Message) { /* render Definitions + Progress */ }));
Horizon->Achievements->Unlock(UserId, TEXT("first_win"),
    FOnAchievementUnlockComplete::CreateLambda([](bool Ok, const FHorizonAchievementUnlockResponse& Result,
        const FString& Code, const FString& Message) { /* Result.Achievements/NewlyUnlocked/bReplayed/DeliveryResults */ }));
// Explicit player action; retain/retry these SAME pending keys after an uncertain response.
Horizon->Achievements->DeliverRewards(UserId, PendingKeys,
    FOnAchievementDeliveryComplete::CreateLambda([](bool Ok, const FHorizonAchievementDeliveryResponse& Result,
        const FString& Code, const FString& Message) { /* inspect each Result.Results key/state/code */ }));
```

Callbacks use native delegates (`CreateLambda` / `CreateUObject`, not `BindDynamic`). GetAchievements's optional third argument is the opaque `DefinitionVersion`; it otherwise uses the current player's cache version. Wire `definitions:null` retains only matching materialized metadata; `[]` replaces it. A missing materialization causes at most one explicit full GET, never a POST retry or periodic poll. ClearCache retains revision/server-time watermarks. Session, host/project generations and cache epochs fence late replies; only one operation is active per context, with `REQUEST_IN_PROGRESS` for overlapping callers.

`Progress[].PendingReward.{IssuedAt,Values}` contains the original accepted rewards even if `Definitions[].Rewards.Values` changes. Per-key delivery states are `none`, `pending` and `delivered`; `INBOX_FULL` remains pending. No achievement operation credits player values locally. Delivered `InboxEntryId` links to Inbox; its explicit protected claim adopts the complete balance. Manual unlock retries keep the same key; delivery retries keep one to eight unique original keys. The server owns source identity, with no new client UUID or client reward amounts.

Blueprints expose RequestAchievements, UnlockAchievement and DeliverAchievementRewards with typed result events. `GetCurrentAchievements()` returns the scoped materialized snapshot. Optional hosted realtime accepts `achievements:<userId>`; HTTP supplies content after invalidation/reconnect. REST also works on simpleServer, where realtime is NOT_SUPPORTED.

`AHorizonAchievementsExample` requires an existing restored session. Its Load/Unlock/Deliver methods are explicit UI actions; RefreshAfterStatistics performs one refresh only for a matching successful, non-replayed statistics snapshot with NewlyUnlocked. The sample persists project/player original delivery keys before sending, retains partial/uncertain outcomes across regions/restarts and never replaces unreadable/conflicting pending records. Display original PendingReward values, then load Inbox and bind AHorizonInboxExample's explicit protected Claim to player action. No signup, startup unlock, polling or automatic claim is introduced.

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

Error codes: `SESSION_REQUIRED`, `NO_ACTIVE_RUN` (no started run),
`INVALID_INPUT_LOG_HASH` and `INVALID_CONTENT_DIGEST` (run start context) fail locally without a
request. Server codes: `INITIAL_STATE_INVALID_ENCODING` (400) and `INITIAL_STATE_TOO_LARGE` (413)
for the run start context, `TICKET_INVALID`,
`TICKET_EXPIRED`, `TICKET_FOREIGN`, `TICKET_CONSUMED`, `LEADERBOARD_MISMATCH`,
`STAGE_REQUIRED`, `STAGE_UNKNOWN`, `SCORE_ABOVE_MAX`, `SCORE_BELOW_MIN`,
`STAGE_SCORE_ABOVE_MAX`, `STAGE_SCORE_BELOW_MIN`, `DURATION_TOO_SHORT`,
`SCORE_RATE_TOO_HIGH`, `UNKNOWN_VALUE_KEY`, `DUPLICATE_VALUE_KEY`, `EARNED_ABOVE_MAX`,
`EARNED_BELOW_MIN`, `INSUFFICIENT_BALANCE`, `SCORE_LIMIT_REACHED`, `PLAYER_BANNED`, `SCORE_REQUIRED`, `PLAYER_NAME_REQUIRED`,
`LEADERBOARD_NOT_FOUND`, `PLAYER_NOT_FOUND`, `SESSION_FORBIDDEN`,
`VALIDATED_ACTIONS_UNAVAILABLE`, `RUN_RATE_LIMITED` and `RUN_CAPACITY_REACHED`. The two run
limits are not retried automatically (the wait can be up to an hour). A backend without the
feature (for example a Simple Server) gives `NOT_SUPPORTED`.

#### Run start context and sus runs

`StartRun` takes an optional `FHorizonRunContext`: what the run starts from. Every field is
optional; empty fields are not sent, and an empty context leaves the request as it was.

```cpp
FHorizonRunContext Context;
Context.GameVersion = TEXT("1.4.2");          // at most 64 printable ASCII characters
Context.ContentVersion = TEXT("levels-7");
Context.SimulationVersion = TEXT("sim-3");
Context.ReplayFormatVersion = TEXT("inputs-v1");
Context.ContentDigest = UHorizonValidatedActionsManager::ComputeInputLogHash(LevelBytes); // SHA-256, 64 hex
Context.InitialState = InitialStateBytes;     // TArray<uint8>, sent as base64

Horizon->ValidatedActions->StartRun(TEXT("weekly"), Context, OnStarted);

// Or set it once: StartRun(Key, OnComplete) sends DefaultRunContext (an explicit Context replaces it)
Horizon->ValidatedActions->DefaultRunContext.GameVersion = TEXT("1.4.2");
```

The server binds the context to the run together with what it fixes itself (rule version,
cloud save, server-owned values, seed, start time). A `ContentDigest` that is not 64 hex
characters fails locally with `INVALID_CONTENT_DIGEST`; the server answers
`INITIAL_STATE_TOO_LARGE` (413, above the game's evidence size limit) or
`INITIAL_STATE_INVALID_ENCODING` (400).

`Result.bSus` is true when an accepted run crossed a soft threshold of the rules. The score
counts; the server keeps the run with its start context for a review and asks for the input
log through `Result.Evidence`, which the SDK uploads by itself after `SubmitValidated`, like a
top N record. The reasons stay on the server. Older servers do not send the field (`false`).

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
- **"Start Validated Run With Context"**: Same with a run start context (`FHorizonRunContext`)
- **"Submit Validated Run"**: Hashes the input log and submits the current run (On Success: Result, with `State`)
- **"Submit Validated Run With Hash"**: Same with a ready SHA-256 hash
- **"Get Validated Player State"**: Loads the server-owned values (On Success: State)
- **"Upload Validated Run Evidence"**: Uploads the input log of an accepted run (On Success: Result with `RunId`, `Status`, `Bytes`)
- **"Compute Input Log Hash"**, **"Has Active Run"**, **"Get Last Error Code"**, **"Discard Run"**,
  **"Has State"**, **"Get Balance"**, **"Is Evidence Upload Retryable"**, **"Get Last Evidence Error Code"**,
  the properties **Auto Upload Evidence** and **Default Run Context** and the events **On State Changed**, **On Evidence Uploaded**,
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

**Limit**: counted per account, shared by all players of all your games. On the Free plan the soft limit is 10 and the hard limit 100 requests per minute. Paid plans have higher limits, see [horizon.pm/pricing](https://horizon.pm/pricing). Requests above the hard limit receive HTTP 429 and are automatically retried after the cooldown period.

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
| 429 | Rate Limited | Code-free global limits retry after the server wait plus 0–20% jitter; feature codes return immediately |

### Retry policy

All JSON, binary-upload and JSON-to-binary response paths share one dispatcher. A 429 with any nonempty string `code` is a feature decision and reaches the caller immediately, including unknown future codes. Missing, null or empty codes use the global policy: wait at least `Retry-After` seconds (minimum one second), then add random delay up to 20%, bounded by `MaxRetryAttempts`. Native timers reject NaN/infinite waits and avoid float overflow. Status, machine-readable code, response body and headers remain available after the final attempt.

Requests capture their original JSON/bytes, extra headers and host/project/session context. Host/project lifecycle changes cancel old responses and scheduled retries with `SESSION_CHANGED`, including A→B→A changes. Session-bound calls additionally fence token-generation changes; session-independent calls remain valid during token changes. Backoff uses monotone wall time, so accelerated or paused game time cannot move a retry before the server minimum. Signup always has one transport attempt. Gift V2 retries preserve the original immutable campaign/count/revision tuple. V1 arbitrary-ID calls are closed and never authorize a replacement operation.

`Horizon.Transport.RateLimit.NativeHTTP` uses the strict artificial `Tests/mock_rate_limit_server.py` on `127.0.0.1:18817`. `Tests/run_native_rate_limit_test.py --engine /path/to/UE_5.8 --host-project /path/to/HostProject.uproject --report /path/to/report` runs the native test after building the plugin. It verifies all three data paths, minimum waits, feature-code passthrough, retry exhaustion, immutable payloads/headers, single-attempt legacy/signup and a session change during backoff.

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

## Transport checks

Run `bash Tests/run_transport_test.sh` from the repository root. The standalone tests include selected production auth, user-model, binary-load and HTTP helper methods compiled against in-memory engine boundaries. They cover server-issued token signup, direct signup sessions, failed and expired session recovery, returning accounts, binary headers/body, 204 and 401. A full plugin build still requires an Unreal Engine 5.5+ project.

## Documentation

- **[Quickstart Guide](https://horizon.pm/quickstart#unreal)** - Interactive setup
- **[Plugin README](Plugins/HorizonSDK/README.md)** - Plugin-specific documentation
- **[horizOn Docs](https://horizon.pm/docs)** - Full API documentation

## Support

- 📖 **Documentation**: [horizon.pm/quickstart](https://horizon.pm/quickstart)
- 💬 **Discord**: [discord.gg/JFmaXtguku](https://discord.gg/JFmaXtguku)
- 🐛 **Issues**: [GitHub Issues](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues)

## License

MIT License - Copyright (c) [ProjectMakers](https://projectmakers.de)

See [LICENSE](LICENSE) for details.

## Realtime invalidations (Cloud)

After sign-in, use the subsystem's `Realtime->Subscribe("inbox:" + Subsystem->Auth->GetCurrentUser().UserId)`. Blueprint exposes the same manager, subscription methods and `OnResyncRequired`, `OnEvent`, `OnPollingRequired`, `OnConnectionError` delegates. Reload authorized content through the feature HTTP API after events and resync signals. Respect `PollAfterSeconds` during HTTP fallback; chat never polls. Unsubscribe when no longer needed.

Native WebSockets reconnect with jitter and reauthorize topics; events contain IDs and revisions only. Sign-out clears subscriptions, same-user token refresh reconnects. simpleServer reports `NOT_SUPPORTED`; realtime is Cloud only. The local native automation test `Horizon.Realtime.NativeTransport` runs against the Server repository's artificial SDK fixture when `HORIZON_REALTIME_FIXTURE_URL` is set to loopback.

## Player values

`UHorizonSubsystem::PlayerValues` exposes `GetState(UserId, FOnPlayerValuesComplete)`, `GetOffers(UserId, FOnPlayerValuesOffersComplete)`, `Purchase(UserId,OfferKey,OperationId,Delegate)`, `Adjust(UserId,Key,int64Amount,Reason,OperationId,Delegate)` and `CreateOperationId()`. Include `Managers/HorizonPlayerValuesManager.h`; it includes the reflected models in `Models/HorizonPlayerValues.h`. Completion delegates return `(bool bSuccess, const StateOrOffers&, const FString& ErrorCode, const FString& ErrorMessage)`. `FHorizonPlayerValuesState.bPresent` indicates whether current balances accompanied a success or business failure. Values/costs/applied deltas use int64. Nullable caps and offer day limits use explicit `bHasDailyCap`/`bHasPurchasesPerDay` flags, so zero stays distinct from null.

Catalog and balance lists follow the server's current configuration; the SDK does not impose the old tariff counts. A lowered daily cap does not erase an existing balance or earned-today value. Removed/omitted offer day quotas leave `bHasPurchasesPerDay=false`.

A fresh purchase may return immediate `Operation.Applied` and `Operation.Items`. A compact replay returns the current full state, `Operation.bReplayed=true` and empty deltas/item references. Replace displayed balances and refresh Inventory when needed; never add replay grants or reconstruct discarded items locally.

Blueprints use `RequestState`, `RequestOffers`, `PurchaseOffer`, `AdjustValue` and receive `OnValuesResult`/`OnOffersResult`. `CreateOperationId`, `GetCurrentState`, `GetCurrentOffers`, `HasServerClock`, `ClearCache` are reflected. State is bound to the matching signed-in session/project/host; stale responses cannot repopulate a cleared cache. Revision/server-clock watermarks survive catalogue invalidation.

Load state/offers before creating an operation ID to learn server time. IDs then advance using monotonic elapsed time; local UTC is the visible fallback (`HasServerClock()==false`). Persist one operation ID and immutable payload per logical action and retain it after a timeout. Never create a replacement automatically after `OPERATION_EXPIRED`; refresh and reconcile first. Integers are bounded to +/-9,007,199,254,740,991. Realtime remains optional and does not replace HTTP state refresh after validated runs or another balance mutation. No automatic purchase/poll loop exists.

## Inbox

```cpp
#include "HorizonSubsystem.h"
#include "Managers/HorizonInboxManager.h"
#include "Models/HorizonInbox.h"

// Use an already restored player session. Loading never claims attachments.
const FString UserId = Horizon->Auth->GetCurrentUser().UserId;
Horizon->Inbox->GetInbox(UserId,
    FOnInboxComplete::CreateLambda([](bool Ok, const FHorizonInboxSnapshot& Inbox,
                                     const FString& Code, const FString& Message)
    {
        if (Ok) { /* Render Entries, Counts and Limits. Retain Cursor for a conditional refresh. */ }
    }));
// Only on an explicit player action, using the original persisted entry ID:
Horizon->Inbox->Claim(UserId, OriginalEntryId,
    FOnInboxClaimComplete::CreateLambda([](bool Ok, const FHorizonInboxClaim& Claim,
                                          const FString& Code, const FString& Message)
    {
        if (Ok) { /* Replace UI with Claim.Inbox; PlayerValues already adopted Claim.PlayerValues. */ }
        // An uncertain result retains OriginalEntryId for the next explicit retry.
    }));
```

`MarkRead(UserId, EntryId, FOnInboxComplete)` and `Remove(...)` return full snapshots.
`GetInbox(UserId, Callback, Since)` accepts an opaque cursor; an unchanged reply is materialized from the matching cache or triggers one full reload. Concurrent refresh bursts are coalesced into one active request and one queued request. Cache clears preserve revision and server-time watermarks; session, project, region and epoch changes fence late replies. Claims also capture the PlayerValues cache epoch. Balances are replaced from the full authorized server state, never credited locally.

Blueprint entry points are `RequestInbox`, `MarkEntryRead`, `RemoveEntry` and `ClaimEntry`; their result events expose the same typed models. The explicit `AHorizonInboxExample` actor preserves a pending claim before HTTP, scoped to the project and player, and blocks a different entry until the original is reconciled. New claims require an unclaimed valuable entry in the current authorized snapshot. `GetPendingClaimEntryId` recovers the original entry after a restart; `GetCurrentInbox` always reads the manager cache, so logout never leaves a second actor snapshot. It performs no automatic signup, polling or claims.

Realtime is optional and Cloud-only. Subscribe to `inbox:<userId>` and, when its ID is supplied by your project setup, `inbox:project.<apiKeyId>`; the API-key ID is distinct from the secret API-key value. Invalidation/resync events trigger a coalesced HTTP refresh. Respect `PollAfterSeconds` for a scheduled fallback; chat has no polling fallback. Inbox REST also works with simpleServer; its realtime channel returns `NOT_SUPPORTED`.

Native `Horizon.Inbox.NativeTransport` uses `Tests/mock_inbox_server.py` with `HORIZON_INBOX_FIXTURE_URL=http://127.0.0.1:18816`. It checks original-entry replay, full balance adoption, bounded refresh bursts, stale cache/session responses and exactly one signup request after HTTP 503 or a dropped connection. `Horizon.Inbox.StrictModels` checks safe integer tokens and malformed rewards/snapshots.

## Player statistics

Configure `kills` with mode `sum` and `best_score` with mode `max` in the project's statistics catalog. Modes are `sum`, `max`, `min` and `last`; optional resets use UTC days or ISO weeks. Each player has one state document. Initialization and sign-in never load statistics automatically.

```cpp
#include "Managers/HorizonStatisticsManager.h"

Horizon->Statistics->GetStatistics(FOnStatisticsLoaded::CreateLambda(
    [Horizon](bool Success, const FHorizonStatisticsSnapshot& Snapshot, const FString& Code, const FString& Message)
    {
        if (!Success) return;
        FHorizonStatisticUpdate Kills; Kills.Key = TEXT("kills"); Kills.Value = 3;
        const FString OperationId = Horizon->Statistics->CreateOperationId();
        // Persist OperationId + the original batch before sending; keep them after a timeout.
        Horizon->Statistics->UpdateStatistics({Kills}, OperationId, FOnStatisticsLoaded::CreateLambda(
            [](bool Applied, const FHorizonStatisticsSnapshot& Result, const FString& ErrorCode, const FString& ErrorMessage) {}));
    }));
```

`GetDefinitions` loads the catalog explicitly. Snapshots cache definitions by `definitionVersion`, including conditional responses, and carry `Revision`, `Values`, `OperationWindow`, `OperationId`, `bReplayed` and reserved `NewlyUnlocked`. `GetCurrentStatistics`, `GetCurrentDefinitions` and `ClearCache` manage the local cache. Session/host/project changes clear it; late responses cannot restore old data. Revision and server-time fences survive a catalog refresh, preventing older counters or UTC projections from replacing newer state.

Blueprint provides **Get Statistics**, **Get Statistics Definitions**, **Update Statistics** async nodes and **Create Operation Id** on the manager. Updates use Blueprint `int64`, bounded to ±9,007,199,254,740,991. Empty batches, duplicate or malformed keys, unsafe values and missing `<13-digit Unix milliseconds>:<UUID>` IDs fail locally. The server validates per-update bounds and rejects a whole batch on overflow.

An uncertain request must use its original ID and unchanged payload on retry; the SDK never generates a replacement ID. The server replays a retained receipt first, rejects changed payloads with `OPERATION_CONFLICT`, and rejects expired/evicted unknown IDs with `OPERATION_EXPIRED`. Only eight receipts are retained; the persisted eviction floor can reject out-of-order delayed IDs before the 24-hour window ends. Snapshot/server operation time helps create new IDs within the allowed 30-second clock skew. Never create a fresh ID to retry an unknown outcome.

C++ delegates and Blueprint failure pins expose server error codes. Other stable codes include `SESSION_REQUIRED`, `SESSION_CHANGED`, `STATE_CONFLICT`, `INVALID_REQUEST`, `INVALID_RESPONSE` and the HTTP transport's `CONNECTION_FAILED`/`RATE_LIMITED`. Cloud and simpleServer use the same statistics REST contract; realtime is optional and unrelated to these explicit batches.

`AHorizonStatisticsExample` restores an anonymous player, explicitly loads a snapshot and persists a project/player-scoped pending batch before posting it. It keeps the original intent on errors or an uncertain timeout. Add it to a level to try the flow. Native fixture test `Horizon.SDK.Statistics.NativeTransport` uses `Tests/mock_statistics_server.py` at the fixed loopback URL in `HORIZON_STATISTICS_FIXTURE_URL`; `bash Tests/run_transport_test.sh` checks the shared C++ contract without Unreal.


### Example identity recovery

`AHorizonStatisticsExample` uses `UHorizonExampleIdentity` for its example-only local recovery. Before signup it saves a project-bound uncertainty marker; an unknown result never triggers another signup on recreation. Unvalidated legacy caches stay unbound until successful authentication. An explicit guest restore selects that project's protected guest ahead of a cached email account. Previously authenticated foreign-project credentials are backed up before a new project signs up.

Games reusing this helper pass the optional fourth `SignUpAnonymous` argument `FOnAnonymousIdentityIssued`. It receives the immutable issuing API key and actual server-issued `FHorizonUserData` before follow-up sign-in; return false if durable preservation fails. Existing calls and the deprecated third argument remain compatible. The SDK retains its issued credential cache even when this observer rejects continuation. A changed shared disk cache is never evidence that a signup issued those credentials. `ClearSession` clears in-memory authentication and retains the disk cache.

The sample runs once per actor, gates auth re-entry, keeps original pending statistics on failure, validates a saved batch before replay, and removes only the saved batch matching its successful response. `Tests/run_native_identity_test.py` runs `Horizon.Identity.NativeRecovery` against its strict artificial loopback fixture on port 18812. This is a local engine/HTTP proof, without real accounts or Cloud deployment.

Groups: `Groups` manager, typed views, explicit revision/request IDs, one-attempt writes and
lossless JSON metadata. See [the plugin quickstart](Plugins/HorizonSDK/Docs/QUICKSTART.md#groups-task-1010)
and `AHorizonGroupsExample`; no automatic signup, polling or replacement intent is performed.

Items: `Items` exposes five typed inventory/catalog methods and Blueprint async
nodes. Property integers, Unicode/NUL strings, purchase item rewards and retained
instance data stay lossless. `AHorizonItemsExample` persists original actions
and never replaces an uncertain purchase ID. See [the quickstart](Plugins/HorizonSDK/Docs/QUICKSTART.md#items-task-1011)
for single-attempt CAS writes, explicit receipt recovery and the remaining GET-to-POST price window.

Item limits are current positive Int32 operator configuration, with no old 5000/150/50 tariff
clamps. A lower current limit does not hide retained inventory or catalog totals. Catalog total
has the storage-format ceiling of 65,536 permanent UInt16 template slots; individual pages remain
at most 50 definitions. Compact storage preserves the public instance properties, CAS revisions
and original property deltas. Replayed purchases return current state with empty applied/item
references; the sample refreshes inventory without synthesizing a new instance or operation ID.


Shared Objects: `SharedObjects` exposes seven typed methods, opaque byte payloads,
strict search scalars, explicit original If-Match and Blueprint async nodes.
`AHorizonSharedObjectsExample` durably preserves original intents across regional
switches and reconciles uncertain creates through owner reads. See
[the quickstart](Plugins/HorizonSDK/Docs/QUICKSTART.md#shared-objects-task-1012)
for world-map queries and the separate existing Validated Actions defense recipe.


### Asynchronous Matches

`UHorizonSubsystem::Matches` exposes all14 app routes through the typed
`UHorizonMatchesManager`. Include `Managers/HorizonMatchesManager.h` and
`Models/HorizonMatches.h`; native callbacks use four parameters
`(bool, const ResultType&, const FString& ErrorCode, const FString& ErrorMessage)`
and `CreateUObject`/`BindUObject`. Blueprint async nodes in
`AsyncActions/HorizonAsync_Matches.h` freeze the current world/session context.

Read with `GetMatches`, `GetLimits`, `FindOpenMatches` (one opaque cursor page),
and `GetMatch`. `CreateMatch` takes the original player-ledger revision;
`JoinMatch`, `DeclineInvite`, `StartMatch`, `SubmitTurn`, `AdvanceTimeout`,
`LeaveMatch` and `CancelMatch` take the original match revision. `ReportResult`
and `StartRankedRun` take the frozen final-state revision. Every Matches mutation
makes exactly one transport attempt. An uncertain response requires an explicit
read/reconciliation; the SDK never substitutes a newer revision or move.

State and result payloads are `TArray<uint8>`, with strict canonical Base64 on
the wire. `bHasData` distinguishes invite-only metadata (`null`) from accepted
empty state. Safe integer revisions and strength, false, nullable deadlines,
Unicode and embedded NUL names are preserved. Snapshots retained after a
subscription downgrade remain readable up to the server's maximum shape.
Realtime `matches:<UUID>` events only invalidate caches. There is no background
polling, automatic cursor traversal, turn advancement or inferred game winner.

`AHorizonMatchesExample` is an executable Actor for each of two existing signed-in
clients. It has no automatic auth or Tick work. Call `Refresh()` for the ledger,
`Refresh(id)` for the selected match, and explicit `StartAction` calls for lobby,
turn, timeout, result and ranked actions. Poll intervals apply to refresh;
`FindOpen` permits explicit cursor continuation. The game determines its own
combat and must explicitly confirm the same final bytes/winner IDs.

The Actor persists one immutable original under project+player identity. Hosts
and sessions only fence requests, so switching region cannot create another
pending slot. Two validated SaveGame generations preserve the last complete
original if an interrupted write damages the other generation. Create names
carry a unique sample recovery nonce; the supplied opaque state is unchanged.
After a lost create, `RetryOriginal` reads the ledger, then the player explicitly
reads and calls `ConfirmRecoveredCreate(id)`. A definitely rejected first create
can be deliberately acknowledged after a fresh ledger. A lost departure or
known match purged after retention can be acknowledged after a fresh ledger no
longer references it and the original point read authoritatively returns
`MATCH_NOT_FOUND`. A genuinely unknown
create with no discoverable match remains blocked and requires reconciliation;
absence in a response cannot prove that an older request will never commit.

The separate ranked recipe adopts the original confirmed-match run into
`ValidatedActions`; it does not prove honest gameplay or automatically grant a
match reward. The sample freezes score/input before `SubmitRankedResult`, keeps
uncertain submissions across restarts, and validates the accepted run/board
receipt before clearing them. Expired/consumed runs never mint replacement runs.
`simpleServer` ordinary Matches REST remains usable; ranked is explicitly
`FEATURE_NOT_SUPPORTED` and no cloud validation is simulated.

`Tests/run_native_matches_test.py` runs `Horizon.Matches.StrictModels` and
`Horizon.Matches.NativeTransport` against artificial loopback origins18820/18821.
The strict fixture checks exact requests for two independent players, all14
routes, original revisions/bytes, lost responses, region change, ranked recovery,
Blueprint context, cache/realtime invalidation and one-attempt failures. Its
simulated turn timeline advances only the test Actor's refresh clock instead
of waiting120 real seconds per move. Native Editor/Development/Shipping builds
are separate evidence from live Develop, standalone games and Hello integration.


Generic `ValidatedActions.StartRun` and Matches ranked starts share one exclusive
in-flight start lease. A competing start returns local `RUN_IN_PROGRESS` before
HTTP. Cache clearing, signout and region/session changes do not release another
pending request's lease. Release follows the original completion callback;
late context responses cannot adopt a run. An active unrelated run blocks a
match start, while the same original match/final-state run can be recovered.
An already adopted active match run is protected from a generic replacement
until submit or the player's explicit `DiscardRun` action.


### Hosted Chat (TASK-1014)

`UHorizonSubsystem::Chat` exposes typed `GetLimits`, `OpenChannel`, `SyncChannel`,
`SendMessage`, `GetOperation`, and `ReportMessage`. Include
`Managers/HorizonChatManager.h` and `Models/HorizonChat.h`. Native callbacks have
four arguments `(bool, const FHorizonChat…&, const FString& code, const FString& message)`;
use `BindUObject`/`CreateLambda`. Blueprint actions and result delegates expose the
same six envelopes. Optional dates use explicit `bHas…` flags; integers are parsed
from their original lexemes. Text is plain Unicode (500 scalars/2000 UTF-8 bytes),
with LF/TAB allowed and other ISO controls rejected. No markup interpretation.

All six methods reach REST, including an explicit `SyncChannel` without realtime.
An unwatched/disconnected explicit read requests a full snapshot and does not expose
a managed feed. simpleServer `501 NOT_SUPPORTED` and structured server errors are
returned unchanged. `GetLastResponseMetadata()` exposes status, structured code and
original headers inside the completion callback, without retaining message payloads.
Open, send and report each have one transport attempt; sync is
a bounded read-only POST with frozen JSON. A rate response never schedules a send.

For an active feed call `WatchChannel(userId, channelId)`. The existing realtime
manager obtains an authorized subscription ACK before requesting content. Listen
to `OnFeedChanged(channelId)` and render `GetFeed(channelId).Messages` only when
`bPresent` is true. `IsChannelSubscribed` exposes subscription state. The cache
atomically composes the full visible ID order using immutable cached/returned text;
missing, changed, duplicate or cross-channel payloads cannot produce partial success.
One request per channel plus a dirty bit coalesces event bursts. There is no HTTP
polling or disconnected fallback. Disconnect, permission loss, local expiry and
host/session/project ABA hide content; a fresh authorized subscription and sync are
required. `UnwatchChannel` releases the selected feed. At most32 watches are retained.

Drop `AHorizonChatExample` into a map with an existing player session and call
`ShowUI()`. Its minimal UMG interface offers direct-friend/group/project selection,
subscription status, a plain `UTextBlock` feed, explicit send/reconcile/retry, and
report reason. The Actor never signs up a player or embeds admin credentials.

The sample journals an exact original operation ID, millisecond UTC issued-at,
channel and UTF-8 text **before** its first send. Namespace is project+player+chat;
regional host/session are response fences only. A per-scope OS-exclusive lock protects
read-modify-write, and two verified generations atomically retain pending and
acknowledged originals across actor/process restarts. A bounded version/Base64/SHA256
outer envelope is verified before Unreal deserializes any SaveGame bytes. Desktop uses Unreal's native
system-wide lock; iOS/Android use POSIX file locking because their Unreal system-wide
mutex stub is fatal. A damaged journal fails closed. Missing operation receipts do
not prove a missing write: recovery first reads `GetOperation`, then an explicit
retry may send only the identical saved original. An expired operation remains an
explicit unresolved journal and blocks fresh IDs. Reports retain their original
channel/message/reason. Server reports deduplicate only while the shared report exists. Acknowledged
receipts remain in the bounded journal until a new explicitly prepared original
replaces them. Save files contain no session token. Persisting a journal is local
crash recovery, not a new server price/value guarantee.

Verification: `bash Tests/run_transport_test.sh`; after building a dedicated local
HostProject, run `python3 Tests/run_native_chat_test.py --engine <existing-UE-root>
--host-project <HostProject.uproject> --report <isolated-report-directory>`. The latter
starts only artificial loopback HTTP18822/18823 and WebSocket18824, requires readiness,
and checks native automation plus the fixture's invariants. Do not run concurrently
with another instance on these ports. Native fixture evidence is separate from real
Develop/production or official game/Hello acceptance; mobile lock source support is
separate from a mobile package build.


### Device Chat history

Hosted Chat is available in every tier, including Free. The server holds at most
20 project/global messages for seven days, or 15 direct/group transport messages
for 30 seconds. Private buffers do not promise offline delivery. `knownMessageIds`
contains at most 20 IDs from the live window, never IDs from the device archive.
A new operation must be later than the player's accepted issued-at floor and
server time minus 30 seconds, with at most 60 seconds future tolerance. Operation
lookup/replay uses the original message while it remains buffered. There is no
personal server receipt history. Missing operations remain uncertain. No send is
retried automatically, including after a local storage failure.

`Chat->ReadLocalHistory(channelId, cursor, limit)` returns
`FHorizonChatHistoryPage` with `bSuccess`, `Messages`, `NextCursor`, and `ErrorCode`.
`ClearLocalHistory(channelId)` and `ClearAllLocalHistory()` erase only the current
identity's device copies. Blueprint exposes the same methods, `OnLocalHistoryChanged`
and `OnLocalHistoryFailed`. The UMG sample has separate live/device views and explicit
read/older/clear controls, including a channel UUID input for offline reads.

The archive stores received immutable text and confirmed original send input. Its
scope hashes the API key and player, with one append-only UTF-8 JSONL log and one
append-only ID/hash/offset index per channel. No key or session token is stored in
plaintext. Only the small ID index is loaded on first use, then its appended tail.
Text reads are bounded per record (16 KiB) and page (at most 100 messages). Byte
cursors follow local delivery order, newest delivery first within every page.
An interrupted complete log append can repair its missing final index entry.
Damaged/locked/unwritable storage reports a local error and never resends a message.

Server eviction, expiry, moderation, epoch changes and revoked membership retain
already delivered device copies. The active feed still hides immediately. Same-identity
authenticated offline reads are allowed. Logout and project/player changes remove
access in memory. Captured request identity and persistent clear generations fence
late replies. `ClearCache()` clears the live feed and pending reads; an already accepted
send in the same authenticated context still completes and archives its original text.
Clearing local history invalidates prior page cursors. A new device has no old archive,
and the SDK neither uploads it nor uses Cloud Save. There is no implicit local cap.
