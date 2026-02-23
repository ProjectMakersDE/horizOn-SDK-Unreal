// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Misc/OutputDevice.h"
#include "HorizonCrashManager.generated.h"

/**
 * Crash Report Manager for the horizOn SDK.
 *
 * Captures unhandled engine errors, non-fatal exceptions, and contextual data
 * (breadcrumbs, device info, custom keys), then sends structured crash reports
 * to the horizOn backend. Supports automatic breadcrumb recording from SDK
 * events and rate-limited report submission.
 */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonCrashManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with HTTP client and auth manager references. */
	void Initialize(UHorizonHttpClient* InHttpClient, UHorizonAuthManager* InAuthManager);

	/** Ensure crash hooks are cleaned up before GC. */
	virtual void BeginDestroy() override;

	// --- Lifecycle ---

	/** Begin crash capture: registers a session, hooks engine error handlers. */
	void StartCapture();

	/** Stop crash capture: unhooks error handlers. */
	void StopCapture();

	// --- Manual Reporting ---

	/**
	 * Record a non-fatal exception.
	 * @param Message     Error message (max 5000 chars).
	 * @param StackTrace  Optional stack trace string.
	 * @param ExtraKeys   Optional additional key-value metadata merged with custom keys.
	 * @param OnComplete  Called with (bSuccess, ReportId, GroupId).
	 */
	void RecordException(
		const FString& Message,
		const FString& StackTrace = TEXT(""),
		const TMap<FString, FString>& ExtraKeys = {},
		FOnCrashReportComplete OnComplete = {});

	/**
	 * Report a fatal crash.
	 * @param Message     Error message (max 5000 chars).
	 * @param StackTrace  Optional stack trace string.
	 * @param OnComplete  Called with (bSuccess, ReportId, GroupId).
	 */
	void ReportCrash(
		const FString& Message,
		const FString& StackTrace = TEXT(""),
		FOnCrashReportComplete OnComplete = {});

	// --- Context ---

	/** Record a breadcrumb with a type and message. Stored in a ring buffer. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void RecordBreadcrumb(const FString& Type, const FString& Message);

	/** Convenience: record a breadcrumb with type "log". */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void Log(const FString& Message);

	/** Set a custom key-value pair attached to future crash reports. Max 10 keys. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void SetCustomKey(const FString& Key, const FString& Value);

	/** Override the user ID sent with crash reports. If empty, uses AuthManager. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|CrashReport")
	void SetUserId(const FString& UserId);

	// --- State ---

	/** Returns true if crash capture is currently active. */
	UFUNCTION(BlueprintPure, Category = "horizOn|CrashReport")
	bool IsCapturing() const { return bIsCapturing; }

	// --- Events ---

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonCrashReported OnCrashReported;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonCrashReportFailed OnCrashReportFailed;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonCrashSessionRegistered OnSessionRegistered;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|CrashReport|Events")
	FOnHorizonBreadcrumbRecorded OnBreadcrumbRecorded;

private:
	// --- Dependencies ---

	UPROPERTY()
	UHorizonHttpClient* HttpClient = nullptr;

	UPROPERTY()
	UHorizonAuthManager* AuthManager = nullptr;

	// --- State ---

	bool bIsCapturing = false;
	FString SessionId;
	FString UserIdOverride;

	// --- Device info (cached once on Initialize) ---

	FString CachedPlatform;
	FString CachedOS;
	FString CachedDeviceModel;
	int32 CachedDeviceMemoryMb = 0;

	// --- SDK version ---

	static const FString SdkVersion;

	// --- Breadcrumb ring buffer ---

	static constexpr int32 MaxBreadcrumbs = 50;

	struct FBreadcrumb
	{
		FString Timestamp;
		FString Type;
		FString Message;
	};

	TArray<FBreadcrumb> Breadcrumbs;
	int32 BreadcrumbHead = 0;
	int32 BreadcrumbCount = 0;

	// --- Custom keys ---

	static constexpr int32 MaxCustomKeys = 10;
	TMap<FString, FString> CustomKeys;

	// --- Rate limiting ---

	static constexpr int32 TokensPerMinute = 5;
	static constexpr int32 MaxSessionReports = 20;
	static constexpr double RefillIntervalSeconds = 60.0;
	double Tokens = 5.0;
	double LastRefillTime = 0.0;
	int32 SessionReportCount = 0;

	// --- Internal methods ---

	void CacheDeviceInfo();
	FString GenerateSessionId() const;
	FString GenerateFingerprint(const FString& StackTrace) const;
	FString NormalizeFrame(const FString& Frame) const;
	bool IsEngineFrame(const FString& Frame) const;
	FString HashSHA256(const FString& Input) const;
	void RefillTokens();
	bool ConsumeRateLimitToken();
	FString ResolveUserId() const;
	TArray<TSharedRef<FJsonObject>> CollectBreadcrumbsJson() const;
	void AddBreadcrumb(const FString& Type, const FString& Message);
	void RegisterSession();

	void SubmitReport(
		EHorizonCrashType Type,
		const FString& Message,
		const FString& StackTrace,
		const TMap<FString, FString>& ExtraKeys,
		FOnCrashReportComplete OnComplete);

	/** Convert EHorizonCrashType to the server-expected string. */
	static FString CrashTypeToString(EHorizonCrashType Type);

	// --- Custom FOutputDevice for log interception ---

	class FHorizonCrashOutputDevice : public FOutputDevice
	{
	public:
		FHorizonCrashOutputDevice(UHorizonCrashManager* InOwner);
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

	private:
		// Raw pointer: non-UObject class cannot use UPROPERTY.
		// Lifetime safe because TUniquePtr<OutputDevice> is destroyed in StopCapture()/BeginDestroy().
		UHorizonCrashManager* Owner;
		bool bInSerialize = false;
	};

	// --- Auto-breadcrumb handlers (bound to subsystem delegate events) ---

	UFUNCTION()
	void OnAutoUserSignedIn();

	UFUNCTION()
	void OnAutoUserSignedOut();

	UFUNCTION()
	void OnAutoConnected();

	UFUNCTION()
	void OnAutoConnectionFailed(const FString& ErrorMessage);

	// --- Engine error hook ---

	FDelegateHandle SystemErrorHandle;
	TUniquePtr<FHorizonCrashOutputDevice> OutputDevice;

	void OnEngineError();
	void OnLogMessage(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category);
};
