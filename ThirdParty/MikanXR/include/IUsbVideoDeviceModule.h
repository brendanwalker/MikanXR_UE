#pragma once

#include "IMikanModule.h"
#include "IUsbVideoDeviceManager.h"

class IUsbVideoDeviceModule : public IMikanModule
{
public:
	IUsbVideoDeviceModule()= default;

	virtual IUsbVideoDeviceManagerPtr createUsbVideoDeviceManager()= 0;
};
