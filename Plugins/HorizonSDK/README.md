# horizOn SDK for Unreal Engine

The horizOn SDK is a fully-featured Unreal Engine plugin that connects your game to the horizOn backend-as-a-service platform. It provides authentication, cloud saves, leaderboards, remote configuration, in-game news, gift code redemption, player feedback, and server-side user logging -- all accessible from both C++ and Blueprints.

## Features

- **Authentication** -- Anonymous, email, and Google sign-up / sign-in with automatic session caching.
- **Cloud Save** -- Save and load player data in JSON (string) or binary format.
- **Leaderboard** -- Submit scores, retrieve top entries, player rank, and surrounding entries with in-memory caching.
- **Remote Config** -- Typed key-value retrieval (string, int, float, bool) with cache support.
- **News** -- Fetch and display in-game news entries with language filtering and 5-minute TTL cache.
- **Gift Codes** -- Validate and redeem promotional codes.
- **Feedback** -- Submit bug reports, feature requests, and general feedback with optional device info.
- **User Logs** -- Send structured log entries (info, warning, error) to the server for analytics and debugging.

## Requirements

- Unreal Engine **5.5** or later
- A horizOn project with an API key (obtain from the horizOn dashboard)

## Installation

1. Copy the `HorizonSDK` folder into your project's `Plugins/` directory.
2. Open your project in Unreal Editor.
3. Go to **Edit > Plugins**, search for "horizOn SDK", and enable it.
4. Restart the editor when prompted.

## Quick Start (C++)

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

// Submit a leaderboard score
Horizon->Leaderboard->SubmitScore(42000,
    FOnRequestComplete::CreateLambda([](bool bSuccess, const FString& Error)
    {
        // Handle result
    }));
```

## Quick Start (Blueprints)

1. Use the **"horizOn Connect"** async node to connect to the server.
2. Use **"horizOn Sign Up Anonymous"** or **"horizOn Sign In Email"** to authenticate.
3. Call any feature node (Submit Score, Save Data, Load News, etc.).

All async nodes expose **On Success** and **On Failure** execution pins for easy error handling.

## Configuration

Open **Project Settings > Plugins > horizOn SDK** to set:

- **API Key** -- Your project API key.
- **Backend Hosts** -- One or more backend URLs. Multiple hosts enable automatic ping-based selection.
- **Connection Timeout**, **Max Retries**, **Retry Delay** -- Network resilience settings.
- **Log Level** -- Control SDK log verbosity.

Alternatively, use **Tools > horizOn > Import Config...** to import a JSON config file from the horizOn dashboard.

## Documentation

See [Docs/QUICKSTART.md](Docs/QUICKSTART.md) for a 5-minute setup guide.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
