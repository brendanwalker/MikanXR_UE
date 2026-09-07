#pragma once

#include <memory>
#include <stdint.h>

#include "INetworkVideoDevice.h"

class INetworkVideoDeviceManager
{
public:
	INetworkVideoDeviceManager()= default;
	virtual ~INetworkVideoDeviceManager() {}

	virtual bool startup()= 0;
	virtual void update(float deltaTime)= 0;
	virtual void shutdown()= 0;

	virtual size_t getDeviceCount() const= 0;
	virtual INetworkVideoDevicePtr getDeviceByIndex(size_t index)= 0;
	virtual INetworkVideoDevicePtr getDeviceByPath(const char* devicePath)= 0;

	virtual INetworkVideoDevicePtr createVideoDevice(const NetworkVideoConnectionSettings& settings)= 0;
	virtual void destroyVideoDevice(INetworkVideoDevicePtr device)= 0;
};
using INetworkVideoDeviceManagerPtr= std::shared_ptr<INetworkVideoDeviceManager>;