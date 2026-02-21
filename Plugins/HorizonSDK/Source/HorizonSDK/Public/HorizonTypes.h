#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.generated.h"

// ============================================================
// horizOn SDK - Shared Types
// ============================================================

// --- Enums ---

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

// --- Dynamic Multicast Delegates (Blueprint-bindable events) ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonConnectionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonUserSignedIn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonUserSignedOut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonOutputUpdated, const FString&, OutputText);

// --- C++ Delegates (non-Blueprint, for manager callbacks) ---

DECLARE_DELEGATE_OneParam(FOnAuthComplete, bool /*bSuccess*/);
DECLARE_DELEGATE_TwoParams(FOnRequestComplete, bool /*bSuccess*/, const FString& /*ErrorMessage*/);
DECLARE_DELEGATE_TwoParams(FOnStringComplete, bool /*bSuccess*/, const FString& /*Data*/);
DECLARE_DELEGATE_TwoParams(FOnBinaryComplete, bool /*bSuccess*/, const TArray<uint8>& /*Data*/);
