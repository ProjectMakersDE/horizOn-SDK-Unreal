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

1. Place an **"horizOn Connect"** async node.
2. From the **On Success** pin, place an **"horizOn Sign Up Anonymous"** node.
3. You are now connected and authenticated.

## 5. Use Features

Once connected and signed in, call any manager method:

| Feature       | C++ Manager                   | Blueprint Node             |
|---------------|-------------------------------|----------------------------|
| Cloud Save    | `Horizon->CloudSave->Save()`  | horizOn Save Data          |
| Cloud Load    | `Horizon->CloudSave->Load()`  | horizOn Load Data          |
| Leaderboard   | `Horizon->Leaderboard->SubmitScore()` | horizOn Submit Score |
| Remote Config | `Horizon->RemoteConfig->GetAllConfigs()` | horizOn Get All Configs |
| News          | `Horizon->News->LoadNews()`   | horizOn Load News          |
| Gift Codes    | `Horizon->GiftCodes->Redeem()`| horizOn Redeem Gift Code   |
| Feedback      | `Horizon->Feedback->Submit()` | horizOn Submit Feedback    |
| User Logs     | `Horizon->UserLogs->Info()`   | horizOn Create User Log    |

All async operations use callbacks (C++) or output execution pins (Blueprints) for success and failure handling.

## Troubleshooting

- **"HttpClient not initialized"** -- Make sure you call `ConnectToServer()` before any other operation.
- **Authentication required** -- Cloud Save, Leaderboard, Gift Codes, and User Logs require the player to be signed in first.
- **Multiple hosts** -- If you configure multiple backend hosts, the SDK automatically pings each one and selects the fastest.
- **Session caching** -- The SDK saves session tokens to disk. Returning players are automatically re-authenticated via `RestoreSession()`.
- **Clear cache** -- Use **Tools > horizOn > Clear Session Cache** in the editor to wipe local session data.
