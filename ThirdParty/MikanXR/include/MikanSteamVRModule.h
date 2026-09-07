#pragma once

#include "IMikanModule.h"
#include "MikanSteamVRExport.h"
#include "IVRDeviceModule.h"

MIKAN_STEAMVR_FUNC(IVRDeviceModule*) AllocatePluginModule();
MIKAN_STEAMVR_FUNC(void) FreePluginModule(IVRDeviceModule* module);