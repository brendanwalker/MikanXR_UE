#pragma once

#include "IMikanModule.h"
#include "IARKitVideoDeviceManager.h"

class IARKitVideoDeviceModule : public IMikanModule
{
public:
	IARKitVideoDeviceModule()= default;

	virtual IARKitVideoDeviceManagerPtr createARKitVideoDeviceManager()= 0;
};
