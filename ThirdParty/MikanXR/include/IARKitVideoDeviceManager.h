#pragma once

#include <memory>
#include <stdint.h>

#include "IARKitVideoDevice.h"

class IARKitVideoDeviceManager
{
public:
	IARKitVideoDeviceManager()= default;
	virtual ~IARKitVideoDeviceManager() {}

	virtual bool startup()= 0;
	virtual void update(float deltaTime)= 0;
	virtual void shutdown()= 0;

	virtual size_t getDeviceCount() const= 0;
	virtual IARKitVideoDevicePtr getDeviceByIndex(size_t index)= 0;
	virtual IARKitVideoDevicePtr getDeviceByPath(const char* devicePath)= 0;

	virtual IARKitVideoDevicePtr createVideoDevice(const ARKitVideoConnectionSettings& settings)= 0;
	virtual void destroyVideoDevice(IARKitVideoDevicePtr device)= 0;
};
using IARKitVideoDeviceManagerPtr= std::shared_ptr<IARKitVideoDeviceManager>;
