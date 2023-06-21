#pragma once

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "MikanClientTypes.h"

class MIKANXR_API IMikanXRModule : public IModuleInterface
{
public:
	static inline IMikanXRModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IMikanXRModule>("MikanXR");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MikanXR");
	}

	virtual void ConnectSubsystem(class UMikanWorldSubsystem* Subsystem) = 0;
	virtual void DisconnectSubsystem(class UMikanWorldSubsystem* Subsystem) = 0;
	virtual bool GetIsConnected() = 0;
	virtual void GetClientInfo(MikanClientInfo* OutClientInfo) = 0;
};
