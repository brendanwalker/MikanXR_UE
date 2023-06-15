// Copyright (c)  2023 Brendan Walker. All rights reserved.

#include "MikanXR_UEEditorCommands.h"
#include "EditorStyleSet.h"

FMikanXR_UEEditorCommands::FMikanXR_UEEditorCommands() :
	TCommands<FMikanXR_UEEditorCommands>(
		TEXT("MikanXR Commands"), 
		FText::FromString(TEXT("Commands to control Mikan XR")), 
		NAME_None, 
		FEditorStyle::GetStyleSetName()
	)
{}

void FMikanXR_UEEditorCommands::RegisterCommands()
{
#define LOCTEXT_NAMESPACE "MikanXR_UELoc"
	UI_COMMAND(OpenMikanXR_UEWindow, "MikanXR", "Opens MikanXR Window", EUserInterfaceActionType::Check, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::M));
#undef LOCTEXT_NAMESPACE
}
