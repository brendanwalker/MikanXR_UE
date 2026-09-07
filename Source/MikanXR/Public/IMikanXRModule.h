#pragma once

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(MikanXRLog, Verbose, All);

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

	virtual class IMikanAPI* GetMikanAPI() = 0;
	virtual void GetClientInfo(struct MikanClientInfo* OutClientInfo) = 0;
};
