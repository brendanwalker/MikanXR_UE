#pragma once

#include "MikanCoreAppExport.h"
#include "IMikanModule.h"

#include <string>

class IMikanModuleManager
{
public:
	virtual ~IMikanModuleManager() {};

	virtual IMikanModule* getModule(const std::string& moduleName)= 0;
	virtual bool disposeModule(IMikanModule* moduleIterface)= 0;

	template <typename t_module_type>
	t_module_type* getModule(const std::string& moduleName)
	{
		return static_cast<t_module_type*>(getModule(moduleName));
	}
};

MIKAN_COREAPP_FUNC(bool) initMikanModuleManager();
MIKAN_COREAPP_FUNC(void) shutdownMikanModuleManager();
MIKAN_COREAPP_FUNC(IMikanModuleManager*) getMikanModuleManager();