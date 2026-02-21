// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Slate dashboard widget for the horizOn SDK editor panel.
 *
 * Displays:
 *  - Config section: API key (masked), host list, timeout/retry settings.
 *  - Connection section: status text and a "Test Connection" button.
 */
class SHorizonEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHorizonEditorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Build the config info section. */
	TSharedRef<SWidget> BuildConfigSection();

	/** Build the connection test section. */
	TSharedRef<SWidget> BuildConnectionSection();

	/** Called when the "Test Connection" button is clicked. */
	FReply OnTestConnectionClicked();

	/** Current connection status text. */
	TSharedPtr<STextBlock> ConnectionStatusText;
};
