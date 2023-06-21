// Copyright (c) 2022 Damian Nowakowski. All rights reserved.

#include "MikanXREditorWidget.h"

void UMikanXREditorWidget::TestButtonPressed()
{
	OnTestButtonPressedDelegate.ExecuteIfBound();
}
