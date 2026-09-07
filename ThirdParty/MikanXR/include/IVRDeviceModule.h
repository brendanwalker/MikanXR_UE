#pragma once

#include "IMikanModule.h"
#include "IVRDeviceManager.h"

class IVRDeviceModule : public IMikanModule
{
public:
	IVRDeviceModule()= default;

	virtual IVRDeviceManagerPtr createTrackingRuntime()= 0;
};
