// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "SHorizonEditorWidget.h"
#include "HorizonConfig.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#define LOCTEXT_NAMESPACE "SHorizonEditorWidget"

void SHorizonEditorWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				// Title
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 8)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DashboardTitle", "horizOn SDK Dashboard"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 8)
				[
					SNew(SSeparator)
				]

				// Config section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				[
					BuildConfigSection()
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 8)
				[
					SNew(SSeparator)
				]

				// Connection section
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildConnectionSection()
				]
			]
		]
	];
}

TSharedRef<SWidget> SHorizonEditorWidget::BuildConfigSection()
{
	const UHorizonConfig* Config = UHorizonConfig::Get();

	// Mask the API key: show first 10 chars + "..."
	FString MaskedKey;
	if (Config->ApiKey.IsEmpty())
	{
		MaskedKey = TEXT("(not set)");
	}
	else if (Config->ApiKey.Len() > 10)
	{
		MaskedKey = Config->ApiKey.Left(10) + TEXT("...");
	}
	else
	{
		MaskedKey = Config->ApiKey;
	}

	// Build hosts list string
	FString HostsStr;
	if (Config->Hosts.Num() == 0)
	{
		HostsStr = TEXT("(none configured)");
	}
	else
	{
		HostsStr = FString::Join(Config->Hosts, TEXT("\n    "));
	}

	return SNew(SVerticalBox)

		// Section header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ConfigHeader", "Configuration"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		// API Key
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("ApiKeyLabel", "API Key: {0}"), FText::FromString(MaskedKey)))
		]

		// Hosts
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("HostsLabel", "Hosts ({0}):  {1}"),
				FText::AsNumber(Config->Hosts.Num()),
				FText::FromString(HostsStr)))
		]

		// Timeout
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("TimeoutLabel", "Connection Timeout: {0}s"),
				FText::AsNumber(Config->ConnectionTimeoutSeconds)))
		]

		// Retry attempts
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("RetryLabel", "Max Retries: {0}  (delay {1}s)"),
				FText::AsNumber(Config->MaxRetryAttempts),
				FText::AsNumber(Config->RetryDelaySeconds)))
		]

		// Log level
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("LogLevelLabel", "Log Level: {0}"),
				FText::AsNumber(static_cast<int32>(Config->LogLevel))))
		];
}

TSharedRef<SWidget> SHorizonEditorWidget::BuildConnectionSection()
{
	return SNew(SVerticalBox)

		// Section header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ConnectionHeader", "Connection Test"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		// Status text
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 2)
		[
			SAssignNew(ConnectionStatusText, STextBlock)
			.Text(LOCTEXT("StatusIdle", "Status: idle"))
		]

		// Test button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8, 8, 8, 0)
		[
			SNew(SBox)
			.WidthOverride(200.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("TestBtn", "Test Connection"))
				.OnClicked(this, &SHorizonEditorWidget::OnTestConnectionClicked)
			]
		];
}

FReply SHorizonEditorWidget::OnTestConnectionClicked()
{
	const UHorizonConfig* Config = UHorizonConfig::Get();

	if (Config->Hosts.Num() == 0)
	{
		ConnectionStatusText->SetText(LOCTEXT("NoHosts", "Status: no hosts configured"));
		return FReply::Handled();
	}

	const FString& Host = Config->Hosts[0];

	ConnectionStatusText->SetText(FText::Format(
		LOCTEXT("Pinging", "Status: pinging {0}..."),
		FText::FromString(Host)));

	// Simple GET to the host root to test connectivity
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(Host);
	Request->SetTimeout(static_cast<float>(Config->ConnectionTimeoutSeconds));

	TWeakPtr<STextBlock> WeakStatus = ConnectionStatusText;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakStatus](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			TSharedPtr<STextBlock> StatusText = WeakStatus.Pin();
			if (!StatusText.IsValid())
			{
				return;
			}

			if (bConnectedSuccessfully && Response.IsValid())
			{
				StatusText->SetText(FText::Format(
					LOCTEXT("PingSuccess", "Status: connected (HTTP {0})"),
					FText::AsNumber(Response->GetResponseCode())));
			}
			else
			{
				StatusText->SetText(LOCTEXT("PingFailed", "Status: connection failed"));
			}
		});

	Request->ProcessRequest();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
