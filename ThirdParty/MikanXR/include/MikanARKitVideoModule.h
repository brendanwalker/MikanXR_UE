#pragma once

#include "IMikanModule.h"
#include "MikanARKitVideoExport.h"
#include "IARKitVideoDeviceModule.h"

MIKAN_ARKIT_VIDEO_FUNC(IARKitVideoDeviceModule*)
AllocatePluginModule();
MIKAN_ARKIT_VIDEO_FUNC(void)
FreePluginModule(IARKitVideoDeviceModule* module);
