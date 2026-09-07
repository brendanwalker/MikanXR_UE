#pragma once

class IMikanModule
{
public:
	IMikanModule()= default;
	virtual ~IMikanModule() {}

	virtual bool startup()= 0;
	virtual void shutdown()= 0;
};

#define ALLOCATE_MODULE_FUNCTION_NAME "AllocatePluginModule"
#define FREE_MODULE_FUNCTION_NAME "FreePluginModule"