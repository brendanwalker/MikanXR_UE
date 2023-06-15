// Copyright (c) 2022 Damian Nowakowski. All rights reserved.

#include "MikanXR_UEEditorWidget.h"

void UMikanXR_UEEditorWidget::TestButtonPressed()
{
	OnTestButtonPressedDelegate.ExecuteIfBound();
}
