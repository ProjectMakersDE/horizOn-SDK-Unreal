// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "HorizonSDKEditorModule.h"
#include "SHorizonEditorWidget.h"

#include "HorizonConfig.h"
#include "HorizonSessionSave.h"

#include "ToolMenus.h"
#include "DesktopPlatformModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FHorizonSDKEditorModule"

const FName FHorizonSDKEditorModule::DashboardTabId(TEXT("HorizonSDKDashboard"));

// ============================================================
// Module lifecycle
// ============================================================

void FHorizonSDKEditorModule::StartupModule()
{
	// Register menus after ToolMenus is ready
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHorizonSDKEditorModule::RegisterMenus));

	// Register the nomad tab spawner for the dashboard
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DashboardTabId,
		FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) -> TSharedRef<SDockTab>
		{
			return SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)
				[
					SNew(SHorizonEditorWidget)
				];
		}))
		.SetDisplayName(LOCTEXT("DashboardTabTitle", "horizOn SDK Dashboard"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FHorizonSDKEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DashboardTabId);
}

// ============================================================
// Menu registration
// ============================================================

void FHorizonSDKEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = ToolsMenu->AddSection("HorizonSDK", LOCTEXT("HorizonSectionLabel", "horizOn"));

	// 1. Import Config...
	Section.AddMenuEntry(
		"HorizonImportConfig",
		LOCTEXT("ImportConfig", "Import Config..."),
		LOCTEXT("ImportConfigTooltip", "Import an API key and hosts from a JSON file"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FHorizonSDKEditorModule::ImportConfigFromFile))
	);

	// 2. Open SDK Dashboard
	Section.AddMenuEntry(
		"HorizonOpenDashboard",
		LOCTEXT("OpenDashboard", "Open SDK Dashboard"),
		LOCTEXT("OpenDashboardTooltip", "Open the horizOn SDK dashboard panel"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FHorizonSDKEditorModule::OpenDashboard))
	);

	// 3. Clear Session Cache
	Section.AddMenuEntry(
		"HorizonClearCache",
		LOCTEXT("ClearCache", "Clear Session Cache"),
		LOCTEXT("ClearCacheTooltip", "Delete the local session save file"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FHorizonSDKEditorModule::ClearSessionCache))
	);
}

// ============================================================
// Menu actions
// ============================================================

void FHorizonSDKEditorModule::ImportConfigFromFile()
{
	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> OutFiles;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select horizOn config JSON"),
		FPaths::ProjectDir(),
		TEXT(""),
		TEXT("JSON Files (*.json)|*.json"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (!bOpened || OutFiles.Num() == 0)
	{
		return;
	}

	// Read file
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *OutFiles[0]))
	{
		FNotificationInfo Info(LOCTEXT("ImportFailed", "Failed to read the selected file."));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		FNotificationInfo Info(LOCTEXT("ParseFailed", "Failed to parse JSON config file."));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Extract values
	FString ApiKey;
	JsonObject->TryGetStringField(TEXT("apiKey"), ApiKey);

	TArray<FString> Hosts;
	const TArray<TSharedPtr<FJsonValue>>* DomainsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("backendDomains"), DomainsArray))
	{
		for (const auto& Val : *DomainsArray)
		{
			FString Host;
			if (Val->TryGetString(Host))
			{
				Hosts.Add(Host);
			}
		}
	}
	else
	{
		// Fallback: single backendUrl
		FString SingleUrl;
		if (JsonObject->TryGetStringField(TEXT("backendUrl"), SingleUrl))
		{
			Hosts.Add(SingleUrl);
		}
	}

	if (ApiKey.IsEmpty())
	{
		FNotificationInfo Info(LOCTEXT("NoApiKey", "Config JSON does not contain an 'apiKey' field."));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Write to UHorizonConfig CDO
	UHorizonConfig* Config = GetMutableDefault<UHorizonConfig>();
	Config->ApiKey = ApiKey;
	Config->Hosts = Hosts;
	Config->SaveConfig();

	// Show confirmation with masked API key
	FString MaskedKey = ApiKey.Len() > 10
		? ApiKey.Left(10) + TEXT("...")
		: ApiKey;

	FNotificationInfo Info(FText::Format(
		LOCTEXT("ImportSuccess", "horizOn config imported. API Key: {0}, Hosts: {1}"),
		FText::FromString(MaskedKey),
		FText::AsNumber(Hosts.Num())
	));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

void FHorizonSDKEditorModule::OpenDashboard()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DashboardTabId);
}

void FHorizonSDKEditorModule::ClearSessionCache()
{
	UHorizonSessionSave::DeleteFromDisk();

	FNotificationInfo Info(LOCTEXT("CacheCleared", "horizOn session cache cleared."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHorizonSDKEditorModule, HorizonSDKEditor)
