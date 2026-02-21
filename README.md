<p align="center">
  <a href="https://horizon.pm">
    <img src="https://horizon.pm/media/images/og-image.png" alt="horizOn - Game Backend & Live-Ops Dashboard" />
  </a>
</p>

# horizOn SDK for Unreal Engine

[![Unreal Engine 5.5+](https://img.shields.io/badge/Unreal_Engine-5.5%2B-blue?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-orange)](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/releases)

Official Unreal Engine SDK for **horizOn** Backend-as-a-Service by [ProjectMakers](https://projectmakers.de).

## Features

| Feature | Manager | Description |
|---------|---------|-------------|
| Authentication | `UHorizonAuthManager` | Anonymous, email, and Google sign-up / sign-in with session caching |
| Cloud Save | `UHorizonCloudSaveManager` | Save and load player data in JSON or binary format |
| Leaderboard | `UHorizonLeaderboardManager` | Submit scores, retrieve top entries, rank, and surrounding entries |
| Remote Config | `UHorizonRemoteConfigManager` | Typed key-value retrieval (string, int, float, bool) with caching |
| News | `UHorizonNewsManager` | In-game news feed with language filtering and TTL cache |
| Gift Codes | `UHorizonGiftCodeManager` | Validate and redeem promotional codes |
| Feedback | `UHorizonFeedbackManager` | Submit bug reports, feature requests, and general feedback |
| User Logs | `UHorizonUserLogManager` | Server-side structured logging for analytics and debugging |

## Requirements

- Unreal Engine **5.5** or later
- A horizOn project with an API key (obtain from the [horizOn dashboard](https://horizon.pm))

## Installation

1. Download the latest release ZIP from [Releases](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/releases).
2. Extract `HorizonSDK/` into your project's `Plugins/` directory.
3. Open your project in Unreal Editor.
4. Go to **Edit > Plugins**, search for "horizOn SDK", and enable it.
5. Restart the editor when prompted.

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

| Setting | Description |
|---------|-------------|
| API Key | Your project API key |
| Backend Hosts | One or more backend URLs (multiple hosts enable ping-based selection) |
| Connection Timeout | HTTP request timeout in seconds |
| Max Retries | Number of retry attempts on failure |
| Retry Delay | Delay between retries in seconds |
| Log Level | SDK log verbosity |

Alternatively, use **Tools > horizOn > Import Config...** to import a JSON config file from the horizOn dashboard.

## Rate Limiting

The SDK enforces a default rate limit of **10 requests per minute**. Requests exceeding this limit receive an HTTP 429 response and are automatically retried after the cooldown period.

## Documentation

- [Quick Start Guide](Plugins/HorizonSDK/Docs/QUICKSTART.md)
- [Plugin README](Plugins/HorizonSDK/README.md)
- [horizOn Documentation](https://horizon.pm/docs)

## Project Structure

```
Plugins/HorizonSDK/
├── HorizonSDK.uplugin
├── Source/
│   ├── HorizonSDK/              (Runtime module)
│   │   ├── Public/
│   │   │   ├── Http/            HTTP client
│   │   │   ├── Models/          Data structs
│   │   │   ├── Managers/        Feature managers
│   │   │   ├── AsyncActions/    Blueprint async nodes
│   │   │   └── Example/         Example widget
│   │   └── Private/
│   └── HorizonSDKEditor/        (Editor module)
│       ├── Public/
│       └── Private/
├── Docs/
└── Config/
```

## Support

- [horizOn Dashboard](https://horizon.pm)
- [Documentation](https://horizon.pm/docs)
- [GitHub Issues](https://github.com/ProjectMakersDE/horizOn-SDK-Unreal/issues)

## License

MIT License - Copyright (c) 2025 [ProjectMakers](https://projectmakers.de). See [LICENSE](LICENSE) for details.
