#pragma once

#include "IMikanModule.h"
#include "MikanGStreamerVideoExport.h"
#include "INetworkVideoDeviceModule.h"

MIKAN_GSTREAMER_VIDEO_FUNC(INetworkVideoDeviceModule*)
AllocatePluginModule();
MIKAN_GSTREAMER_VIDEO_FUNC(void)
FreePluginModule(INetworkVideoDeviceModule* module);