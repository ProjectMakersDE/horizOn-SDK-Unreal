# horizOn Unreal SDK Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a Marketplace-ready Unreal Engine 5.5+ plugin that provides the full horizOn backend SDK (auth, cloud save, leaderboard, remote config, news, gift codes, feedback, user logs) with Blueprint support.

**Architecture:** UGameInstanceSubsystem entry point owning UObject-based feature managers. HTTP client wraps FHttpModule with retry/rate-limiting. Configuration via UDeveloperSettings. Session persistence via USaveGame. Blueprint exposure via static library + async action nodes.

**Tech Stack:** Unreal Engine 5.5+ C++, FHttpModule, FJsonSerializer, UMG, UDeveloperSettings, USaveGame, UBlueprintAsyncActionBase

**Reference SDKs:** Unity SDK at `/home/projectmakers/Dokumente/GitHub/horizOn-SDK-Unity/` and Godot SDK at `/home/projectmakers/Dokumente/GitHub/horizOn-SDK-Godot/`

---

## Task 1: Plugin Scaffold

**Files:**
- Create: `Plugins/HorizonSDK/HorizonSDK.uplugin`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/HorizonSDK.Build.cs`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSDKModule.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSDKModule.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/HorizonSDKEditor.Build.cs`
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Public/HorizonSDKEditorModule.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Private/HorizonSDKEditorModule.cpp`
- Create: `Plugins/HorizonSDK/Config/FilterPlugin.ini`

**Step 1: Create .uplugin descriptor**

```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "horizOn SDK",
    "Description": "horizOn Backend SDK for Unreal Engine. Provides authentication, cloud saves, leaderboards, remote config, news, gift codes, feedback, and server-side logging.",
    "Category": "Networking",
    "CreatedBy": "ProjectMakers",
    "CreatedByURL": "https://horizon.pm",
    "DocsURL": "https://horizon.pm/docs",
    "MarketplaceURL": "",
    "SupportURL": "https://horizon.pm/support",
    "CanContainContent": true,
    "IsBetaVersion": false,
    "IsExperimentalVersion": false,
    "Installed": false,
    "EngineVersion": "5.5.0",
    "Modules": [
        {
            "Name": "HorizonSDK",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        },
        {
            "Name": "HorizonSDKEditor",
            "Type": "Editor",
            "LoadingPhase": "Default"
        }
    ]
}
```

**Step 2: Create Runtime Build.cs**

```csharp
using UnrealBuildTool;

public class HorizonSDK : ModuleRules
{
    public HorizonSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "HTTP",
            "Json",
            "JsonUtilities"
        });
    }
}
```

**Step 3: Create Editor Build.cs**

```csharp
using UnrealBuildTool;

public class HorizonSDKEditor : ModuleRules
{
    public HorizonSDKEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "DesktopPlatform",
            "HorizonSDK"
        });
    }
}
```

**Step 4: Create Runtime module .h/.cpp**

HorizonSDKModule.h:
```cpp
#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHorizonSDK, Log, All);

class FHorizonSDKModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
```

HorizonSDKModule.cpp:
```cpp
#include "HorizonSDKModule.h"

DEFINE_LOG_CATEGORY(LogHorizonSDK);

#define LOCTEXT_NAMESPACE "FHorizonSDKModule"

void FHorizonSDKModule::StartupModule()
{
}

void FHorizonSDKModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHorizonSDKModule, HorizonSDK)
```

**Step 5: Create Editor module .h/.cpp**

HorizonSDKEditorModule.h:
```cpp
#pragma once

#include "Modules/ModuleManager.h"

class FHorizonSDKEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
```

HorizonSDKEditorModule.cpp:
```cpp
#include "HorizonSDKEditorModule.h"

#define LOCTEXT_NAMESPACE "FHorizonSDKEditorModule"

void FHorizonSDKEditorModule::StartupModule()
{
}

void FHorizonSDKEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHorizonSDKEditorModule, HorizonSDKEditor)
```

**Step 6: Create FilterPlugin.ini**

```ini
[FilterPlugin]
; This section lists additional files which will be packaged along with your plugin.
/Config/FilterPlugin.ini
```

**Step 7: Commit**

```bash
git add Plugins/
git commit -m "feat: scaffold HorizonSDK plugin with runtime and editor modules"
```

---

## Task 2: Types, Enums, and Delegates

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonTypes.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonTypes.cpp`

**Step 1: Create HorizonTypes.h**

All shared enums, structs, and delegate declarations used across the SDK. Reference the Unity SDK's `EventKeys.cs`, `AuthType.cs`, `ConnectionStatus.cs`, and `LogType.cs` for enum values.

Enums to define:
```cpp
UENUM(BlueprintType)
enum class EHorizonAuthType : uint8
{
    Anonymous UMETA(DisplayName = "Anonymous"),
    Email UMETA(DisplayName = "Email"),
    Google UMETA(DisplayName = "Google")
};

UENUM(BlueprintType)
enum class EHorizonConnectionStatus : uint8
{
    Disconnected,
    Connecting,
    Connected,
    Failed
};

UENUM(BlueprintType)
enum class EHorizonLogLevel : uint8
{
    Debug,
    Info,
    Warning,
    Error,
    None
};

UENUM(BlueprintType)
enum class EHorizonErrorCode : uint8
{
    None = 0,
    InvalidRequest,
    Unauthorized,
    Forbidden,
    NotFound,
    Conflict,
    RateLimited,
    ServerError,
    ConnectionFailed,
    Unknown
};
```

Delegates to define:
```cpp
// Auth callbacks
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonAuthComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonConnectionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonUserSignedIn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonUserSignedOut);

// Non-dynamic delegates for C++ callbacks
DECLARE_DELEGATE_OneParam(FOnAuthComplete, bool /*bSuccess*/);
DECLARE_DELEGATE_TwoParams(FOnRequestComplete, bool /*bSuccess*/, const FString& /*ErrorMessage*/);
DECLARE_DELEGATE_TwoParams(FOnStringComplete, bool /*bSuccess*/, const FString& /*Data*/);
DECLARE_DELEGATE_TwoParams(FOnBinaryComplete, bool /*bSuccess*/, const TArray<uint8>& /*Data*/);
```

**Step 2: Create HorizonTypes.cpp** (empty, just includes header for UHT generation)

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonTypes.h Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonTypes.cpp
git commit -m "feat: add shared types, enums, and delegate declarations"
```

---

## Task 3: Data Models

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Models/HorizonUserData.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Models/HorizonLeaderboardEntry.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Models/HorizonNewsEntry.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Models/HorizonNetworkResponse.h`

**Step 1: Create all four model headers**

Each model is a `USTRUCT(BlueprintType)` mirroring the Unity SDK's data classes. Reference:
- Unity `UserData.cs` for FHorizonUserData fields
- Unity `LeaderboardResponses.cs` (SimpleLeaderboardEntry) for FHorizonLeaderboardEntry
- Unity `NewsResponses.cs` (UserNewsResponse) for FHorizonNewsEntry
- Unity `NetworkService.cs` (NetworkResponse<T>) for FHorizonNetworkResponse

FHorizonUserData:
```cpp
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonUserData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString UserId;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString Email;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    EHorizonAuthType AuthType = EHorizonAuthType::Anonymous;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString AccessToken;

    UPROPERTY()
    FString AnonymousToken;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    bool bIsEmailVerified = false;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    bool bIsAnonymous = false;

    bool IsValid() const { return !UserId.IsEmpty() && !AccessToken.IsEmpty(); }
    void Clear();

    /** Populate from auth response JSON */
    void UpdateFromAuthResponse(const TSharedPtr<FJsonObject>& JsonObject);
};
```

FHorizonLeaderboardEntry:
```cpp
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonLeaderboardEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    int32 Position = 0;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    FString Username;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Leaderboard")
    int64 Score = 0;

    static FHorizonLeaderboardEntry FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
```

FHorizonNewsEntry:
```cpp
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonNewsEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString Title;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString ReleaseDate;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|News")
    FString LanguageCode;

    static FHorizonNewsEntry FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};
```

FHorizonNetworkResponse:
```cpp
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonNetworkResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    int32 StatusCode = 0;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    EHorizonErrorCode ErrorCode = EHorizonErrorCode::None;

    /** JSON response data (C++ only, not exposed to BP) */
    TSharedPtr<FJsonObject> JsonData;

    /** Binary response data */
    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Network")
    TArray<uint8> BinaryData;

    static EHorizonErrorCode StatusToErrorCode(int32 HttpStatus);
};
```

**Step 2: Create matching .cpp files** in Private/Models/ with implementations for `Clear()`, `UpdateFromAuthResponse()`, `FromJson()`, and `StatusToErrorCode()`.

Reference the Unity SDK's `AuthResponse.cs` for exact JSON field names used in `UpdateFromAuthResponse`:
- `userId`, `username`, `email`, `accessToken`, `anonymousToken`, `authStatus`, `isAnonymous`, `isVerified`, `googleId`

Reference `SimpleLeaderboardEntry` for leaderboard JSON fields:
- `position`, `username`, `score`

Reference `UserNewsResponse` for news JSON fields:
- `id`, `title`, `message`, `releaseDate`, `languageCode`

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Models/ Plugins/HorizonSDK/Source/HorizonSDK/Private/Models/
git commit -m "feat: add data models (UserData, LeaderboardEntry, NewsEntry, NetworkResponse)"
```

---

## Task 4: Configuration (UDeveloperSettings)

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonConfig.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonConfig.cpp`

**Step 1: Create HorizonConfig**

Reference Unity's `HorizonConfig.cs` for the settings fields. This extends `UDeveloperSettings` so it appears in Project Settings > Plugins > horizOn.

```cpp
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "horizOn SDK"))
class HORIZONSDK_API UHorizonConfig : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UHorizonConfig();

    /** API key from horizOn dashboard */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (DisplayName = "API Key"))
    FString ApiKey;

    /** Backend host URLs. Single host = direct connection. Multiple = ping-based selection. */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (DisplayName = "Backend Hosts"))
    TArray<FString> Hosts;

    /** Connection timeout in seconds */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (ClampMin = "5", ClampMax = "60"))
    int32 ConnectionTimeoutSeconds = 10;

    /** Maximum retry attempts for failed requests */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (ClampMin = "0", ClampMax = "10"))
    int32 MaxRetryAttempts = 3;

    /** Delay between retries in seconds */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float RetryDelaySeconds = 1.0f;

    /** Minimum log level for SDK messages */
    UPROPERTY(Config, EditAnywhere, Category = "Logging")
    EHorizonLogLevel LogLevel = EHorizonLogLevel::Info;

    /** Get the CDO config instance */
    static const UHorizonConfig* Get();

    // UDeveloperSettings overrides
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("horizOn SDK"); }
};
```

**Step 2: Implement .cpp** with `Get()` returning `GetDefault<UHorizonConfig>()`.

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonConfig.h Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonConfig.cpp
git commit -m "feat: add UHorizonConfig developer settings"
```

---

## Task 5: Session Persistence (USaveGame)

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSessionSave.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSessionSave.cpp`

**Step 1: Create HorizonSessionSave**

Reference Unity's PlayerPrefs-based session caching in `UserManager.cs` (keys: `horizOn_UserSession`, `horizOn_AnonymousToken`).

```cpp
UCLASS()
class HORIZONSDK_API UHorizonSessionSave : public USaveGame
{
    GENERATED_BODY()

public:
    static const FString SlotName; // "HorizonSession"
    static const int32 UserIndex = 0;

    UPROPERTY()
    FString CachedUserId;

    UPROPERTY()
    FString CachedAccessToken;

    UPROPERTY()
    FString CachedAnonymousToken;

    UPROPERTY()
    FString CachedDisplayName;

    UPROPERTY()
    bool bIsAnonymous = false;

    /** Save current state to disk */
    bool SaveToDisk() const;

    /** Load from disk, returns nullptr if no save exists */
    static UHorizonSessionSave* LoadFromDisk();

    /** Delete saved session from disk */
    static void DeleteFromDisk();

    /** Check if a saved session exists */
    static bool HasSavedSession();
};
```

**Step 2: Implement .cpp** using `UGameplayStatics::SaveGameToSlot` / `LoadGameFromSlot` / `DoesSaveGameExist` / `DeleteGameInSlot`.

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSessionSave.h Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSessionSave.cpp
git commit -m "feat: add session persistence via USaveGame"
```

---

## Task 6: HTTP Client

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Http/HorizonHttpClient.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Http/HorizonHttpClient.cpp`

**Step 1: Create HorizonHttpClient.h**

Reference Unity's `NetworkService.cs` for the full HTTP handling pattern: headers, retry logic, rate limiting (429 + Retry-After), host ping selection, and response parsing.

```cpp
DECLARE_DELEGATE_OneParam(FOnHttpResponse, const FHorizonNetworkResponse& /*Response*/);

UCLASS()
class HORIZONSDK_API UHorizonHttpClient : public UObject
{
    GENERATED_BODY()

public:
    void Initialize();

    /** Connect to the best available host */
    void ConnectToServer(FOnRequestComplete OnComplete);

    /** Disconnect and clear active host */
    void Disconnect();

    // State
    bool IsConnected() const { return !ActiveHost.IsEmpty(); }
    FString GetActiveHost() const { return ActiveHost; }
    EHorizonConnectionStatus GetConnectionStatus() const { return ConnectionStatus; }

    /** Set session token for authenticated requests */
    void SetSessionToken(const FString& Token);
    void ClearSessionToken();

    // HTTP methods
    void Get(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);
    void PostJson(const FString& Endpoint, const TSharedRef<FJsonObject>& Body, bool bUseSessionToken, FOnHttpResponse OnComplete);
    void PostBinary(const FString& Endpoint, const TArray<uint8>& Data, bool bUseSessionToken, FOnHttpResponse OnComplete);
    void GetBinary(const FString& Endpoint, bool bUseSessionToken, FOnHttpResponse OnComplete);

private:
    FString ActiveHost;
    FString SessionToken;
    EHorizonConnectionStatus ConnectionStatus = EHorizonConnectionStatus::Disconnected;

    /** Internal request with retry logic */
    void SendRequest(
        const FString& Verb,
        const FString& Url,
        const FString& ContentType,
        const TArray<uint8>& Payload,
        bool bUseSessionToken,
        int32 RetryCount,
        FOnHttpResponse OnComplete
    );

    /** Build standard headers */
    void ApplyHeaders(const TSharedRef<IHttpRequest>& Request, const FString& ContentType, bool bUseSessionToken) const;

    /** Parse HTTP response into FHorizonNetworkResponse */
    FHorizonNetworkResponse ParseResponse(FHttpResponsePtr Response, bool bConnectedSuccessfully) const;

    /** Handle retry logic for a failed request */
    void HandleRetry(
        const FString& Verb,
        const FString& Url,
        const FString& ContentType,
        const TArray<uint8>& Payload,
        bool bUseSessionToken,
        int32 RetryCount,
        int32 StatusCode,
        const FString& RetryAfterHeader,
        FOnHttpResponse OnComplete
    );

    /** Ping a single host and measure latency */
    void PingHost(const FString& Host, int32 PingIndex, TSharedRef<TMap<FString, TArray<float>>> Results, TSharedRef<int32> PendingCount, FOnRequestComplete FinalCallback);
};
```

**Step 2: Implement HorizonHttpClient.cpp**

Key implementation details from Unity's `NetworkService.cs`:
- **Headers:** `X-API-Key` from config, `Authorization: Bearer {token}` if useSessionToken, `Content-Type` as specified
- **Host selection:** If 1 host, ping `/actuator/health` once. If multiple, ping each 3 times, select minimum average latency. Health response must contain `"status":"UP"`.
- **Retry:** Loop up to `MaxRetryAttempts + 1`. On 429, read `Retry-After` header (float seconds) and schedule retry via `FTimerManager`. On 5xx or connection failure, wait `RetryDelaySeconds` then retry. On 4xx (except 429), return immediately.
- **Response parsing:** Attempt JSON parse via `FJsonSerializer::Deserialize`. On failure, store raw string as ErrorMessage. Map status codes to `EHorizonErrorCode` via `StatusToErrorCode()`.

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Http/ Plugins/HorizonSDK/Source/HorizonSDK/Private/Http/
git commit -m "feat: add HTTP client with retry, rate-limiting, and host selection"
```

---

## Task 7: Subsystem (Entry Point)

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSubsystem.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSubsystem.cpp`

**Step 1: Create HorizonSubsystem.h**

Reference Unity's `HorizonApp.cs` for the entry point pattern. The subsystem owns all managers and the HTTP client.

```cpp
UCLASS()
class HORIZONSDK_API UHorizonSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // Manager access
    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonAuthManager* Auth;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonCloudSaveManager* CloudSave;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonLeaderboardManager* Leaderboard;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonRemoteConfigManager* RemoteConfig;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonNewsManager* News;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonGiftCodeManager* GiftCodes;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonFeedbackManager* Feedback;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn")
    UHorizonUserLogManager* UserLogs;

    // Connection
    UFUNCTION(BlueprintCallable, Category = "horizOn")
    void ConnectToServer();

    UFUNCTION(BlueprintCallable, Category = "horizOn")
    void Disconnect();

    UFUNCTION(BlueprintPure, Category = "horizOn")
    bool IsConnected() const;

    UFUNCTION(BlueprintPure, Category = "horizOn")
    EHorizonConnectionStatus GetConnectionStatus() const;

    /** Get the HTTP client (C++ only) */
    UHorizonHttpClient* GetHttpClient() const { return HttpClient; }

    // Blueprint events
    UPROPERTY(BlueprintAssignable, Category = "horizOn|Events")
    FOnHorizonConnected OnConnected;

    UPROPERTY(BlueprintAssignable, Category = "horizOn|Events")
    FOnHorizonConnectionFailed OnConnectionFailed;

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;
};
```

**Step 2: Implement HorizonSubsystem.cpp**

In `Initialize()`:
1. Create `UHorizonHttpClient` via `NewObject<UHorizonHttpClient>(this)`
2. Call `HttpClient->Initialize()`
3. Create all 8 managers via `NewObject<UManagerType>(this)` and call their `Initialize(HttpClient)` methods
4. Log `"horizOn SDK initialized"`

In `Deinitialize()`:
1. Disconnect if connected
2. Log `"horizOn SDK shut down"`

In `ConnectToServer()`:
1. Call `HttpClient->ConnectToServer()` with a callback that broadcasts `OnConnected` or `OnConnectionFailed`

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSubsystem.h Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSubsystem.cpp
git commit -m "feat: add UHorizonSubsystem as main SDK entry point"
```

---

## Task 8: Auth Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonAuthManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonAuthManager.cpp`

**Step 1: Create HorizonAuthManager**

Reference Unity's `UserManager.cs` for exact API endpoints, request JSON field names, response handling, and session caching logic.

**API endpoints:**
- `POST /api/v1/app/user-management/signup`
- `POST /api/v1/app/user-management/signin`
- `POST /api/v1/app/user-management/check-auth`
- `POST /api/v1/app/user-management/change-name`
- `POST /api/v1/app/user-management/verify-email`
- `POST /api/v1/app/user-management/forgot-password`
- `POST /api/v1/app/user-management/reset-password`

**Request JSON field names** (from Unity's `SignUpRequest.cs`, `SignInRequest.cs`):
- SignUp Anonymous: `{ "type": "ANONYMOUS", "username": "...", "anonymousToken": "..." }`
- SignUp Email: `{ "type": "EMAIL", "email": "...", "password": "...", "username": "..." }`
- SignUp Google: `{ "type": "GOOGLE", "googleAuthorizationCode": "...", "googleRedirectUri": "...", "username": "..." }`
- SignIn Email: `{ "type": "EMAIL", "email": "...", "password": "..." }`
- SignIn Anonymous: `{ "type": "ANONYMOUS", "anonymousToken": "..." }`
- SignIn Google: `{ "type": "GOOGLE", "googleAuthorizationCode": "...", "googleRedirectUri": "..." }`
- CheckAuth: `{ "userId": "...", "sessionToken": "..." }`
- ChangeName: `{ "userId": "...", "sessionToken": "...", "newName": "..." }`

**Response JSON field names** (from Unity's `AuthResponse.cs`):
- `userId`, `username`, `email`, `accessToken`, `anonymousToken`, `authStatus`, `isAnonymous`, `isVerified`, `googleId`, `createdAt`

**Anonymous token generation:** `FGuid::NewGuid().ToString(EGuidFormats::DigitsLower)` (32 hex chars, matches Unity's `Guid.NewGuid().ToString("N")`)

**Session caching:** On successful auth, save to `UHorizonSessionSave`. On `RestoreSession()`, load from save game and call `CheckAuth`.

```cpp
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonAuthManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient);

    // Sign Up
    void SignUpAnonymous(const FString& DisplayName, FOnAuthComplete OnComplete, const FString& AnonymousToken = TEXT(""));
    void SignUpEmail(const FString& Email, const FString& Password, const FString& Username, FOnAuthComplete OnComplete);
    void SignUpGoogle(const FString& GoogleAuthCode, const FString& RedirectUri, const FString& Username, FOnAuthComplete OnComplete);

    // Sign In
    void SignInEmail(const FString& Email, const FString& Password, FOnAuthComplete OnComplete);
    void SignInAnonymous(const FString& AnonymousToken, FOnAuthComplete OnComplete);
    void SignInGoogle(const FString& GoogleAuthCode, const FString& RedirectUri, FOnAuthComplete OnComplete);
    void RestoreSession(FOnAuthComplete OnComplete);
    void CheckAuth(FOnAuthComplete OnComplete);

    // State
    UFUNCTION(BlueprintPure, Category = "horizOn|Auth")
    bool IsSignedIn() const;

    UFUNCTION(BlueprintPure, Category = "horizOn|Auth")
    FHorizonUserData GetCurrentUser() const;

    UFUNCTION(BlueprintCallable, Category = "horizOn|Auth")
    void SignOut();

    // Account management
    void ChangeName(const FString& NewName, FOnRequestComplete OnComplete);
    void ForgotPassword(const FString& Email, FOnRequestComplete OnComplete);
    void VerifyEmail(const FString& Token, FOnRequestComplete OnComplete);
    void ResetPassword(const FString& Token, const FString& NewPassword, FOnRequestComplete OnComplete);

    // Cached token access
    bool HasCachedAnonymousToken() const;
    FString GetCachedAnonymousToken() const;

    // Blueprint events
    UPROPERTY(BlueprintAssignable, Category = "horizOn|Auth|Events")
    FOnHorizonUserSignedIn OnUserSignedIn;

    UPROPERTY(BlueprintAssignable, Category = "horizOn|Auth|Events")
    FOnHorizonUserSignedOut OnUserSignedOut;

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    FHorizonUserData CurrentUser;

    void HandleAuthResponse(const FHorizonNetworkResponse& Response, FOnAuthComplete OnComplete);
    void CacheSession();
    void ClearSession();
};
```

**Step 2: Implement HorizonAuthManager.cpp**

For each auth method:
1. Build JSON request body using `MakeShared<FJsonObject>()`
2. Set fields matching Unity's exact field names
3. Call `HttpClient->PostJson(Endpoint, Body, false, Callback)`
4. In callback: parse response via `CurrentUser.UpdateFromAuthResponse(Response.JsonData)`
5. Set session token via `HttpClient->SetSessionToken(CurrentUser.AccessToken)`
6. Cache session via `UHorizonSessionSave`
7. Broadcast `OnUserSignedIn`

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonAuthManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonAuthManager.cpp
git commit -m "feat: add auth manager (signup, signin, session caching)"
```

---

## Task 9: Cloud Save Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonCloudSaveManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCloudSaveManager.cpp`

**Step 1: Create HorizonCloudSaveManager**

Reference Unity's `CloudSaveManager.cs` for exact endpoints and request/response formats.

**API endpoints:**
- `POST /api/v1/app/cloud-save/save` (JSON body: `{ "userId": "...", "saveData": "..." }`)
- `POST /api/v1/app/cloud-save/load` (JSON body: `{ "userId": "..." }`)
- `POST /api/v1/app/cloud-save/save?userId={userId}` (binary body, Content-Type: application/octet-stream)
- `GET /api/v1/app/cloud-save/load?userId={userId}` (binary response, Accept: application/octet-stream, 204 = not found)

**Response JSON fields:**
- Save: `{ "success": bool, "dataSizeBytes": int }`
- Load: `{ "found": bool, "saveData": "..." }`

```cpp
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonCloudSaveManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

    // JSON string save/load
    void Save(const FString& Data, FOnRequestComplete OnComplete);
    void Load(FOnStringComplete OnComplete);

    // Binary save/load
    void SaveBytes(const TArray<uint8>& Data, FOnRequestComplete OnComplete);
    void LoadBytes(FOnBinaryComplete OnComplete);

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    UPROPERTY()
    UHorizonAuthManager* AuthManager;
};
```

**Step 2: Implement .cpp** using the exact endpoint paths and JSON field names from the Unity SDK.

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonCloudSaveManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonCloudSaveManager.cpp
git commit -m "feat: add cloud save manager (JSON and binary modes)"
```

---

## Task 10: Leaderboard Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonLeaderboardManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonLeaderboardManager.cpp`

**Step 1: Create HorizonLeaderboardManager**

Reference Unity's `LeaderboardManager.cs` for exact endpoints, caching strategy, and response parsing.

**API endpoints:**
- `POST /api/v1/app/leaderboard/submit` (JSON body: `{ "userId": "...", "score": 123 }`)
- `GET /api/v1/app/leaderboard/top?userId={userId}&limit={limit}`
- `GET /api/v1/app/leaderboard/rank?userId={userId}`
- `GET /api/v1/app/leaderboard/around?userId={userId}&range={range}`

**Response JSON fields:**
- Top/Around: `{ "entries": [ { "position": 1, "username": "...", "score": 123 } ] }`
- Rank: `{ "position": 1, "username": "...", "score": 123 }`
- Submit: empty body on 200

**Caching:** `TMap<FString, TArray<FHorizonLeaderboardEntry>>` with keys like `"top10"`, `"around5"`. Cache cleared on `SubmitScore`.

```cpp
DECLARE_DELEGATE_TwoParams(FOnLeaderboardEntriesComplete, bool, const TArray<FHorizonLeaderboardEntry>&);
DECLARE_DELEGATE_TwoParams(FOnLeaderboardRankComplete, bool, const FHorizonLeaderboardEntry&);

UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonLeaderboardManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

    void SubmitScore(int64 Score, FOnRequestComplete OnComplete, const FString& Metadata = TEXT(""));
    void GetTop(int32 Limit, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete);
    void GetRank(bool bUseCache, FOnLeaderboardRankComplete OnComplete);
    void GetAround(int32 Range, bool bUseCache, FOnLeaderboardEntriesComplete OnComplete);

    UFUNCTION(BlueprintCallable, Category = "horizOn|Leaderboard")
    void ClearCache();

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    UPROPERTY()
    UHorizonAuthManager* AuthManager;

    TMap<FString, TArray<FHorizonLeaderboardEntry>> EntriesCache;
    TOptional<FHorizonLeaderboardEntry> RankCache;

    TArray<FHorizonLeaderboardEntry> ParseEntriesArray(const TSharedPtr<FJsonObject>& JsonObject) const;
};
```

**Step 2: Implement .cpp**

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonLeaderboardManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonLeaderboardManager.cpp
git commit -m "feat: add leaderboard manager with caching"
```

---

## Task 11: Remote Config Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonRemoteConfigManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonRemoteConfigManager.cpp`

**Step 1: Create HorizonRemoteConfigManager**

Reference Unity's `RemoteConfigManager.cs` for endpoints and caching.

**API endpoints:**
- `GET /api/v1/app/remote-config/{key}` -- Response: `{ "configKey": "...", "configValue": "...", "found": bool }`
- `GET /api/v1/app/remote-config/all` -- Response: `{ "total": int, "configs": { "key1": "val1", "key2": "val2" } }`

**Cache:** `TMap<FString, FString>` in memory.

```cpp
DECLARE_DELEGATE_TwoParams(FOnConfigComplete, bool, const FString& /*Value*/);
DECLARE_DELEGATE_TwoParams(FOnAllConfigsComplete, bool, const TMap<FString, FString>& /*Configs*/);

UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonRemoteConfigManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient);

    void GetConfig(const FString& Key, bool bUseCache, FOnConfigComplete OnComplete);
    void GetAllConfigs(bool bUseCache, FOnAllConfigsComplete OnComplete);

    // Type-safe getters (C++ convenience)
    void GetString(const FString& Key, const FString& DefaultValue, bool bUseCache, FOnConfigComplete OnComplete);
    void GetInt(const FString& Key, int32 DefaultValue, bool bUseCache, TDelegate<void(bool, int32)> OnComplete);
    void GetFloat(const FString& Key, float DefaultValue, bool bUseCache, TDelegate<void(bool, float)> OnComplete);
    void GetBool(const FString& Key, bool DefaultValue, bool bUseCache, TDelegate<void(bool, bool)> OnComplete);

    UFUNCTION(BlueprintCallable, Category = "horizOn|RemoteConfig")
    void ClearCache();

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    TMap<FString, FString> ConfigCache;
};
```

**Step 2: Implement .cpp**

For `GetAllConfigs`, parse the `"configs"` JSON object by iterating `JsonObject->Values` (Unreal's FJsonObject stores members as a TMap, so dictionary parsing is trivial unlike Unity's JsonUtility).

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonRemoteConfigManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonRemoteConfigManager.cpp
git commit -m "feat: add remote config manager with typed getters and caching"
```

---

## Task 12: News Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonNewsManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonNewsManager.cpp`

**Step 1: Create HorizonNewsManager**

Reference Unity's `NewsManager.cs` for the 5-minute cache expiry, limit capping at 100, and language code filtering.

**API endpoint:**
- `GET /api/v1/app/news?limit={n}&languageCode={code}`
- Response: JSON array of `{ "id", "title", "message", "releaseDate", "languageCode" }`

```cpp
DECLARE_DELEGATE_TwoParams(FOnNewsComplete, bool, const TArray<FHorizonNewsEntry>&);

UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonNewsManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient);

    void LoadNews(int32 Limit, const FString& LanguageCode, bool bUseCache, FOnNewsComplete OnComplete);

    UFUNCTION(BlueprintPure, Category = "horizOn|News")
    FHorizonNewsEntry GetNewsById(const FString& Id) const;

    UFUNCTION(BlueprintCallable, Category = "horizOn|News")
    void ClearCache();

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    TArray<FHorizonNewsEntry> NewsCache;
    double LastFetchTime = 0.0;
    static constexpr double CacheDurationSeconds = 300.0; // 5 minutes

    bool IsCacheExpired() const;
};
```

**Step 2: Implement .cpp**

Use `FPlatformTime::Seconds()` for cache timing (engine-independent monotonic clock).

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonNewsManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonNewsManager.cpp
git commit -m "feat: add news manager with 5-minute cache"
```

---

## Task 13: Gift Code Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonGiftCodeManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonGiftCodeManager.cpp`

**Step 1: Create HorizonGiftCodeManager**

Reference Unity's `GiftCodeManager.cs`.

**API endpoints:**
- `POST /api/v1/app/gift-codes/redeem` (body: `{ "code": "...", "userId": "..." }`)
- `POST /api/v1/app/gift-codes/validate` (body: `{ "code": "...", "userId": "..." }`)

**Response JSON fields:**
- Redeem: `{ "success": bool, "message": "...", "giftData": "..." }`
- Validate: `{ "valid": bool }`

```cpp
DECLARE_DELEGATE_ThreeParams(FOnGiftCodeRedeemComplete, bool /*bSuccess*/, const FString& /*GiftData*/, const FString& /*Message*/);
DECLARE_DELEGATE_TwoParams(FOnGiftCodeValidateComplete, bool /*bSuccess*/, bool /*bValid*/);

UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonGiftCodeManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

    void Redeem(const FString& Code, FOnGiftCodeRedeemComplete OnComplete);
    void Validate(const FString& Code, FOnGiftCodeValidateComplete OnComplete);

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    UPROPERTY()
    UHorizonAuthManager* AuthManager;
};
```

**Step 2: Implement .cpp**

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonGiftCodeManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonGiftCodeManager.cpp
git commit -m "feat: add gift code manager (validate and redeem)"
```

---

## Task 14: Feedback Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonFeedbackManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonFeedbackManager.cpp`

**Step 1: Create HorizonFeedbackManager**

Reference Unity's `FeedbackManager.cs` for device info collection and category constants.

**API endpoint:**
- `POST /api/v1/app/user-feedback/submit`
- Body: `{ "userId": "...", "title": "...", "category": "BUG|FEATURE|GENERAL", "message": "...", "email": "...", "deviceInfo": "..." }`

**Device info format** (Unreal equivalent of Unity's pattern):
```
UE {ENGINE_VERSION} | {OS} | {DEVICE_MODEL} | {GPU}
```
Use `FEngineVersion::Current().ToString()`, `FPlatformMisc::GetOSVersion()`, `FPlatformMisc::GetDeviceMakeAndModel()`, `GRHIAdapterName`.

```cpp
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonFeedbackManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

    void Submit(const FString& Title, const FString& Category, const FString& Message, const FString& Email, bool bIncludeDeviceInfo, FOnRequestComplete OnComplete);
    void ReportBug(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email = TEXT(""));
    void RequestFeature(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email = TEXT(""));
    void SendGeneral(const FString& Title, const FString& Message, FOnRequestComplete OnComplete, const FString& Email = TEXT(""));

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    UPROPERTY()
    UHorizonAuthManager* AuthManager;

    FString CollectDeviceInfo() const;
};
```

**Step 2: Implement .cpp**

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonFeedbackManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonFeedbackManager.cpp
git commit -m "feat: add feedback manager with device info collection"
```

---

## Task 15: User Log Manager

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonUserLogManager.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonUserLogManager.cpp`

**Step 1: Create HorizonUserLogManager**

Reference Unity's `UserLogManager.cs` for validation (message max 1000 chars, errorCode max 50 chars, requires auth, 403 = free tier).

**API endpoint:**
- `POST /api/v1/app/user-logs/create`
- Body: `{ "message": "...", "type": "INFO|WARN|ERROR", "userId": "...", "errorCode": "..." }`
- Response: `{ "id": "...", "createdAt": "..." }`

```cpp
DECLARE_DELEGATE_ThreeParams(FOnUserLogComplete, bool /*bSuccess*/, const FString& /*LogId*/, const FString& /*CreatedAt*/);

UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonUserLogManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

    void CreateLog(EHorizonLogLevel Level, const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));
    void Info(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));
    void Warn(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));
    void Error(const FString& Message, FOnUserLogComplete OnComplete, const FString& ErrorCode = TEXT(""));

private:
    UPROPERTY()
    UHorizonHttpClient* HttpClient;

    UPROPERTY()
    UHorizonAuthManager* AuthManager;
};
```

**Step 2: Implement .cpp** with message truncation at 1000 chars and errorCode truncation at 50 chars.

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Managers/HorizonUserLogManager.h Plugins/HorizonSDK/Source/HorizonSDK/Private/Managers/HorizonUserLogManager.cpp
git commit -m "feat: add user log manager (server-side logging)"
```

---

## Task 16: Wire Up Subsystem to All Managers

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonSubsystem.cpp`
- Modify: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonSubsystem.h` (add includes)

**Step 1: Update HorizonSubsystem.cpp Initialize()**

Add all manager creation and initialization calls:
```cpp
void UHorizonSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    HttpClient = NewObject<UHorizonHttpClient>(this);
    HttpClient->Initialize();

    Auth = NewObject<UHorizonAuthManager>(this);
    Auth->Initialize(HttpClient);

    CloudSave = NewObject<UHorizonCloudSaveManager>(this);
    CloudSave->Initialize(HttpClient, Auth);

    Leaderboard = NewObject<UHorizonLeaderboardManager>(this);
    Leaderboard->Initialize(HttpClient, Auth);

    RemoteConfig = NewObject<UHorizonRemoteConfigManager>(this);
    RemoteConfig->Initialize(HttpClient);

    News = NewObject<UHorizonNewsManager>(this);
    News->Initialize(HttpClient);

    GiftCodes = NewObject<UHorizonGiftCodeManager>(this);
    GiftCodes->Initialize(HttpClient, Auth);

    Feedback = NewObject<UHorizonFeedbackManager>(this);
    Feedback->Initialize(HttpClient, Auth);

    UserLogs = NewObject<UHorizonUserLogManager>(this);
    UserLogs->Initialize(HttpClient, Auth);

    UE_LOG(LogHorizonSDK, Log, TEXT("horizOn SDK initialized"));
}
```

**Step 2: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/
git commit -m "feat: wire all managers into subsystem initialization"
```

---

## Task 17: Blueprint Library (Static Functions)

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonBlueprintLibrary.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonBlueprintLibrary.cpp`

**Step 1: Create HorizonBlueprintLibrary**

Static `UFUNCTION(BlueprintCallable)` wrappers that resolve the subsystem from WorldContextObject. This provides clean Blueprint nodes without requiring the user to get the subsystem manually.

```cpp
UCLASS()
class HORIZONSDK_API UHorizonBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Get the horizOn subsystem from any world context */
    UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
    static UHorizonSubsystem* GetHorizonSubsystem(const UObject* WorldContextObject);

    /** Quick check if SDK is connected */
    UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
    static bool IsHorizonConnected(const UObject* WorldContextObject);

    /** Quick check if user is signed in */
    UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
    static bool IsHorizonSignedIn(const UObject* WorldContextObject);

    /** Get current user data */
    UFUNCTION(BlueprintPure, Category = "horizOn", meta = (WorldContext = "WorldContextObject"))
    static FHorizonUserData GetHorizonCurrentUser(const UObject* WorldContextObject);
};
```

**Step 2: Implement .cpp**

`GetHorizonSubsystem` resolves via:
```cpp
UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
return GI ? GI->GetSubsystem<UHorizonSubsystem>() : nullptr;
```

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/HorizonBlueprintLibrary.h Plugins/HorizonSDK/Source/HorizonSDK/Private/HorizonBlueprintLibrary.cpp
git commit -m "feat: add Blueprint function library for convenient access"
```

---

## Task 18: Blueprint Async Actions

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncConnect.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncConnect.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncAuth.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncAuth.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncCloudSave.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncCloudSave.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncLeaderboard.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncLeaderboard.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncRemoteConfig.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncRemoteConfig.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncNews.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncNews.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncGiftCode.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncGiftCode.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/HorizonAsyncFeedback.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/HorizonAsyncFeedback.cpp`

**Step 1: Create async action classes**

Each class extends `UBlueprintAsyncActionBase` and provides a latent Blueprint node with `OnSuccess` and `OnFailure` output exec pins.

Pattern for each:
```cpp
UCLASS()
class HORIZONSDK_API UHorizonAsyncConnect : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnHorizonConnected OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOnHorizonConnectionFailed OnFailure;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "horizOn")
    static UHorizonAsyncConnect* ConnectToServer(const UObject* WorldContextObject);

    virtual void Activate() override;

private:
    TWeakObjectPtr<const UObject> WorldContext;
};
```

Create async actions for:
- `ConnectToServer`
- `SignUpAnonymous`, `SignUpEmail`, `SignInEmail`, `SignInAnonymous`, `RestoreSession`
- `CloudSaveSave`, `CloudSaveLoad`, `CloudSaveSaveBytes`, `CloudSaveLoadBytes`
- `LeaderboardSubmitScore`, `LeaderboardGetTop`, `LeaderboardGetRank`, `LeaderboardGetAround`
- `RemoteConfigGet`, `RemoteConfigGetAll`
- `NewsLoad`
- `GiftCodeRedeem`, `GiftCodeValidate`
- `FeedbackSubmit`

**Step 2: Implement all .cpp files**

Each `Activate()` resolves the subsystem, calls the manager method, and in the callback broadcasts the appropriate output pin.

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Async/ Plugins/HorizonSDK/Source/HorizonSDK/Private/Async/
git commit -m "feat: add Blueprint async action nodes for all SDK operations"
```

---

## Task 19: Editor Module - Config Importer

**Files:**
- Modify: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Public/HorizonSDKEditorModule.h`
- Modify: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Private/HorizonSDKEditorModule.cpp`
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Private/HorizonConfigImporter.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Private/HorizonConfigImporter.cpp`

**Step 1: Add menu entries to editor module**

Register three menu items under `Tools > horizOn`:
1. Import Config... -- opens file dialog, parses JSON, writes to UHorizonConfig
2. Open SDK Dashboard -- opens editor widget (Task 20)
3. Clear Session Cache -- deletes HorizonSession save game

Use `UToolMenus` for menu registration.

**Step 2: Implement HorizonConfigImporter**

Reference Unity's `ConfigImporter.cs` for the config JSON format:
```json
{
    "apiKey": "horizon_XXXXX...",
    "backendDomains": ["https://horizon.pm"]
}
```

The importer:
1. Opens `IDesktopPlatform::OpenFileDialog` for JSON selection
2. Reads and parses JSON via `FJsonSerializer`
3. Extracts `apiKey` and `backendDomains` (or `backendUrl` for single host)
4. Writes values to `UHorizonConfig` CDO and saves config via `SaveConfig()`
5. Shows confirmation dialog with masked API key

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDKEditor/
git commit -m "feat: add editor config importer and Tools menu entries"
```

---

## Task 20: Editor Utility Widget (Dashboard)

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Private/SHorizonEditorWidget.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDKEditor/Private/SHorizonEditorWidget.cpp`

**Step 1: Create SHorizonEditorWidget as a Slate widget**

A dockable editor tab showing:
- **Config section:** Current API key (masked), hosts list, connection settings (read-only display)
- **Status section:** Connection status indicator, active host, last ping results
- **Actions:** "Test Connection" button that pings the configured hosts

Use `SCompoundWidget` with vertical box layout. Register as a nomad tab via `FGlobalTabmanager`.

**Step 2: Register tab spawner in editor module StartupModule()**

**Step 3: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDKEditor/
git commit -m "feat: add editor dashboard widget with config display and connection test"
```

---

## Task 21: Example Level and UI Widget

**Note:** UMG widgets (.uasset) and maps (.umap) are binary assets that cannot be created from C++ code alone. Instead, create a C++ example class that can be used as a base for a Blueprint widget, and provide setup instructions.

**Files:**
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Public/Example/HorizonExampleWidget.h`
- Create: `Plugins/HorizonSDK/Source/HorizonSDK/Private/Example/HorizonExampleWidget.cpp`

**Step 1: Create HorizonExampleWidget**

A `UUserWidget` subclass that demonstrates all SDK features. It exposes `UFUNCTION(BlueprintCallable)` methods that can be bound to buttons in a Blueprint-derived widget:

```cpp
UCLASS(Blueprintable)
class HORIZONSDK_API UHorizonExampleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Connection
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestConnect();

    // Auth
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestSignUpAnonymous(const FString& DisplayName);

    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestSignInEmail(const FString& Email, const FString& Password);

    // Cloud Save
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestSaveData(const FString& Data);

    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestLoadData();

    // Leaderboard
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestSubmitScore(int64 Score);

    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestGetTopScores(int32 Limit);

    // Remote Config
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestGetAllConfigs();

    // News
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestLoadNews(int32 Limit);

    // Gift Code
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestRedeemCode(const FString& Code);

    // Feedback
    UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
    void TestSubmitFeedback(const FString& Title, const FString& Message);

    // Output display
    UPROPERTY(BlueprintReadOnly, Category = "horizOn|Example")
    FString OutputLog;

    UPROPERTY(BlueprintAssignable, Category = "horizOn|Example")
    FOnOutputUpdated OnOutputUpdated; // Dynamic delegate to refresh UI

protected:
    void AppendOutput(const FString& Text);
};
```

**Step 2: Implement all test methods**

Each method gets the subsystem, calls the appropriate manager, and in the callback appends results to `OutputLog` and broadcasts `OnOutputUpdated`.

**Step 3: Create a README section** documenting how to create the example Blueprint widget and map in the editor.

**Step 4: Commit**

```bash
git add Plugins/HorizonSDK/Source/HorizonSDK/Public/Example/ Plugins/HorizonSDK/Source/HorizonSDK/Private/Example/
git commit -m "feat: add example widget base class for SDK demonstration"
```

---

## Task 22: Plugin Metadata and Documentation

**Files:**
- Create: `Plugins/HorizonSDK/Resources/Icon128.png` (placeholder or actual icon)
- Create: `Plugins/HorizonSDK/README.md`
- Create: `Plugins/HorizonSDK/Docs/QUICKSTART.md`
- Create: `Plugins/HorizonSDK/LICENSE`
- Create: `Plugins/HorizonSDK/CHANGELOG.md`

**Step 1: Create README.md**

Cover:
- What is horizOn SDK
- Features list (8 features)
- Installation (copy to Plugins/, enable in editor)
- Quick start (configure, connect, authenticate)
- Blueprint usage examples
- C++ usage examples
- Link to full docs

**Step 2: Create QUICKSTART.md**

5-minute setup:
1. Copy plugin to `Plugins/HorizonSDK/`
2. Enable plugin in Editor > Plugins
3. Tools > horizOn > Import Config
4. In your GameMode BeginPlay, get subsystem and connect
5. Authenticate
6. Use features

**Step 3: Create LICENSE** (match the Unity/Godot SDK license)

**Step 4: Create CHANGELOG.md** with initial 1.0.0 entry

**Step 5: Commit**

```bash
git add Plugins/HorizonSDK/README.md Plugins/HorizonSDK/Docs/ Plugins/HorizonSDK/LICENSE Plugins/HorizonSDK/CHANGELOG.md
git commit -m "docs: add README, quickstart guide, license, and changelog"
```

---

## Task 23: CLAUDE.md Project Memory

**Files:**
- Create: `CLAUDE.md`

**Step 1: Create CLAUDE.md**

Document project conventions, architecture, build commands, and patterns for future Claude sessions.

Key sections:
- Project overview
- Architecture (subsystem + managers pattern)
- Build/compile instructions
- File organization
- Naming conventions (UE5 standard: `F` prefix for structs, `E` for enums, `U` for UObjects)
- API endpoint reference
- Reference SDK locations

**Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "chore: add CLAUDE.md project memory"
```

---

## Task 24: Final Review and Compilation Check

**Files:**
- All files created in Tasks 1-23

**Step 1: Review all includes and forward declarations**

Ensure every header includes or forward-declares its dependencies. Check for circular includes.

**Step 2: Verify HORIZONSDK_API export macro** on all public classes and structs.

**Step 3: Verify all GENERATED_BODY() macros** are present in UCLASS/USTRUCT/UENUM declarations.

**Step 4: Commit any fixes**

```bash
git add -A
git commit -m "fix: resolve compilation issues from final review"
```

---

## Dependency Graph

```
Task 1 (Scaffold)
  └── Task 2 (Types)
       ├── Task 3 (Models)
       ├── Task 4 (Config)
       └── Task 5 (Session Save)
            └── Task 6 (HTTP Client)
                 └── Task 7 (Subsystem skeleton)
                      ├── Task 8 (Auth Manager)
                      │    ├── Task 9 (Cloud Save) ──────┐
                      │    ├── Task 10 (Leaderboard) ────┤
                      │    ├── Task 13 (Gift Codes) ─────┤
                      │    ├── Task 14 (Feedback) ───────┤
                      │    └── Task 15 (User Logs) ──────┤
                      ├── Task 11 (Remote Config) ───────┤
                      └── Task 12 (News) ────────────────┤
                                                         │
                      Task 16 (Wire subsystem) ←─────────┘
                           │
                      Task 17 (BP Library)
                           │
                      Task 18 (BP Async Actions)
                           │
                      ┌────┴────┐
                      │         │
                 Task 19    Task 21
                 (Editor)   (Example)
                      │
                 Task 20
                 (Dashboard)
                      │
                 Task 22 (Docs)
                      │
                 Task 23 (CLAUDE.md)
                      │
                 Task 24 (Final Review)
```

**Note:** Tasks 9-15 can be implemented in parallel since they are independent managers that only depend on the HTTP client and Auth manager being in place.
