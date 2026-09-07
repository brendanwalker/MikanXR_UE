#pragma once

#include "IMikanModule.h"
#include "MikanWMFVideoExport.h"
#include "IUsbVideoDeviceModule.h"

MIKAN_WMF_VIDEO_FUNC(IUsbVideoDeviceModule*)
AllocatePluginModule();
MIKAN_WMF_VIDEO_FUNC(void)
FreePluginModule(IUsbVideoDeviceModule* module);