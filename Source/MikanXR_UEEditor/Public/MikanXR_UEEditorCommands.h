// Copyright (c) 2022 Damian Nowakowski. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * Class handling UICommands of the editor.
 * Currently only "Open My Plugin" commands is required.
 * It is done via commands, because we want to have a keyboard shortcut for it.
 */

class MIKANXR_UEEDITOR_API FMikanXR_UEEditorCommands : public TCommands<FMikanXR_UEEditorCommands>
{

public:

	FMikanXR_UEEditorCommands();
	void RegisterCommands() override;
	TSharedPtr<FUICommandInfo> OpenMikanXR_UEWindow;
};
