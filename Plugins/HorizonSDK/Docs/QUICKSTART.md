# horizOn SDK -- Quick Start Guide

Get your Unreal Engine project connected to horizOn in 5 minutes.

## 1. Copy the Plugin

Copy the `HorizonSDK` folder into your project's `Plugins/` directory:

```
YourProject/
  Plugins/
    HorizonSDK/          <-- here
      HorizonSDK.uplugin
      Source/
      ...
```

## 2. Enable in Editor

1. Open your project in Unreal Editor.
2. Go to **Edit > Plugins**.
3. Search for **"horizOn SDK"** and check the **Enabled** box.
4. Restart the editor when prompted.

## 3. Import Your Config

1. Download your config JSON from the horizOn dashboard.
2. In Unreal Editor, go to **Tools > horizOn > Import Config...**
3. Select the JSON file. The API key and backend hosts will be applied automatically.

Alternatively, open **Project Settings > Plugins > horizOn SDK** and enter the values manually.

## 4. Connect and Authenticate

### C++

Add `"HorizonSDK"` to your module's `Build.cs` dependencies, then:

```cpp
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"

void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    UHorizonSubsystem* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();

    // Step 1: Connect
    Horizon->OnConnected.AddDynamic(this, &AMyGameMode::OnHorizonConnected);
    Horizon->ConnectToServer();
}

void AMyGameMode::OnHorizonConnected()
{
    UHorizonSubsystem* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();

    // Step 2: Authenticate
    Horizon->Auth->SignUpAnonymous(TEXT("Player1"),
        FOnAuthComplete::CreateLambda([](bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Signed in!"));
            }
        }));
}
```

### Blueprints

1. Place a **"Connect to horizOn Server"** async node.
2. From the **On Success** pin, place a **"Sign Up Anonymous"** node.
3. You are now connected and authenticated.

## 5. Use Features

Once connected and signed in, call any manager method:

| Feature       | C++ Manager                   | Blueprint Node             |
|---------------|-------------------------------|----------------------------|
| Cloud Save    | `Horizon->CloudSave->Save()`  | Cloud Save Data            |
| Cloud Load    | `Horizon->CloudSave->Load()`  | Cloud Load Data            |
| Leaderboard   | `Horizon->Leaderboard->SubmitScore()` | Submit Leaderboard Score |
| Remote Config | `Horizon->RemoteConfig->GetAllConfigs()` | Get All Remote Configs |
| News          | `Horizon->News->LoadNews()`   | Load News                  |
| Gift Codes    | `PrepareRedemption()` then persist / `RedeemPrepared()` | PrepareRedemptionCode / RedeemPreparedCode           |
| Player Profile | `Horizon->PlayerProfile->GetProfile()` / `SetProfile()` | Get Player Profile / Set Player Profile |
| Validated Actions | `Horizon->ValidatedActions->StartRun()` / `SubmitValidated()` / `GetState()` / `UploadEvidence()` | Start Validated Run / Submit Validated Run / Get Validated Player State / Upload Validated Run Evidence |
| Feedback      | `Horizon->Feedback->Submit()` | Submit Feedback            |
| User Logs     | `Horizon->UserLogs->Info()`   | none (C++ only)            |

All async operations use callbacks (C++) or output execution pins (Blueprints) for success and failure handling.

## Troubleshooting

- **"HttpClient not initialized"** -- Make sure you call `ConnectToServer()` before any other operation.
- **Authentication required** -- Cloud Save, Leaderboard, Gift Codes, Player Profile, Validated Actions, and User Logs require the player to be signed in first.
- **`PLAYER_BANNED`** -- The player is banned from that leaderboard. `SubmitScore` reports it through `GetLastSubmitErrorCode()`, `SubmitValidated` through its error code (the run stays, `DiscardRun()` drops it).
- **Evidence upload** -- `SubmitValidated` uploads the input log by itself when the result asks for it (`Result.Evidence.bRequired`). After `SubmitValidatedWithHash` call `UploadEvidence(Result.Evidence.RunId, InputLog)` yourself. Results arrive on `OnEvidenceUploaded` / `OnEvidenceUploadFailed`.
- **Run start context and sus runs**: `StartRun(Key, Context, OnComplete)` (Blueprint: Start Validated Run With Context) declares versions, content digest and initial state (`FHorizonRunContext`); `DefaultRunContext` is sent when no context is passed. `Result.bSus` marks an accepted run the server keeps for a review; its input log is uploaded like a top N record.
- **Multiple hosts** -- If you configure multiple backend hosts, the SDK automatically pings each one and selects the fastest.
- **Session caching** -- The SDK saves session tokens to disk. Call `Horizon->Auth->RestoreAnonymousSession()` (Blueprint: **"Restore Anonymous Session"**) to sign a returning anonymous player back in.
- **Clear cache** -- Use **Tools > horizOn > Clear Session Cache** in the editor to wipe local session data.

### Groups (TASK-1010)

Include `Managers/HorizonGroupsManager.h` and `Models/HorizonGroups.h`; the existing
subsystem exposes `SDK->Groups`. A valid player session is required. The manager never
creates a replacement guest. Native callbacks have `(bool, const TypedView&, const FString& code,
const FString& message)` and bind with `BindUObject` / `CreateWeakLambda`.

```cpp
SDK->Groups->GetGroups(UserId, FOnGroupsComplete::CreateWeakLambda(this,
    [this](bool Ok, const FHorizonMyGroupsView& State, const FString& Code, const FString& Message)
    {
        // Display State.Groups / IncomingInvites / OutgoingRequests and retain State.Revision.
    }));
FHorizonGroupMetadata Metadata;
Metadata.Name = TEXT("My team");
Metadata.JoinMode = TEXT("request");
FHorizonGroupJsonData::TryFromJson(TEXT(R"({"motd":"Welcome","large":9007199254740993})"), Metadata.Data);
// On an explicit player action: persist the original revision and full metadata BEFORE CreateGroup.
// SDK->Groups->CreateGroup(UserId, SavedOriginalPlayerRevision, Metadata, Callback);
```

`FHorizonGroupJsonData` is immutable. `TryFromJson(Raw, Out)` validates an object,
finite numbers with a separate depth 1024 technical parser bound. Name/description/data bytes
and data depth are current Admin System Config policy; depth 16 / 2048 bytes are defaults, not
reader ceilings. Aggregate responses are parsed against their actual received UTF-8 budget. `GetJson()` preserves number
lexemes, empty/dot/dollar keys, Unicode and null; use this original text when editing data.
Do not round-trip it through a `double` map. Blueprint uses `ParseGroupData` / `GroupDataJson`.

All 16 writes send one transport attempt. Revision/request identity is mandatory;
there is no operation receipt. After a timeout retain the original tuple, explicitly
refresh and reconcile, and only explicitly retry that same tuple. Never substitute the
new revision or a new invitation ID. Feature codes are returned unchanged. `GetMembers`
returns one page (1..current `limits.memberPageSize`, default 50); callers explicitly request
the next cursor and check revision
changes between pages. No automatic full-list fetch or polling runs. Private snapshots
are cleared on session/project/host/cache invalidation and authorization failure.
Revision/time watermarks survive `ClearCache` for every observed group in the current
authentication context. There is no 256-group admission ceiling; an authentication change
resets these floors.

`AHorizonGroupsExample` is a runnable two-player workflow helper: use separate authenticated
clients, explicitly `Refresh(GroupId)`, then `StartAction(...)` for create/update/join/request/
invite/accept/decline/cancel/leave/remove/role/transfer/delete. It persists the full original
project/player/action/revision/request/metadata tuple and send serial before HTTP. An
uncertain result blocks a new intent. `RetrySavedAction()` keeps the original; explicit
`AcknowledgePendingAfterRefresh()` removes only the exact original that was refreshed.
This acknowledgement does not cancel a possibly running server operation. Nothing signs
in, mutates or repeats on actor startup. The optional Cloud topic `groups:{groupId}` is an
invalidation hint: reload authorized HTTP state. simpleServer realtime is `NOT_SUPPORTED`.

Local proof commands (existing UE engine / independently built HostProject):

```sh
bash Tests/run_transport_test.sh
python3 Tests/run_native_groups_test.py --engine /path/to/UE --host-project /path/to/HostProject.uproject --report /tmp/groups-report
```

The native fixture binds only `127.0.0.1:18810`, uses artificial identities, checks all
20 routes and rejects extra retries/polls. This is SDK/loopback evidence; shared Develop
and public Cloud behavior require their own acceptance run.

## Items (TASK-1011)

Use an existing player session and `UHorizonSubsystem::Items`. Include
`Managers/HorizonItemsManager.h` and `Models/HorizonItems.h`.

```cpp
const FString UserId = Horizon->Auth->GetCurrentUser().UserId;
Horizon->Items->GetInventory(UserId, FOnInventoryComplete::CreateLambda(
    [](bool Ok, const FHorizonInventorySnapshot& Inventory,
       const FString& Code, const FString& Message) {
        if (!Ok) return;
        // Render the complete authoritative snapshot. No local grant or delta addition.
    }));
// Only on an explicit player action, using the instance/revision the player reviewed:
Horizon->Items->SetEquipped(UserId, OriginalInstanceId, OriginalRevision, true,
    FOnInventoryComplete::CreateLambda([](bool Ok, const FHorizonInventorySnapshot& Inventory,
                                         const FString& Code, const FString& Message) {}));
```

The five methods are `GetInventory(UserId, Callback)`,
`GetCatalog(UserId, Callback, AfterKey="", Limit=50, CatalogRevision=-1)`,
`AdjustProperties(UserId, InstanceId, ExpectedRevision, Changes, Callback)`,
`SetEquipped(UserId, InstanceId, ExpectedRevision, Equipped, Callback)` and
`DiscardItem(UserId, InstanceId, ExpectedRevision, Callback)`.
`FOnInventoryComplete` and `FOnItemCatalogComplete` return
`(bool, const TypedResult&, const FString& ErrorCode, const FString& ErrorMessage)`.
Use native `BindUObject`/`CreateUObject`, not `BindDynamic`, for those callbacks.
A cursor continuation always requires its original catalog revision. Each call
loads one bounded page; there is no automatic page fetch, mutation, grant or poll.

`FHorizonItemProperty` is a typed array entry (`Key`, `Type`, `IntegerValue`,
`BooleanValue`, `StringValue`). Types are `EHorizonItemPropertyType::Integer`,
`Boolean`, `String`. Integers stay in int64 within ±9,007,199,254,740,991; strings
retain Unicode, embedded NUL and their suffix. JSON null, fractional/unsafe
numbers, duplicate/non-ASCII property keys and nested values are rejected.
A property object has at most 16 keys and 256 canonical UTF-8 bytes. Properties
are parsed from original JSON before Unreal's shared-key DOM can alter them.
Archived definitions and retained inventories above a downgraded limit remain
readable. Instances and purchase references never imply that an archived or
already discarded/transferred item should be recreated.

All three inventory writes make one transport attempt. Persist the exact
instance, revision and mutation payload before sending. On uncertainty, retain
that tuple; never silently substitute a newer revision. `ClearCache` invalidates
late replies while retaining revision/time floors. Session, project and host
context changes also fence same-value ABA transitions. Blueprint manager
request methods/events expose the same DTOs. Async nodes in
`AsyncActions/HorizonAsync_Items.h` freeze caller arguments and request context,
and activate once: `GetInventory`, `GetCatalog`, `AdjustProperties`,
`SetEquipped`, `DiscardItem`.

Player-value offers add `FHorizonPlayerValueOffer::ItemRewards`
(`ItemKey`, `Quantity`); operation receipts add
`FHorizonPlayerValuesState::Operation.Items` (`InstanceId`, `ItemKey`). Missing
fields in old responses mean empty arrays. Existing purchase IDs and callbacks
remain compatible. A successful receipt must match the submitted operation ID.
Use the full returned balances and explicitly read inventory afterward. Never
add item references to local inventory or rebuy because that read failed.

`AHorizonItemsExample` is an executable actor for an already restored player:
call `Refresh`, then bind `PurchaseItemOffer` or `StartItemAction` to explicit UI
actions. Its one project/player-bound save records the original tuple before
HTTP and blocks a different pending action. `RetrySavedAction` preserves that
original across actor recreation and regional changes. Confirmed purchases are
persisted before inventory refresh; their retry performs only GET. Unconfirmed
purchases keep the original ID even after `OPERATION_EXPIRED`. Removed/disabled
offers can still recover a retained original receipt after explicit review.
Non-purchase CAS intents have an explicit `AcknowledgeItemActionAfterRefresh`;
this acknowledges the exact original and does not cancel a server request.

Before an unconfirmed purchase retry, the actor reads current offers. A changed
or unavailable offer requires a second explicit confirmation; the original
operation ID is unchanged. The player can inspect current offers via
`PlayerValues->GetCurrentOffers()`. There is no atomic `expectedCatalogVersion`
in the existing purchase contract: configuration may change between GET and
POST, so the sample promises no price lock. An expired/evicted unknown operation
requires external reconciliation; the sample never manufactures a replacement.

REST is available in Cloud and simpleServer. Items adds no realtime topic;
realtime remains Cloud-only. Local native automation uses the strict artificial
fixture on loopback 18811:

```bash
python3 Tests/run_native_items_test.py --engine '/path/to/UE_5.8' \
  --host-project /absolute/path/to/independently-built/HostProject.uproject \
  --report /absolute/path/to/fresh-report
```

This native HTTP proof is separate from a deployment or real-account acceptance.


## Shared Objects (TASK-1012)

`UHorizonSubsystem::SharedObjects` is `UHorizonSharedObjectsManager`. Include
`Managers/HorizonSharedObjectsManager.h` and `Models/HorizonSharedObjects.h`.
Use an already authenticated player; no method creates or restores an identity.
All native delegates have `(bool, const TypedResult&, const FString& ErrorCode,
const FString& ErrorMessage)`. Bind native callbacks with `CreateUObject` or
`BindUObject`. Error codes remain the original server codes.

| Call | Result / delegate |
|---|---|
| `GetCollections(UserId, Callback, AfterKey="", Limit=50, CatalogRevision=-1)` | `FHorizonSharedCollectionPage` / `FOnSharedCollectionsComplete` |
| `GetUsage(UserId, Collection, Callback)` | `FHorizonSharedUsage` / `FOnSharedUsageComplete` |
| `GetObject(UserId, Collection, ObjectId, Callback)` | `FHorizonSharedObjectRead` / `FOnSharedObjectComplete` |
| `QueryObjects(UserId, Collection, Query, Callback)` | `FHorizonSharedQueryResult` / `FOnSharedQueryComplete` |
| `CreateObject(UserId, Collection, ExpectedUsageRevision, Input, Callback, GroupId="")` | `FHorizonSharedWriteResult` / `FOnSharedWriteComplete` |
| `UpdateObject(UserId, Collection, ObjectId, ExpectedObjectRevision, Input, Callback)` | `FHorizonSharedWriteResult` / `FOnSharedWriteComplete` |
| `DeleteObject(UserId, Collection, ObjectId, ExpectedObjectRevision, Callback)` | `FHorizonSharedDeleteResult` / `FOnSharedDeleteComplete` |

Every result has `bPresent`. Read results expose `.Object`, query results `.Objects`,
write results `.Object` and `.Usage`; delete results `.ObjectId`, `.bDeleted` and
`.Usage`. Polling envelopes expose `.PollAfterSeconds` and `.ServerTime`.
Null cursors use `bHasNextCursor=false`; an empty page with a continuation is valid.
Collection pagination uses `.NextAfterKey`/`.bHasNextAfterKey` and the original
`.Revision` passed as `CatalogRevision` on explicit continuation.

`FHorizonSharedObjectInput.Data` and returned `.Data` are opaque `TArray<uint8>`.
The transport produces/validates canonical padded Base64. The SDK does not decode
these bytes as a game's JSON. Inputs and outputs preserve empty bytes, false,
nullable group/position/expiry, safe integer tokens and Unicode including NUL.
Search is at most three unique `FHorizonSharedSearchValue {Key,Value}` entries.
`Value.Type` selects `EHorizonSharedScalarType::Integer` or `String`; values use
`.IntegerValue` or `.StringValue`. Strings are at most96 UTF-8 bytes. Positions
use `.bHasPosition` and `.Position.X/Y`, bounded by +/-9007199254740991.
Collection/owner counts use positive Int32 operator values; storage bytes remain exact
safe integers. The object bound is 1 MiB decoded. Retained definitions remain readable after
current policy is lowered; former tariff defaults are not parser ceilings.

`FHorizonSharedQuery.Mode` is List, Owner, Field, Rectangle or Random. Owner mode
requires `.OwnerId`; `.GroupId` selects the explicit group partition. Field mode
uses `.Field` and either `.bUseEqual/.Equal` or `.Min/.Max` typed scalars.
Rectangle uses `.MinX/.MaxX/.MinY/.MaxY`. Random requires an integer field/range;
it is bounded nonuniform sampling and accepts no cursor. Set `.Limit` explicitly
(1..50, default20). `.Cursor` is an opaque continuation, never an authorization.
The server checks current ACLs; callers do not reconstruct friendship/group ACLs.
Group, owner and expiry are immutable after creation.

Blueprint request/event wrappers live on the manager. Typed async nodes are in
`AsyncActions/HorizonAsync_SharedObjects.h`: SharedCollections, SharedUsage,
SharedObject, SharedQuery, SharedWrite(Create/Update) and SharedDelete. Factories
freeze the original player/context and inputs; each node activates once. A
host/project/session ABA, sign-out or explicit cache clear rejects stale answers.
One logical manager request is active at a time. It retains one materialized
request scope, with monotone revision/server-time watermarks across ClearCache;
it starts no timers, automatic page traversal or automatic auth.

All three mutations send the original decimal `If-Match` and make exactly one
HTTP attempt, including429,5xx and connection loss. Query POST is read-only and
uses the existing bounded read retry policy. A timeout never authorizes replacing
a revision. Persist the original project/player/collection/action/object/revision
and bytes before sending, then reconcile explicitly. Unknown create: read usage
and owner objects to discover the original object, rather than creating another.

`AHorizonSharedObjectsExample` is an executable, existing-session Actor:

1. `RefreshOwned(Collection, GroupId)` reads one usage and one owner page.
2. A player action calls `CreateMarker(Input)`, `UpdateMarker(Id,OriginalRevision,Input)`
   or `DeleteMarker(Id,OriginalRevision)`. The sample prepends a unique marker nonce
   on create, an example game format used for exact owner recovery.
3. `RetryOriginal()` may issue the first send of a definitely-unsent serial0
   original; serial1 creates only trigger owner reconciliation reads. Update/delete
   retry their original tuple. No fresh revision is substituted.
4. Continue explicitly with `RefreshOwned(Collection, GroupId, OriginalNextCursor)`.
   Review the matching marker and call `ConfirmRecoveredCreate(Id)`. An uncertain
   create cannot be blindly acknowledged. Other conflicts may be explicitly
   acknowledged using `AcknowledgeAfterRefresh()` after a fresh exact owner read.
5. `QueryRectangle(...)` reads a world-map area. `QueryOpponents(Collection,Field,Min,Max)`
   samples an explicitly selected power range. No Tick/BeginPlay operation runs.

The sample's single pending slot is keyed by project+player+feature, independent of
host and session. It persists original search strings as counted UTF-8 Base64,
since Unreal SaveGame's ordinary FString serialization can alter Unicode after NUL.
It validates the complete original hash and signature on replay/acknowledgement.
Regional switching cannot create a second pending slot. Refresh respects returned
pollAfterSeconds for each query shape (at most256 unexpired local refresh scopes);
explicit cursor continuation may proceed immediately. A changed context/cache
generation invalidates displayed state. Files that fail validation stay protected.

### Asynchronous defense recipe

Store the game's defense bytes in `.Data`, an integer `power` search field and an
owned collection. Choose a row from `QueryOpponents`, freeze its ID, owner, revision
and exact bytes in the game's durable run intent before calling Validated Actions.
The game defines and validates a versioned initial-state format containing this
binding. It owns combat rules, input recording and reward interpretation.
The existing calls are:

```cpp
#include "Managers/HorizonValidatedActionsManager.h"
FHorizonRunContext Context;
Context.SimulationVersion = TEXT("defense-v1");
Context.InitialState = FrozenGameInitialState;
SDK->ValidatedActions->StartRun(TEXT(""), Context, OnRunStarted);
// After the game simulates with the returned seed and retains the same run/input log:
SDK->ValidatedActions->SubmitValidated(Score, FrozenInputLog, TEXT("defense_demo"),
    TEXT(""), {}, OnSubmitted);
```

These are `/validated-actions/runs` and `/validated-actions/submit`. Shared Objects
supplies state; it does not validate the claimed defender, choose combat winners
or provide live matchmaking. The game's trusted backend validates target/result
binding and alone sends ordinary notices through the scoped admin Inbox API.
Never include an Account Key or trusted result authority in an engine build.
Validated Actions and realtime retain their Cloud-only capability boundary;
ordinary Shared Objects REST also has the separate simpleServer implementation.
A full standalone game/Hello, trusted notice worker and live Develop proof are
separate acceptance gates from this SDK's local fixture.

`Tests/run_native_shared_objects_test.py` runs the actual Unreal strict model and
HTTP/Actor suite, including the composed range-query/validated-start/submit defense recipe, against isolated127.0.0.1:18818 and18819, after building a dedicated
HostProject. The fixture rejects extra requests, wrong original If-Match values,
a second uncertain create, automatic auth, polling and page traversal. No real
account, database or deployed backend participates.
