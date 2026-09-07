#include "IMikanXREditorModule.h"
#include "MikanClientDetailsCustomization.h"
#include "MikanClient.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"

class FMikanXREditorModule : public IMikanXREditorModule
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.RegisterCustomClassLayout(
			AMikanClient::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(
				&FMikanClientDetailsCustomization::MakeInstance));

		PropertyModule.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule =
				FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

			PropertyModule.UnregisterCustomClassLayout(
				AMikanClient::StaticClass()->GetFName());
		}
	}
};

IMPLEMENT_MODULE(FMikanXREditorModule, MikanXREditor)
