// Copyright (c) 2023 Brendan Walker. All rights reserved.

#include "MikanXR_UEEditor.h"
#include "MikanXR_UEEditorWidget.h"

#include "AssetRegistryModule.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"

void UMikanXR_UEEditor::Init()
{
	// Put initialization code here
}

void UMikanXR_UEEditor::InitializeTheWidget()
{
	// Initialize the widget here
	EditorWidget->SetNumberOfTestButtonPressed(NumberOfTestButtonPressed);

	// Bind all required delegates to the Widget.
	EditorWidget->OnTestButtonPressedDelegate.BindUObject(this, &UMikanXR_UEEditor::OnTestButtonPressed);
}

void UMikanXR_UEEditor::OnTestButtonPressed()
{
	// Button on the widget has been pressed. Increase the counter and inform the widget about it.
	NumberOfTestButtonPressed++;
	EditorWidget->SetNumberOfTestButtonPressed(NumberOfTestButtonPressed);
}


