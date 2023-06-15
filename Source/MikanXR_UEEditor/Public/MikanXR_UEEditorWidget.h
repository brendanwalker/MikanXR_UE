// Copyright (c) 2022 Damian Nowakowski. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "MikanXR_UEEditorWidget.generated.h"

/**
 * Widget code used to communicate between code and BP widget.
 */

/**
 * Declaration of all required delegates for a proper communication with UI.
 * They will be broadcasted to inform the plugin code about changes in UI.
 */
DECLARE_DELEGATE(FOnTestButton);

UCLASS()
class MIKANXR_UEEDITOR_API UMikanXR_UEEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

	/**
	 * Set "Global Namespace" option to the Widget.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void SetNumberOfTestButtonPressed(int32 NewNumber);

	/**
	 * "Global Namespace" option has been changed on the Widget.
	 */
	UFUNCTION(BlueprintCallable)
	void TestButtonPressed();


	/**
	 * Callbacks to be binded to the MikanXR_UEEditor methods.
	 */
	FOnTestButton OnTestButtonPressedDelegate;
};