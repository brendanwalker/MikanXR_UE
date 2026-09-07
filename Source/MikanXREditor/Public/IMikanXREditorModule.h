#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IMikanXREditorModule : public IModuleInterface
{
public:
	static inline IMikanXREditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IMikanXREditorModule>("MikanXREditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MikanXREditor");
	}
};
