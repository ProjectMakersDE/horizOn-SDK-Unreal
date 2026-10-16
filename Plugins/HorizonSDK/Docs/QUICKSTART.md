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
| Gift Codes    | `Horizon->GiftCodes->Redeem()`| Redeem Gift Code           |
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
finite numbers, depth 16 and 2048 compact UTF-8 bytes. `GetJson()` preserves number
lexemes, empty/dot/dollar keys, Unicode and null; use this original text when editing data.
Do not round-trip it through a `double` map. Blueprint uses `ParseGroupData` / `GroupDataJson`.

All 16 writes send one transport attempt. Revision/request identity is mandatory;
there is no operation receipt. After a timeout retain the original tuple, explicitly
refresh and reconcile, and only explicitly retry that same tuple. Never substitute the
new revision or a new invitation ID. Feature codes are returned unchanged. `GetMembers`
returns one page (1–50); callers explicitly request the next cursor and check revision
changes between pages. No automatic full-list fetch or polling runs. Private snapshots
are cleared on session/project/host/cache invalidation and authorization failure.
Revision/time watermarks survive `ClearCache`; tracking is bounded to 256 distinct group
IDs per connection/session context (additional scopes require a new context).

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
