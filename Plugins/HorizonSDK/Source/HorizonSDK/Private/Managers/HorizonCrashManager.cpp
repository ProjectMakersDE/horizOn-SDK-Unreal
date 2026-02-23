// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonCrashManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformStackWalk.h"
#include "Misc/App.h"
#include "Misc/SecureHash.h"
#include "Misc/CoreDelegates.h"
#include "Internationalization/Regex.h"

// ============================================================
// Static Members
// ============================================================

const FString UHorizonCrashManager::SdkVersion = TEXT("unreal-1.0.0");

// ============================================================
// Section 1: Initialize + CacheDeviceInfo
// ============================================================

void UHorizonCrashManager::Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager)
{
	HttpClient = InHttpClient;
	AuthManager = InAuthManager;

	CacheDeviceInfo();

	// Pre-allocate breadcrumb ring buffer
	Breadcrumbs.SetNum(MaxBreadcrumbs);
	BreadcrumbHead = 0;
	BreadcrumbCount = 0;

	// Initialize rate limiter
	Tokens = static_cast<double>(TokensPerMinute);
	LastRefillTime = FPlatformTime::Seconds();

	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonCrashManager initialized."));
}

void UHorizonCrashManager::CacheDeviceInfo()
{
	CachedPlatform = FPlatformProperties::PlatformName();
	CachedOS = FPlatformMisc::GetOSVersion();
	CachedDeviceModel = FPlatformMisc::GetDeviceMakeAndModel();

	const FPlatformMemoryConstants& MemConstants = FPlatformMemory::GetConstants();
	CachedDeviceMemoryMb = static_cast<int32>(MemConstants.TotalPhysical / (1024ULL * 1024ULL));
}

void UHorizonCrashManager::BeginDestroy()
{
	if (bIsCapturing)
	{
		StopCapture();
	}
	Super::BeginDestroy();
}

// ============================================================
// Section 2: Session ID + User ID + CrashType Helpers
// ============================================================

FString UHorizonCrashManager::GenerateSessionId() const
{
	return FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
}

FString UHorizonCrashManager::ResolveUserId() const
{
	if (!UserIdOverride.IsEmpty())
	{
		return UserIdOverride;
	}

	if (AuthManager && AuthManager->IsSignedIn())
	{
		return AuthManager->GetCurrentUser().UserId;
	}

	return FString();
}

void UHorizonCrashManager::SetUserId(const FString& UserId)
{
	UserIdOverride = UserId;
}

FString UHorizonCrashManager::CrashTypeToString(EHorizonCrashType Type)
{
	switch (Type)
	{
	case EHorizonCrashType::Crash:
		return TEXT("CRASH");
	case EHorizonCrashType::NonFatal:
		return TEXT("NON_FATAL");
	case EHorizonCrashType::Anr:
		return TEXT("ANR");
	default:
		return TEXT("CRASH");
	}
}

// ============================================================
// Section 3: Breadcrumb Ring Buffer
// ============================================================

void UHorizonCrashManager::AddBreadcrumb(const FString& Type, const FString& Message)
{
	FBreadcrumb& Entry = Breadcrumbs[BreadcrumbHead];
	Entry.Timestamp = FDateTime::UtcNow().ToIso8601();
	Entry.Type = Type;
	Entry.Message = Message;

	BreadcrumbHead = (BreadcrumbHead + 1) % MaxBreadcrumbs;

	if (BreadcrumbCount < MaxBreadcrumbs)
	{
		++BreadcrumbCount;
	}
}

void UHorizonCrashManager::RecordBreadcrumb(const FString& Type, const FString& Message)
{
	AddBreadcrumb(Type, Message);
	OnBreadcrumbRecorded.Broadcast(Message);
}

void UHorizonCrashManager::Log(const FString& Message)
{
	RecordBreadcrumb(TEXT("log"), Message);
}

TArray<TSharedRef<FJsonObject>> UHorizonCrashManager::CollectBreadcrumbsJson() const
{
	TArray<TSharedRef<FJsonObject>> Result;
	Result.Reserve(BreadcrumbCount);

	if (BreadcrumbCount <= 0)
	{
		return Result;
	}

	if (BreadcrumbCount < MaxBreadcrumbs)
	{
		// Buffer hasn't wrapped yet - read linearly from 0 to count-1
		for (int32 i = 0; i < BreadcrumbCount; ++i)
		{
			const FBreadcrumb& Entry = Breadcrumbs[i];
			TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("timestamp"), Entry.Timestamp);
			Obj->SetStringField(TEXT("type"), Entry.Type);
			Obj->SetStringField(TEXT("message"), Entry.Message);
			Result.Add(Obj);
		}
	}
	else
	{
		// Buffer has wrapped - start from BreadcrumbHead (oldest) and wrap around
		for (int32 i = 0; i < MaxBreadcrumbs; ++i)
		{
			int32 Index = (BreadcrumbHead + i) % MaxBreadcrumbs;
			const FBreadcrumb& Entry = Breadcrumbs[Index];
			TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("timestamp"), Entry.Timestamp);
			Obj->SetStringField(TEXT("type"), Entry.Type);
			Obj->SetStringField(TEXT("message"), Entry.Message);
			Result.Add(Obj);
		}
	}

	return Result;
}

// ============================================================
// Section 4: Custom Keys + Rate Limiting
// ============================================================

void UHorizonCrashManager::SetCustomKey(const FString& Key, const FString& Value)
{
	if (Key.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::SetCustomKey -- Key cannot be empty."));
		return;
	}

	// Allow updates to existing keys even if at the limit
	if (!CustomKeys.Contains(Key) && CustomKeys.Num() >= MaxCustomKeys)
	{
		UE_LOG(LogHorizonSDK, Warning,
			TEXT("CrashManager::SetCustomKey -- Maximum of %d custom keys reached. Cannot add key '%s'."),
			MaxCustomKeys, *Key);
		return;
	}

	CustomKeys.Add(Key, Value);
}

void UHorizonCrashManager::RefillTokens()
{
	const double Now = FPlatformTime::Seconds();
	const double Elapsed = Now - LastRefillTime;

	if (Elapsed < RefillIntervalSeconds)
	{
		return;
	}

	const int32 Refills = static_cast<int32>(FMath::FloorToDouble(Elapsed / RefillIntervalSeconds));
	Tokens = FMath::Min(Tokens + static_cast<double>(Refills * TokensPerMinute), static_cast<double>(TokensPerMinute));
	LastRefillTime += Refills * RefillIntervalSeconds;
}

bool UHorizonCrashManager::ConsumeRateLimitToken()
{
	RefillTokens();

	if (Tokens < 1.0)
	{
		const FString Error = TEXT("Rate limit exceeded. Too many crash reports per minute.");
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager -- %s"), *Error);
		OnCrashReportFailed.Broadcast(Error);
		return false;
	}

	if (SessionReportCount >= MaxSessionReports)
	{
		const FString Error = FString::Printf(
			TEXT("Session report limit reached (%d/%d)."), SessionReportCount, MaxSessionReports);
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager -- %s"), *Error);
		OnCrashReportFailed.Broadcast(Error);
		return false;
	}

	Tokens -= 1.0;
	++SessionReportCount;
	return true;
}

// ============================================================
// Section 5: Fingerprint Generation
// ============================================================

bool UHorizonCrashManager::IsEngineFrame(const FString& Frame) const
{
	static const TArray<FString> EnginePrefixes = {
		TEXT("FGenericPlatformMisc"),
		TEXT("FWindowsPlatformStackWalk"),
		TEXT("FWindowsPlatformMisc"),
		TEXT("FLinuxPlatformStackWalk"),
		TEXT("FLinuxPlatformMisc"),
		TEXT("FMacPlatformStackWalk"),
		TEXT("FMacPlatformMisc"),
		TEXT("UnrealEngine"),
		TEXT("CoreUObject"),
		TEXT("Core_"),
		TEXT("Engine_"),
		TEXT("FDebug::"),
		TEXT("FOutputDevice::"),
		TEXT("raise"),
		TEXT("abort"),
		TEXT("CommonUnixCrashHandler")
	};

	const FString Trimmed = Frame.TrimStartAndEnd();
	for (const FString& Prefix : EnginePrefixes)
	{
		if (Trimmed.StartsWith(Prefix))
		{
			return true;
		}
	}
	return false;
}

FString UHorizonCrashManager::NormalizeFrame(const FString& Frame) const
{
	FString Normalized = Frame.TrimStartAndEnd();

	// Strip file paths (everything from '(' onward)
	{
		const FRegexPattern Pattern(TEXT("\\(.*$"));
		FRegexMatcher Matcher(Pattern, Normalized);
		if (Matcher.FindNext())
		{
			Normalized = Normalized.Left(Matcher.GetMatchBeginning()).TrimEnd();
		}
	}

	// Strip 0x hex addresses
	{
		const FRegexPattern Pattern(TEXT("0x[0-9a-fA-F]+"));
		FRegexMatcher Matcher(Pattern, Normalized);
		FString Result;
		int32 LastEnd = 0;
		while (Matcher.FindNext())
		{
			Result += Normalized.Mid(LastEnd, Matcher.GetMatchBeginning() - LastEnd);
			LastEnd = Matcher.GetMatchEnding();
		}
		Result += Normalized.Mid(LastEnd);
		Normalized = Result;
	}

	// Strip :digits line numbers
	{
		const FRegexPattern Pattern(TEXT(":\\d+"));
		FRegexMatcher Matcher(Pattern, Normalized);
		FString Result;
		int32 LastEnd = 0;
		while (Matcher.FindNext())
		{
			Result += Normalized.Mid(LastEnd, Matcher.GetMatchBeginning() - LastEnd);
			LastEnd = Matcher.GetMatchEnding();
		}
		Result += Normalized.Mid(LastEnd);
		Normalized = Result;
	}

	// Strip <...> lambda markers
	{
		const FRegexPattern Pattern(TEXT("<[^>]*>"));
		FRegexMatcher Matcher(Pattern, Normalized);
		FString Result;
		int32 LastEnd = 0;
		while (Matcher.FindNext())
		{
			Result += Normalized.Mid(LastEnd, Matcher.GetMatchBeginning() - LastEnd);
			LastEnd = Matcher.GetMatchEnding();
		}
		Result += Normalized.Mid(LastEnd);
		Normalized = Result;
	}

	// Collapse whitespace
	{
		const FRegexPattern Pattern(TEXT("\\s+"));
		FRegexMatcher Matcher(Pattern, Normalized);
		FString Result;
		int32 LastEnd = 0;
		while (Matcher.FindNext())
		{
			Result += Normalized.Mid(LastEnd, Matcher.GetMatchBeginning() - LastEnd);
			Result += TEXT(" ");
			LastEnd = Matcher.GetMatchEnding();
		}
		Result += Normalized.Mid(LastEnd);
		Normalized = Result.TrimStartAndEnd();
	}

	return Normalized;
}

FString UHorizonCrashManager::HashSHA256(const FString& Input) const
{
	const FTCHARToUTF8 Utf8(*Input);
	FSHA256Signature Signature;
	FSHA256::HashBuffer(Utf8.Get(), Utf8.Length(), Signature.Bytes);
	return Signature.ToString();
}

FString UHorizonCrashManager::GenerateFingerprint(const FString& StackTrace) const
{
	if (StackTrace.IsEmpty())
	{
		return HashSHA256(TEXT("no_stack_trace"));
	}

	// Parse into lines
	TArray<FString> Lines;
	StackTrace.ParseIntoArrayLines(Lines);

	// Filter out engine frames and normalize game frames
	TArray<FString> GameFrames;
	TArray<FString> AllNormalized;

	for (const FString& Line : Lines)
	{
		const FString Trimmed = Line.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			continue;
		}

		const FString Normalized = NormalizeFrame(Trimmed);
		if (Normalized.IsEmpty())
		{
			continue;
		}

		AllNormalized.Add(Normalized);

		if (!IsEngineFrame(Trimmed))
		{
			GameFrames.Add(Normalized);
		}
	}

	// Take top 5 game frames
	if (GameFrames.Num() > 0)
	{
		const int32 Count = FMath::Min(GameFrames.Num(), 5);
		FString Combined;
		for (int32 i = 0; i < Count; ++i)
		{
			if (i > 0)
			{
				Combined += TEXT("|");
			}
			Combined += GameFrames[i];
		}
		return HashSHA256(Combined);
	}

	// Fallback: use all normalized frames
	if (AllNormalized.Num() > 0)
	{
		const int32 Count = FMath::Min(AllNormalized.Num(), 5);
		FString Combined;
		for (int32 i = 0; i < Count; ++i)
		{
			if (i > 0)
			{
				Combined += TEXT("|");
			}
			Combined += AllNormalized[i];
		}
		return HashSHA256(Combined);
	}

	return HashSHA256(TEXT("no_stack_trace"));
}

// ============================================================
// Section 6: Session Registration + Report Submission
// ============================================================

void UHorizonCrashManager::RegisterSession()
{
	if (!HttpClient)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::RegisterSession -- HttpClient is null."));
		return;
	}

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("sessionId"), SessionId);
	Body->SetStringField(TEXT("appVersion"), FApp::GetBuildVersion());
	Body->SetStringField(TEXT("platform"), CachedPlatform);

	const FString UserId = ResolveUserId();
	if (!UserId.IsEmpty())
	{
		Body->SetStringField(TEXT("userId"), UserId);
	}

	TWeakObjectPtr<UHorizonCrashManager> WeakSelf(this);

	HttpClient->PostJson(Body, TEXT("api/v1/app/crash-reports/session"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager::RegisterSession -- Session registered successfully."));
					WeakSelf->OnSessionRegistered.Broadcast();
				}
				else
				{
					UE_LOG(LogHorizonSDK, Warning,
						TEXT("CrashManager::RegisterSession -- Failed: %s"), *Response.ErrorMessage);
				}
			}
		));
}

void UHorizonCrashManager::SubmitReport(
	EHorizonCrashType Type,
	const FString& Message,
	const FString& StackTrace,
	const TMap<FString, FString>& ExtraKeys,
	FOnCrashReportComplete OnComplete)
{
	if (Message.IsEmpty())
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::SubmitReport -- Message cannot be empty."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	if (!ConsumeRateLimitToken())
	{
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	if (!HttpClient)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::SubmitReport -- HttpClient is null."));
		OnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
		return;
	}

	// Build JSON body
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("type"), CrashTypeToString(Type));
	Body->SetStringField(TEXT("message"), Message.Left(5000));
	Body->SetStringField(TEXT("fingerprint"), GenerateFingerprint(StackTrace));
	Body->SetStringField(TEXT("appVersion"), FApp::GetBuildVersion());
	Body->SetStringField(TEXT("sdkVersion"), SdkVersion);
	Body->SetStringField(TEXT("platform"), CachedPlatform);
	Body->SetStringField(TEXT("os"), CachedOS);
	Body->SetStringField(TEXT("deviceModel"), CachedDeviceModel);
	Body->SetNumberField(TEXT("deviceMemoryMb"), CachedDeviceMemoryMb);
	Body->SetStringField(TEXT("sessionId"), SessionId);

	const FString UserId = ResolveUserId();
	if (!UserId.IsEmpty())
	{
		Body->SetStringField(TEXT("userId"), UserId);
	}

	if (!StackTrace.IsEmpty())
	{
		Body->SetStringField(TEXT("stackTrace"), StackTrace.Left(20000));
	}

	// Breadcrumbs array
	TArray<TSharedRef<FJsonObject>> BreadcrumbObjects = CollectBreadcrumbsJson();
	if (BreadcrumbObjects.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> BreadcrumbValues;
		BreadcrumbValues.Reserve(BreadcrumbObjects.Num());
		for (const TSharedRef<FJsonObject>& Obj : BreadcrumbObjects)
		{
			BreadcrumbValues.Add(MakeShared<FJsonValueObject>(Obj));
		}
		Body->SetArrayField(TEXT("breadcrumbs"), BreadcrumbValues);
	}

	// Custom keys: merge persistent + extra
	TMap<FString, FString> MergedKeys = CustomKeys;
	for (const TPair<FString, FString>& Pair : ExtraKeys)
	{
		MergedKeys.Add(Pair.Key, Pair.Value);
	}
	if (MergedKeys.Num() > 0)
	{
		TSharedRef<FJsonObject> KeysObject = MakeShared<FJsonObject>();
		for (const TPair<FString, FString>& Pair : MergedKeys)
		{
			KeysObject->SetStringField(Pair.Key, Pair.Value);
		}
		Body->SetObjectField(TEXT("customKeys"), KeysObject);
	}

	TWeakObjectPtr<UHorizonCrashManager> WeakSelf(this);
	FOnCrashReportComplete CapturedOnComplete = OnComplete;

	HttpClient->PostJson(Body, TEXT("api/v1/app/crash-reports/create"), false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				if (!WeakSelf.IsValid())
				{
					return;
				}

				if (Response.bSuccess)
				{
					FString ReportId;
					FString GroupId;

					if (Response.JsonData.IsValid())
					{
						Response.JsonData->TryGetStringField(TEXT("id"), ReportId);
						Response.JsonData->TryGetStringField(TEXT("groupId"), GroupId);
					}

					UE_LOG(LogHorizonSDK, Log,
						TEXT("CrashManager::SubmitReport -- Report submitted (id=%s, groupId=%s)."),
						*ReportId, *GroupId);

					WeakSelf->OnCrashReported.Broadcast(ReportId, GroupId);
					CapturedOnComplete.ExecuteIfBound(true, ReportId, GroupId);
				}
				else
				{
					FString ErrorMsg = Response.ErrorMessage;

					if (Response.StatusCode == 403)
					{
						ErrorMsg = TEXT("Crash reporting not available for FREE accounts");
					}

					UE_LOG(LogHorizonSDK, Warning,
						TEXT("CrashManager::SubmitReport -- Failed: %s"), *ErrorMsg);

					WeakSelf->OnCrashReportFailed.Broadcast(ErrorMsg);
					CapturedOnComplete.ExecuteIfBound(false, TEXT(""), TEXT(""));
				}
			}
		));
}

// ============================================================
// Section 7: FOutputDevice + Crash Hooks + Lifecycle
// ============================================================

UHorizonCrashManager::FHorizonCrashOutputDevice::FHorizonCrashOutputDevice(UHorizonCrashManager* InOwner)
	: Owner(InOwner)
{
}

void UHorizonCrashManager::FHorizonCrashOutputDevice::Serialize(
	const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	// Guard against re-entrant calls from SubmitReport's own logging
	if (bInSerialize || !Owner || !Owner->bIsCapturing)
	{
		return;
	}

	// Only process Error and Fatal
	if (Verbosity != ELogVerbosity::Error && Verbosity != ELogVerbosity::Fatal)
	{
		return;
	}

	// Also skip our own log category as a secondary guard
	if (Category == TEXT("LogHorizonSDK"))
	{
		return;
	}

	TGuardValue<bool> Guard(bInSerialize, true);
	Owner->OnLogMessage(V, Verbosity, Category);
}

void UHorizonCrashManager::OnLogMessage(
	const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category)
{
	const EHorizonCrashType Type = (Verbosity == ELogVerbosity::Fatal)
		? EHorizonCrashType::Crash
		: EHorizonCrashType::NonFatal;

	const FString MessageStr = FString(Message);

	SubmitReport(Type, MessageStr, FString(), TMap<FString, FString>(), FOnCrashReportComplete());
}

void UHorizonCrashManager::OnEngineError()
{
	// Attempt to capture stack trace
	FString StackTrace;
	{
		ANSICHAR Buffer[4096];
		Buffer[0] = '\0';
		FPlatformStackWalk::StackWalkAndDump(Buffer, UE_ARRAY_COUNT(Buffer), 2);
		StackTrace = ANSI_TO_TCHAR(Buffer);
	}

	SubmitReport(
		EHorizonCrashType::Crash,
		TEXT("Unhandled engine error"),
		StackTrace,
		TMap<FString, FString>(),
		FOnCrashReportComplete());
}

void UHorizonCrashManager::StartCapture()
{
	if (bIsCapturing)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::StartCapture -- Already capturing."));
		return;
	}

	bIsCapturing = true;

	// Generate a new session ID
	SessionId = GenerateSessionId();

	// Reset rate limiter state
	SessionReportCount = 0;
	Tokens = static_cast<double>(TokensPerMinute);
	LastRefillTime = FPlatformTime::Seconds();

	// Register the session with the backend
	RegisterSession();

	// Hook into system error handler
	SystemErrorHandle = FCoreDelegates::OnHandleSystemError.AddUObject(this, &UHorizonCrashManager::OnEngineError);

	// Create and register our output device for log interception
	OutputDevice = MakeUnique<FHorizonCrashOutputDevice>(this);
	GLog->AddOutputDevice(OutputDevice.Get());

	UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager::StartCapture -- Capture started (session=%s)."), *SessionId);
}

void UHorizonCrashManager::StopCapture()
{
	if (!bIsCapturing)
	{
		UE_LOG(LogHorizonSDK, Warning, TEXT("CrashManager::StopCapture -- Not currently capturing."));
		return;
	}

	bIsCapturing = false;

	// Remove system error hook
	if (SystemErrorHandle.IsValid())
	{
		FCoreDelegates::OnHandleSystemError.Remove(SystemErrorHandle);
		SystemErrorHandle.Reset();
	}

	// Remove and destroy output device
	if (OutputDevice.IsValid())
	{
		GLog->RemoveOutputDevice(OutputDevice.Get());
		OutputDevice.Reset();
	}

	UE_LOG(LogHorizonSDK, Log, TEXT("CrashManager::StopCapture -- Capture stopped."));
}

// ============================================================
// Section 8: Public API + Auto-Breadcrumbs
// ============================================================

void UHorizonCrashManager::RecordException(
	const FString& Message,
	const FString& StackTrace,
	const TMap<FString, FString>& ExtraKeys,
	FOnCrashReportComplete OnComplete)
{
	SubmitReport(EHorizonCrashType::NonFatal, Message, StackTrace, ExtraKeys, OnComplete);
}

void UHorizonCrashManager::ReportCrash(
	const FString& Message,
	const FString& StackTrace,
	FOnCrashReportComplete OnComplete)
{
	SubmitReport(EHorizonCrashType::Crash, Message, StackTrace, TMap<FString, FString>(), OnComplete);
}

void UHorizonCrashManager::OnAutoUserSignedIn()
{
	AddBreadcrumb(TEXT("user"), TEXT("User signed in"));
}

void UHorizonCrashManager::OnAutoUserSignedOut()
{
	AddBreadcrumb(TEXT("user"), TEXT("User signed out"));
}

void UHorizonCrashManager::OnAutoConnected()
{
	AddBreadcrumb(TEXT("network"), TEXT("Connected to server"));
}

void UHorizonCrashManager::OnAutoConnectionFailed(const FString& ErrorMessage)
{
	AddBreadcrumb(TEXT("network"), FString::Printf(TEXT("Connection failed: %s"), *ErrorMessage));
}
