// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_Crash.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrashReportAsyncSuccess, const FString&, ReportId, const FString&, GroupId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrashReportAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Record Exception (NON_FATAL).
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CrashRecordException : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncFailure OnFailure;

	/** Record a non-fatal exception. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Record Exception"), Category = "horizOn|CrashReport")
	static UHorizonAsync_CrashRecordException* RecordException(const UObject* WorldContextObject, const FString& Message, const FString& StackTrace);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString ParamMessage;
	FString ParamStackTrace;

	void HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId);
};

/**
 * Async Blueprint node: Report Crash (CRASH).
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_CrashReportCrash : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnCrashReportAsyncFailure OnFailure;

	/** Report a fatal crash. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Report Crash"), Category = "horizOn|CrashReport")
	static UHorizonAsync_CrashReportCrash* ReportCrash(const UObject* WorldContextObject, const FString& Message, const FString& StackTrace);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	FString ParamMessage;
	FString ParamStackTrace;

	void HandleResult(bool bSuccess, const FString& ReportId, const FString& GroupId);
};
