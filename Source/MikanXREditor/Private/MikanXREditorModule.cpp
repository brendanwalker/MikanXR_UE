// Copyright (c)  2023 Brendan Walker. All rights reserved.

#include "MikanXREditorModule.h"
#include "MikanXREditor.h"
#include "MikanXREditorCommands.h"
#include "MikanXREditorStyle.h"

#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IMainFrameModule.h"

#include "PropertyEditorModule.h"
#include "LevelEditor.h"

IMPLEMENT_MODULE(FMikanXREditorModule, FMikanXREditor)

// Id of the MikanXR Tab used to spawn and observe this tab.
const FName MikanXRTabId = FName(TEXT("MikanXR"));

void FMikanXREditorModule::StartupModule()
{
	// Register Styles.
	FMikanXREditorStyle::Initialize();
	FMikanXREditorStyle::ReloadTextures();

	// Register UICommands.
	FMikanXREditorCommands::Register();

	// Register OnPostEngineInit delegate.
	OnPostEngineInitDelegateHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FMikanXREditorModule::OnPostEngineInit);

	// Create and initialize Editor object.
	Editor = NewObject<UMikanXREditorBase>(GetTransientPackage(), UMikanXREditor::StaticClass());
	Editor->Init();

	// Register Tab Spawner. Do not show it in menu, as it will be invoked manually by a UICommand.
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		MikanXRTabId,
		FOnSpawnTab::CreateRaw(this, &FMikanXREditorModule::SpawnEditor),
		FCanSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args) -> bool { return CanSpawnEditor(); })
	)
	.SetMenuType(ETabSpawnerMenuType::Hidden)
	.SetIcon(FSlateIcon(FMikanXREditorStyle::GetStyleSetName(), "MikanXREditorStyle.MenuIcon"));
}

void FMikanXREditorModule::ShutdownModule()
{
	// Unregister Tab Spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MikanXRTabId);

	// Cleanup the Editor object.
	Editor = nullptr;

	// Remove OnPostEngineInit delegate
	FCoreDelegates::OnPostEngineInit.Remove(OnPostEngineInitDelegateHandle);

	// Unregister UICommands.
	FMikanXREditorCommands::Unregister();

	// Unregister Styles.
	FMikanXREditorStyle::Shutdown();
}

void FMikanXREditorModule::OnPostEngineInit()
{
	// This function is for registering UICommand to the engine, so it can be executed via keyboard shortcut.
	// This will also add this UICommand to the menu, so it can also be executed from there.
	
	// This function is valid only if no Commandlet or game is running. It also requires Slate Application to be initialized.
	if ((IsRunningCommandlet() == false) && (IsRunningGame() == false) && FSlateApplication::IsInitialized())
	{
		if (FLevelEditorModule* LevelEditor = FModuleManager::LoadModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			// Create a UICommandList and map editor spawning function to the UICommand of opening My Plugin Editor.
			TSharedPtr<FUICommandList> Commands = MakeShareable(new FUICommandList());
			Commands->MapAction(
				FMikanXREditorCommands::Get().OpenMikanXRWindow,
				FExecuteAction::CreateRaw(this, &FMikanXREditorModule::InvokeEditorSpawn),
				FCanExecuteAction::CreateRaw(this, &FMikanXREditorModule::CanSpawnEditor),
				FIsActionChecked::CreateRaw(this, &FMikanXREditorModule::IsEditorSpawned)
			);

			// Register this UICommandList to the MainFrame.
			// Otherwise nothing will handle the input to trigger this command.
			IMainFrameModule& MainFrame = FModuleManager::Get().LoadModuleChecked<IMainFrameModule>("MainFrame");
			MainFrame.GetMainFrameCommandBindings()->Append(Commands.ToSharedRef());

			// Create a Menu Extender, which adds a button that executes the UICommandList of opening My Plugin Window.
			TSharedPtr<FExtender> MainMenuExtender = MakeShareable(new FExtender);
			MainMenuExtender->AddMenuExtension(
				FName(TEXT("General")),
				EExtensionHook::After, 
				Commands,
				FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& MenuBuilder)
				{
					MenuBuilder.AddMenuEntry(
						FMikanXREditorCommands::Get().OpenMikanXRWindow,
						NAME_None,
						FText::FromString(TEXT("MikanXR")),
						FText::FromString(TEXT("Opens MikanXR Window")),
						FSlateIcon(FMikanXREditorStyle::GetStyleSetName(), "MikanXREditorStyle.MenuIcon")
					);
				})
			);

			// Extend Editors menu with the created Menu Extender.
			LevelEditor->GetMenuExtensibilityManager()->AddExtender(MainMenuExtender);
		}
	}
}

void FMikanXREditorModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Prevent Editor Object from being garbage collected.
	if (Editor)
	{
		Collector.AddReferencedObject(Editor);
	}
}

bool FMikanXREditorModule::CanSpawnEditor()
{
	// Editor can be spawned only when the Editor object say that UI can be created.
	if (Editor && Editor->CanCreateEditorUI())
	{
		return true;
	}
	return false;
}

TSharedRef<SDockTab> FMikanXREditorModule::SpawnEditor(const FSpawnTabArgs& Args)
{	
	// Spawn the Editor only when we can.
	if (CanSpawnEditor())
	{
		// Spawn new DockTab and fill it with newly created editor UI.
		TSharedRef<SDockTab> NewTab = SAssignNew(EditorTab, SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				Editor->CreateEditorUI()
			];

		// Tell the Editor Object about newly spawned DockTab, as it will 
		// need it to handle various editor actions.
		Editor->SetEditorTab(NewTab);

		// Return the DockTab to the Global Tab Manager.
		return NewTab;
	}

	// If editor can't be spawned - create an empty tab.
	return SAssignNew(EditorTab, SDockTab).TabRole(ETabRole::NomadTab);
}

bool FMikanXREditorModule::IsEditorSpawned()
{
	// Checks if the editor tab is already existing in the editor
	return FGlobalTabmanager::Get()->FindExistingLiveTab(MikanXRTabId).IsValid();
}

void FMikanXREditorModule::InvokeEditorSpawn()
{
	// Tries to invoke opening a plugin tab
	FGlobalTabmanager::Get()->TryInvokeTab(MikanXRTabId);
}
