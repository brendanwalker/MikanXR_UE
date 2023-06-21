// Copyright (c) 2023 Brendan Walker. All rights reserved.

#pragma once

#include "MikanXREditorBase.h"
#include "MikanXREditor.generated.h"

/**
 * Editor object which handles all of the logic of the Plugin.
 */

UCLASS()
class MIKANXREDITOR_API UMikanXREditor : public UMikanXREditorBase
{

	GENERATED_BODY()

public:

	// UMikanXREditorBase implementation
	void Init() override;

protected:

	// UMikanXREditorBase implementation
	void InitializeTheWidget();

public:

	/**
	 * Called when the test button has been pressed on the widget.
	 */
	void OnTestButtonPressed();

	// Test variable
	int32 NumberOfTestButtonPressed = 0;
};
