// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HorizonTypes.h"
#include "HorizonExampleWidget.generated.h"

class UHorizonSubsystem;

/**
 * Example UMG widget that demonstrates every horizOn SDK feature.
 *
 * Drop this widget (or a Blueprint subclass) into a level to quickly
 * test connectivity, authentication, cloud save, leaderboard,
 * remote config, news, gift codes, and feedback.
 *
 * Each Test*() method calls the corresponding manager, logs the
 * result into OutputLog, and broadcasts OnOutputUpdated so the
 * UI can refresh.
 */
UCLASS(Blueprintable)
class HORIZONSDK_API UHorizonExampleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Test server connectivity (calls ConnectToServer). */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestConnect();

	/** Test anonymous sign-up. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSignUpAnonymous(const FString& DisplayName);

	/** Test email sign-in. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSignInEmail(const FString& Email, const FString& Password);

	/** Test saving string data to cloud. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSaveData(const FString& Data);

	/** Test loading string data from cloud. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestLoadData();

	/** Test submitting a leaderboard score. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSubmitScore(int64 Score);

	/** Test retrieving top leaderboard scores. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestGetTopScores(int32 Limit);

	/** Test fetching all remote config values. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestGetAllConfigs();

	/** Test loading news entries. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestLoadNews(int32 Limit);

	/** Test redeeming a gift code. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestRedeemCode(const FString& Code);

	/** Test submitting feedback. */
	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSubmitFeedback(const FString& Title, const FString& Message);

	/** Accumulated output log from all test operations. */
	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Example")
	FString OutputLog;

	/** Broadcast whenever OutputLog is updated. */
	UPROPERTY(BlueprintAssignable, Category = "horizOn|Example")
	FOnHorizonOutputUpdated OnOutputUpdated;

protected:
	/** Append a line to OutputLog and broadcast. */
	void AppendOutput(const FString& Text);

	/** Get the HorizonSubsystem from the owning player's GameInstance. */
	UHorizonSubsystem* GetSubsystem() const;
};
