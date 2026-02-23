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

UENUM(BlueprintType)
enum class EHorizonCrashType : uint8
{
    Crash       UMETA(DisplayName = "Crash"),
    NonFatal    UMETA(DisplayName = "Non-Fatal"),
    Anr         UMETA(DisplayName = "ANR")
};

// --- Dynamic Multicast Delegates (Blueprint-bindable events) ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonConnectionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonUserSignedIn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonUserSignedOut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonOutputUpdated, const FString&, OutputText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHorizonCrashReported, const FString&, ReportId, const FString&, GroupId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonCrashReportFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonCrashSessionRegistered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonBreadcrumbRecorded, const FString&, Message);

// --- C++ Delegates (non-Blueprint, for manager callbacks) ---

DECLARE_DELEGATE_OneParam(FOnAuthComplete, bool /*bSuccess*/);
DECLARE_DELEGATE_TwoParams(FOnRequestComplete, bool /*bSuccess*/, const FString& /*ErrorMessage*/);
DECLARE_DELEGATE_TwoParams(FOnStringComplete, bool /*bSuccess*/, const FString& /*Data*/);
DECLARE_DELEGATE_TwoParams(FOnBinaryComplete, bool /*bSuccess*/, const TArray<uint8>& /*Data*/);
DECLARE_DELEGATE_ThreeParams(FOnCrashReportComplete, bool /*bSuccess*/, const FString& /*ReportId*/, const FString& /*GroupId*/);
DECLARE_DELEGATE_OneParam(FOnCrashSessionComplete, bool /*bSuccess*/);
