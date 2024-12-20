// Copyright (c)  2023 Brendan Walker. All rights reserved.

#include "MikanXREditorCommands.h"
#include "Styling/AppStyle.h"

FMikanXREditorCommands::FMikanXREditorCommands() :
	TCommands<FMikanXREditorCommands>(
		TEXT("MikanXR Commands"), 
		FText::FromString(TEXT("Commands to control Mikan XR")), 
		NAME_None, 
		FAppStyle::GetAppStyleSetName()
	)
{}

void FMikanXREditorCommands::RegisterCommands()
{
#define LOCTEXT_NAMESPACE "MikanXRLoc"
	UI_COMMAND(OpenMikanXRWindow, "MikanXR", "Opens MikanXR Window", EUserInterfaceActionType::Check, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::M));
#undef LOCTEXT_NAMESPACE
}
