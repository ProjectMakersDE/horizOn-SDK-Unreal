// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HorizonTypes.h"
#include "HorizonExampleWidget.generated.h"

class UHorizonSubsystem;
class UVerticalBox;
class UEditableTextBox;
class UTextBlock;
class UScrollBox;
class UButton;

/**
 * Example UMG widget that demonstrates every horizOn SDK feature.
 *
 * When used directly (not subclassed in a Blueprint with a custom layout),
 * the widget constructs its own UI in NativeConstruct() with input fields
 * and buttons for every SDK feature plus a scrollable output log.
 *
 * If a Blueprint subclass provides its own widget tree layout, the
 * auto-constructed UI is skipped. Blueprint subclasses can call the
 * Test*() methods directly from their own buttons.
 */
UCLASS(Blueprintable)
class HORIZONSDK_API UHorizonExampleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Test methods (callable from Blueprint or the built-in UI) ---

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestConnect();

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSignUpAnonymous(const FString& DisplayName);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSignInEmail(const FString& Email, const FString& Password);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSaveData(const FString& Data);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestLoadData();

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSubmitScore(int64 Score);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestGetTopScores(int32 Limit);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestGetAllConfigs();

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestLoadNews(int32 Limit);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestRedeemCode(const FString& Code);

	UFUNCTION(BlueprintCallable, Category = "horizOn|Example")
	void TestSubmitFeedback(const FString& Title, const FString& Message);

	UPROPERTY(BlueprintReadOnly, Category = "horizOn|Example")
	FString OutputLog;

	UPROPERTY(BlueprintAssignable, Category = "horizOn|Example")
	FOnHorizonOutputUpdated OnOutputUpdated;

protected:
	virtual void NativeConstruct() override;

	void AppendOutput(const FString& Text);
	UHorizonSubsystem* GetSubsystem() const;

private:
	// --- Self-constructed UI state ---
	bool bDefaultUIBuilt = false;

	void BuildDefaultUI();

	// Input field references (only valid when default UI is built)
	UPROPERTY()
	UEditableTextBox* DisplayNameInput = nullptr;

	UPROPERTY()
	UEditableTextBox* EmailInput = nullptr;

	UPROPERTY()
	UEditableTextBox* PasswordInput = nullptr;

	UPROPERTY()
	UEditableTextBox* SaveDataInput = nullptr;

	UPROPERTY()
	UEditableTextBox* ScoreInput = nullptr;

	UPROPERTY()
	UEditableTextBox* GiftCodeInput = nullptr;

	UPROPERTY()
	UEditableTextBox* FeedbackTitleInput = nullptr;

	UPROPERTY()
	UEditableTextBox* FeedbackMessageInput = nullptr;

	UPROPERTY()
	UTextBlock* OutputLogText = nullptr;

	UPROPERTY()
	UScrollBox* OutputScrollBox = nullptr;

	// --- Button click handlers (UFUNCTION required for AddDynamic) ---

	UFUNCTION()
	void OnConnectClicked();

	UFUNCTION()
	void OnSignUpAnonymousClicked();

	UFUNCTION()
	void OnSignInEmailClicked();

	UFUNCTION()
	void OnSaveDataClicked();

	UFUNCTION()
	void OnLoadDataClicked();

	UFUNCTION()
	void OnSubmitScoreClicked();

	UFUNCTION()
	void OnGetTopScoresClicked();

	UFUNCTION()
	void OnGetAllConfigsClicked();

	UFUNCTION()
	void OnLoadNewsClicked();

	UFUNCTION()
	void OnRedeemCodeClicked();

	UFUNCTION()
	void OnSubmitFeedbackClicked();

	UFUNCTION()
	void OnClearLogClicked();

	// --- Subsystem event handlers ---

	UFUNCTION()
	void OnServerConnected();

	UFUNCTION()
	void OnServerConnectionFailed(const FString& ErrorMessage);

	// --- UI helpers ---

	UButton* CreateButton(const FString& Label);
	UEditableTextBox* CreateInputField(const FString& HintText);
	UTextBlock* CreateLabel(const FString& Text, int32 FontSize = 12);
	void AddSectionHeader(UVerticalBox* Root, const FString& Text);
	void AddRow(UVerticalBox* Root, const TArray<UWidget*>& Widgets);
};
