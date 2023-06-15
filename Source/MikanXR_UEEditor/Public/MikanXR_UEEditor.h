// Copyright (c) 2023 Brendan Walker. All rights reserved.

#pragma once

#include "MikanXR_UEEditorBase.h"
#include "MikanXR_UEEditor.generated.h"

/**
 * Editor object which handles all of the logic of the Plugin.
 */

UCLASS()
class MIKANXR_UEEDITOR_API UMikanXR_UEEditor : public UMikanXR_UEEditorBase
{

	GENERATED_BODY()

public:

	// UMikanXR_UEEditorBase implementation
	void Init() override;

protected:

	// UMikanXR_UEEditorBase implementation
	void InitializeTheWidget();

public:

	/**
	 * Called when the test button has been pressed on the widget.
	 */
	void OnTestButtonPressed();

	// Test variable
	int32 NumberOfTestButtonPressed = 0;
};
