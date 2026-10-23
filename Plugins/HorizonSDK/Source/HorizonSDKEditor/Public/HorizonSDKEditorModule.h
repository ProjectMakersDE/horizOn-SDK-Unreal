// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once

#include "Modules/ModuleManager.h"

class FHorizonSDKEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Register the "Tools > horizOn" submenu. */
	void RegisterMenus();

	/** Menu action: open a file dialog, parse JSON, write to UHorizonConfig. */
	static void ImportConfigFromFile();

	/** Menu action: spawn or focus the SDK dashboard tab. */
	static void OpenDashboard();

	/** Menu action: delete cached session data from disk. */
	static void ClearSessionCache();

	/** Tab spawner ID for the editor dashboard. */
	static const FName DashboardTabId;
};
