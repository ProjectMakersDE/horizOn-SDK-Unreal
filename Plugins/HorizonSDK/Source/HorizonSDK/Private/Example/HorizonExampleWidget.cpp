// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Example/HorizonExampleWidget.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonCloudSaveManager.h"
#include "Managers/HorizonLeaderboardManager.h"
#include "Managers/HorizonRemoteConfigManager.h"
#include "Managers/HorizonNewsManager.h"
#include "Managers/HorizonGiftCodeManager.h"
#include "Managers/HorizonFeedbackManager.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"

// ============================================================
// Helpers
// ============================================================

void UHorizonExampleWidget::AppendOutput(const FString& Text)
{
	OutputLog += Text + TEXT("\n");
	OnOutputUpdated.Broadcast(OutputLog);

	// Update the built-in output text if present
	if (OutputLogText)
	{
		OutputLogText->SetText(FText::FromString(OutputLog));
	}
	if (OutputScrollBox)
	{
		OutputScrollBox->ScrollToEnd();
	}
}

UHorizonSubsystem* UHorizonExampleWidget::GetSubsystem() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}
	return GI->GetSubsystem<UHorizonSubsystem>();
}

// ============================================================
// UI Construction
// ============================================================

void UHorizonExampleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Only build the default UI if no Blueprint layout was provided
	if (WidgetTree && !bDefaultUIBuilt && GetRootWidget() == nullptr)
	{
		BuildDefaultUI();
		bDefaultUIBuilt = true;
	}
}

UButton* UHorizonExampleWidget::CreateButton(const FString& Label)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle BtnStyle;
	FSlateBrush NormalBrush;
	NormalBrush.TintColor = FSlateColor(FLinearColor(0.15f, 0.15f, 0.25f, 1.0f));
	BtnStyle.SetNormal(NormalBrush);
	FSlateBrush HoveredBrush;
	HoveredBrush.TintColor = FSlateColor(FLinearColor(0.25f, 0.25f, 0.4f, 1.0f));
	BtnStyle.SetHovered(HoveredBrush);
	FSlateBrush PressedBrush;
	PressedBrush.TintColor = FSlateColor(FLinearColor(0.1f, 0.1f, 0.18f, 1.0f));
	BtnStyle.SetPressed(PressedBrush);
	Btn->SetStyle(BtnStyle);

	UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>();
	BtnText->SetText(FText::FromString(Label));

	FSlateFontInfo Font = BtnText->GetFont();
	Font.Size = 11;
	BtnText->SetFont(Font);
	BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	Btn->AddChild(BtnText);
	return Btn;
}

UEditableTextBox* UHorizonExampleWidget::CreateInputField(const FString& HintText)
{
	UEditableTextBox* Input = WidgetTree->ConstructWidget<UEditableTextBox>();
	Input->SetHintText(FText::FromString(HintText));
	Input->SetMinimumDesiredWidth(140.0f);

	FEditableTextBoxStyle InputStyle = Input->WidgetStyle;
	FSlateBrush BgBrush;
	BgBrush.TintColor = FSlateColor(FLinearColor(0.08f, 0.08f, 0.12f, 1.0f));
	InputStyle.SetBackgroundImageNormal(BgBrush);
	InputStyle.SetForegroundColor(FSlateColor(FLinearColor::White));
	Input->SetStyle(InputStyle);

	return Input;
}

UTextBlock* UHorizonExampleWidget::CreateLabel(const FString& Text, int32 FontSize)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));

	FSlateFontInfo Font = Label->GetFont();
	Font.Size = FontSize;
	Label->SetFont(Font);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.85f, 1.0f)));

	return Label;
}

void UHorizonExampleWidget::AddSectionHeader(UVerticalBox* Root, const FString& Text)
{
	UTextBlock* Header = CreateLabel(Text, 13);
	Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.27f, 0.38f, 1.0f)));

	FSlateFontInfo Font = Header->GetFont();
	Font.TypefaceFontName = FName("Bold");
	Header->SetFont(Font);

	UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Header);
	Slot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 4.0f));
}

void UHorizonExampleWidget::AddRow(UVerticalBox* Root, const TArray<UWidget*>& Widgets)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	for (UWidget* Widget : Widgets)
	{
		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(Widget);
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

		// Editable text boxes fill available space
		if (Cast<UEditableTextBox>(Widget))
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		else
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Row);
	Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 2.0f));
}

void UHorizonExampleWidget::BuildDefaultUI()
{
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
	WidgetTree->RootWidget = Root;

	// --- Title ---
	UTextBlock* Title = CreateLabel(TEXT("horizOn SDK Demo"), 18);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.TypefaceFontName = FName("Bold");
	Title->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);

	// --- Connection ---
	AddSectionHeader(Root, TEXT("Connection"));
	UButton* ConnectBtn = CreateButton(TEXT("Connect to Server"));
	ConnectBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnConnectClicked);
	AddRow(Root, { ConnectBtn });

	// --- Authentication ---
	AddSectionHeader(Root, TEXT("Authentication"));

	DisplayNameInput = CreateInputField(TEXT("Display Name"));
	UButton* SignUpBtn = CreateButton(TEXT("Sign Up Anonymous"));
	SignUpBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnSignUpAnonymousClicked);
	AddRow(Root, { DisplayNameInput, SignUpBtn });

	EmailInput = CreateInputField(TEXT("Email"));
	PasswordInput = CreateInputField(TEXT("Password"));
	UButton* SignInBtn = CreateButton(TEXT("Sign In"));
	SignInBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnSignInEmailClicked);
	AddRow(Root, { EmailInput, PasswordInput, SignInBtn });

	// --- Cloud Save ---
	AddSectionHeader(Root, TEXT("Cloud Save"));

	SaveDataInput = CreateInputField(TEXT("Data to save..."));
	UButton* SaveBtn = CreateButton(TEXT("Save"));
	SaveBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnSaveDataClicked);
	UButton* LoadBtn = CreateButton(TEXT("Load"));
	LoadBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnLoadDataClicked);
	AddRow(Root, { SaveDataInput, SaveBtn, LoadBtn });

	// --- Leaderboard ---
	AddSectionHeader(Root, TEXT("Leaderboard"));

	ScoreInput = CreateInputField(TEXT("Score"));
	UButton* SubmitScoreBtn = CreateButton(TEXT("Submit Score"));
	SubmitScoreBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnSubmitScoreClicked);
	UButton* TopScoresBtn = CreateButton(TEXT("Get Top 10"));
	TopScoresBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnGetTopScoresClicked);
	AddRow(Root, { ScoreInput, SubmitScoreBtn, TopScoresBtn });

	// --- Remote Config & News ---
	AddSectionHeader(Root, TEXT("Remote Config & News"));

	UButton* ConfigsBtn = CreateButton(TEXT("Get All Configs"));
	ConfigsBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnGetAllConfigsClicked);
	UButton* NewsBtn = CreateButton(TEXT("Load News"));
	NewsBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnLoadNewsClicked);
	AddRow(Root, { ConfigsBtn, NewsBtn });

	// --- Gift Codes ---
	AddSectionHeader(Root, TEXT("Gift Codes"));

	GiftCodeInput = CreateInputField(TEXT("Gift Code"));
	UButton* RedeemBtn = CreateButton(TEXT("Redeem"));
	RedeemBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnRedeemCodeClicked);
	AddRow(Root, { GiftCodeInput, RedeemBtn });

	// --- Feedback ---
	AddSectionHeader(Root, TEXT("Feedback"));

	FeedbackTitleInput = CreateInputField(TEXT("Title"));
	FeedbackMessageInput = CreateInputField(TEXT("Message"));
	AddRow(Root, { FeedbackTitleInput, FeedbackMessageInput });

	UButton* FeedbackBtn = CreateButton(TEXT("Submit Feedback"));
	FeedbackBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnSubmitFeedbackClicked);
	AddRow(Root, { FeedbackBtn });

	// --- Output Log ---
	AddSectionHeader(Root, TEXT("Output Log"));

	OutputScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
	OutputScrollBox->SetAnimateWheelScrolling(true);

	OutputLogText = WidgetTree->ConstructWidget<UTextBlock>();
	OutputLogText->SetText(FText::FromString(OutputLog.IsEmpty() ? TEXT("Ready. Press 'Connect to Server' to begin.") : OutputLog));
	OutputLogText->SetAutoWrapText(true);

	FSlateFontInfo LogFont = OutputLogText->GetFont();
	LogFont.Size = 10;
	OutputLogText->SetFont(LogFont);
	OutputLogText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.75f, 1.0f)));

	OutputScrollBox->AddChild(OutputLogText);

	USizeBox* LogSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	LogSizeBox->SetMinDesiredHeight(200.0f);
	LogSizeBox->SetMaxDesiredHeight(400.0f);
	LogSizeBox->AddChild(OutputScrollBox);

	UVerticalBoxSlot* LogSlot = Root->AddChildToVerticalBox(LogSizeBox);
	LogSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
	LogSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- Clear button ---
	UButton* ClearBtn = CreateButton(TEXT("Clear Log"));
	ClearBtn->OnClicked.AddDynamic(this, &UHorizonExampleWidget::OnClearLogClicked);
	AddRow(Root, { ClearBtn });
}

// ============================================================
// Button Handlers
// ============================================================

void UHorizonExampleWidget::OnConnectClicked()
{
	TestConnect();
}

void UHorizonExampleWidget::OnSignUpAnonymousClicked()
{
	FString Name = DisplayNameInput ? DisplayNameInput->GetText().ToString() : TEXT("Player");
	if (Name.IsEmpty())
	{
		Name = TEXT("Player");
	}
	TestSignUpAnonymous(Name);
}

void UHorizonExampleWidget::OnSignInEmailClicked()
{
	FString Email = EmailInput ? EmailInput->GetText().ToString() : TEXT("");
	FString Password = PasswordInput ? PasswordInput->GetText().ToString() : TEXT("");
	if (Email.IsEmpty() || Password.IsEmpty())
	{
		AppendOutput(TEXT("[SignIn] Please enter both email and password."));
		return;
	}
	TestSignInEmail(Email, Password);
}

void UHorizonExampleWidget::OnSaveDataClicked()
{
	FString Data = SaveDataInput ? SaveDataInput->GetText().ToString() : TEXT("");
	if (Data.IsEmpty())
	{
		AppendOutput(TEXT("[Save] Please enter data to save."));
		return;
	}
	TestSaveData(Data);
}

void UHorizonExampleWidget::OnLoadDataClicked()
{
	TestLoadData();
}

void UHorizonExampleWidget::OnSubmitScoreClicked()
{
	FString ScoreStr = ScoreInput ? ScoreInput->GetText().ToString() : TEXT("0");
	int64 Score = FCString::Atoi64(*ScoreStr);
	TestSubmitScore(Score);
}

void UHorizonExampleWidget::OnGetTopScoresClicked()
{
	TestGetTopScores(10);
}

void UHorizonExampleWidget::OnGetAllConfigsClicked()
{
	TestGetAllConfigs();
}

void UHorizonExampleWidget::OnLoadNewsClicked()
{
	TestLoadNews(10);
}

void UHorizonExampleWidget::OnRedeemCodeClicked()
{
	FString Code = GiftCodeInput ? GiftCodeInput->GetText().ToString() : TEXT("");
	if (Code.IsEmpty())
	{
		AppendOutput(TEXT("[Gift] Please enter a gift code."));
		return;
	}
	TestRedeemCode(Code);
}

void UHorizonExampleWidget::OnSubmitFeedbackClicked()
{
	FString Title = FeedbackTitleInput ? FeedbackTitleInput->GetText().ToString() : TEXT("");
	FString Message = FeedbackMessageInput ? FeedbackMessageInput->GetText().ToString() : TEXT("");
	if (Title.IsEmpty() || Message.IsEmpty())
	{
		AppendOutput(TEXT("[Feedback] Please enter both a title and a message."));
		return;
	}
	TestSubmitFeedback(Title, Message);
}

void UHorizonExampleWidget::OnClearLogClicked()
{
	OutputLog.Empty();
	if (OutputLogText)
	{
		OutputLogText->SetText(FText::FromString(TEXT("Log cleared.")));
	}
	OnOutputUpdated.Broadcast(OutputLog);
}

// ============================================================
// Test Methods
// ============================================================

void UHorizonExampleWidget::TestConnect()
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub)
	{
		AppendOutput(TEXT("[Connect] ERROR: Subsystem not available."));
		return;
	}

	AppendOutput(TEXT("[Connect] Connecting to server..."));

	Sub->OnConnected.AddUniqueDynamic(this, &UHorizonExampleWidget::OnServerConnected);
	Sub->OnConnectionFailed.AddUniqueDynamic(this, &UHorizonExampleWidget::OnServerConnectionFailed);

	Sub->ConnectToServer();
}

void UHorizonExampleWidget::OnServerConnected()
{
	AppendOutput(TEXT("[Connect] SUCCESS: Connected to server."));
}

void UHorizonExampleWidget::OnServerConnectionFailed(const FString& ErrorMessage)
{
	AppendOutput(FString::Printf(TEXT("[Connect] FAILED: %s"), *ErrorMessage));
}

void UHorizonExampleWidget::TestSignUpAnonymous(const FString& DisplayName)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->Auth)
	{
		AppendOutput(TEXT("[SignUp] ERROR: Subsystem or Auth not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[SignUp] Signing up anonymously as '%s'..."), *DisplayName));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->Auth->SignUpAnonymous(DisplayName, FOnAuthComplete::CreateLambda(
		[WeakSelf](bool bSuccess)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(TEXT("[SignUp] SUCCESS: Anonymous sign-up complete."));
				}
				else
				{
					Self->AppendOutput(TEXT("[SignUp] FAILED: Anonymous sign-up failed."));
				}
			}
		}));
}

void UHorizonExampleWidget::TestSignInEmail(const FString& Email, const FString& Password)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->Auth)
	{
		AppendOutput(TEXT("[SignIn] ERROR: Subsystem or Auth not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[SignIn] Signing in with email '%s'..."), *Email));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->Auth->SignInEmail(Email, Password, FOnAuthComplete::CreateLambda(
		[WeakSelf](bool bSuccess)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(TEXT("[SignIn] SUCCESS: Email sign-in complete."));
				}
				else
				{
					Self->AppendOutput(TEXT("[SignIn] FAILED: Email sign-in failed."));
				}
			}
		}));
}

void UHorizonExampleWidget::TestSaveData(const FString& Data)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->CloudSave)
	{
		AppendOutput(TEXT("[Save] ERROR: Subsystem or CloudSave not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[Save] Saving data (%d chars)..."), Data.Len()));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->CloudSave->Save(Data, FOnRequestComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const FString& ErrorMessage)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(TEXT("[Save] SUCCESS: Data saved to cloud."));
				}
				else
				{
					Self->AppendOutput(FString::Printf(TEXT("[Save] FAILED: %s"), *ErrorMessage));
				}
			}
		}));
}

void UHorizonExampleWidget::TestLoadData()
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->CloudSave)
	{
		AppendOutput(TEXT("[Load] ERROR: Subsystem or CloudSave not available."));
		return;
	}

	AppendOutput(TEXT("[Load] Loading data from cloud..."));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->CloudSave->Load(FOnStringComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const FString& Data)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(FString::Printf(TEXT("[Load] SUCCESS: %s"), *Data));
				}
				else
				{
					Self->AppendOutput(FString::Printf(TEXT("[Load] FAILED: %s"), *Data));
				}
			}
		}));
}

void UHorizonExampleWidget::TestSubmitScore(int64 Score)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->Leaderboard)
	{
		AppendOutput(TEXT("[Score] ERROR: Subsystem or Leaderboard not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[Score] Submitting score %lld..."), Score));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->Leaderboard->SubmitScore(Score, FOnRequestComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const FString& ErrorMessage)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(TEXT("[Score] SUCCESS: Score submitted."));
				}
				else
				{
					Self->AppendOutput(FString::Printf(TEXT("[Score] FAILED: %s"), *ErrorMessage));
				}
			}
		}));
}

void UHorizonExampleWidget::TestGetTopScores(int32 Limit)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->Leaderboard)
	{
		AppendOutput(TEXT("[Top] ERROR: Subsystem or Leaderboard not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[Top] Fetching top %d scores..."), Limit));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->Leaderboard->GetTop(Limit, false, FOnLeaderboardEntriesComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const TArray<FHorizonLeaderboardEntry>& Entries)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(FString::Printf(TEXT("[Top] SUCCESS: %d entries returned."), Entries.Num()));
					for (int32 i = 0; i < Entries.Num(); ++i)
					{
						Self->AppendOutput(FString::Printf(TEXT("  #%d  %s  %lld"),
							Entries[i].Position, *Entries[i].Username, Entries[i].Score));
					}
				}
				else
				{
					Self->AppendOutput(TEXT("[Top] FAILED: Could not fetch leaderboard."));
				}
			}
		}));
}

void UHorizonExampleWidget::TestGetAllConfigs()
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->RemoteConfig)
	{
		AppendOutput(TEXT("[Config] ERROR: Subsystem or RemoteConfig not available."));
		return;
	}

	AppendOutput(TEXT("[Config] Fetching all remote configs..."));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->RemoteConfig->GetAllConfigs(false, FOnAllConfigsComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const TMap<FString, FString>& Configs)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(FString::Printf(TEXT("[Config] SUCCESS: %d config(s) returned."), Configs.Num()));
					for (const auto& Pair : Configs)
					{
						Self->AppendOutput(FString::Printf(TEXT("  %s = %s"), *Pair.Key, *Pair.Value));
					}
				}
				else
				{
					Self->AppendOutput(TEXT("[Config] FAILED: Could not fetch configs."));
				}
			}
		}));
}

void UHorizonExampleWidget::TestLoadNews(int32 Limit)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->News)
	{
		AppendOutput(TEXT("[News] ERROR: Subsystem or News not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[News] Loading up to %d news entries..."), Limit));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->News->LoadNews(Limit, TEXT("en"), false, FOnNewsComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const TArray<FHorizonNewsEntry>& Entries)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(FString::Printf(TEXT("[News] SUCCESS: %d entries returned."), Entries.Num()));
					for (const auto& Entry : Entries)
					{
						Self->AppendOutput(FString::Printf(TEXT("  - %s"), *Entry.Title));
					}
				}
				else
				{
					Self->AppendOutput(TEXT("[News] FAILED: Could not load news."));
				}
			}
		}));
}

void UHorizonExampleWidget::TestRedeemCode(const FString& Code)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->GiftCodes)
	{
		AppendOutput(TEXT("[Gift] ERROR: Subsystem or GiftCodes not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[Gift] Redeeming code '%s'..."), *Code));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->GiftCodes->Redeem(Code, FOnGiftCodeRedeemComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const FString& GiftData, const FString& Message)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(FString::Printf(TEXT("[Gift] SUCCESS: %s  Data: %s"), *Message, *GiftData));
				}
				else
				{
					Self->AppendOutput(FString::Printf(TEXT("[Gift] FAILED: %s"), *Message));
				}
			}
		}));
}

void UHorizonExampleWidget::TestSubmitFeedback(const FString& Title, const FString& Message)
{
	UHorizonSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->Feedback)
	{
		AppendOutput(TEXT("[Feedback] ERROR: Subsystem or Feedback not available."));
		return;
	}

	AppendOutput(FString::Printf(TEXT("[Feedback] Submitting: '%s'..."), *Title));

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	Sub->Feedback->SendGeneral(Title, Message, FOnRequestComplete::CreateLambda(
		[WeakSelf](bool bSuccess, const FString& ErrorMessage)
		{
			if (UHorizonExampleWidget* Self = WeakSelf.Get())
			{
				if (bSuccess)
				{
					Self->AppendOutput(TEXT("[Feedback] SUCCESS: Feedback submitted."));
				}
				else
				{
					Self->AppendOutput(FString::Printf(TEXT("[Feedback] FAILED: %s"), *ErrorMessage));
				}
			}
		}));
}
