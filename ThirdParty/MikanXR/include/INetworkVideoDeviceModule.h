#pragma once

#include "IMikanModule.h"
#include "INetworkVideoDeviceManager.h"

class INetworkVideoDeviceModule : public IMikanModule
{
public:
	INetworkVideoDeviceModule()= default;

	virtual INetworkVideoDeviceManagerPtr createNetworkVideoDeviceManager()= 0;
};
