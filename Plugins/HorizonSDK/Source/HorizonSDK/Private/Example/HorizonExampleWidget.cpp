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

// ============================================================
// Helpers
// ============================================================

void UHorizonExampleWidget::AppendOutput(const FString& Text)
{
	OutputLog += Text + TEXT("\n");
	OnOutputUpdated.Broadcast(OutputLog);
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
// Test methods
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

	TWeakObjectPtr<UHorizonExampleWidget> WeakSelf(this);

	// Bind delegates before initiating connection
	Sub->OnConnected.AddWeakLambda(this, [WeakSelf]()
	{
		if (UHorizonExampleWidget* Self = WeakSelf.Get())
		{
			Self->AppendOutput(TEXT("[Connect] SUCCESS: Connected to server."));
		}
	});

	Sub->OnConnectionFailed.AddWeakLambda(this, [WeakSelf](const FString& ErrorMessage)
	{
		if (UHorizonExampleWidget* Self = WeakSelf.Get())
		{
			Self->AppendOutput(FString::Printf(TEXT("[Connect] FAILED: %s"), *ErrorMessage));
		}
	});

	Sub->ConnectToServer();
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
