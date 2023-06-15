// Copyright (c)  2023 Brendan Walker. All rights reserved.

#include "MikanXR_UEEditorModule.h"
#include "MikanXR_UEEditor.h"
#include "MikanXR_UEEditorCommands.h"
#include "MikanXR_UEEditorStyle.h"

#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IMainFrameModule.h"

#include "PropertyEditorModule.h"
#include "LevelEditor.h"

IMPLEMENT_MODULE(FMikanXR_UEEditorModule, FMikanXR_UEEditor)

// Id of the MikanXR_UE Tab used to spawn and observe this tab.
const FName MikanXR_UETabId = FName(TEXT("MikanXR_UE"));

void FMikanXR_UEEditorModule::StartupModule()
{
	// Register Styles.
	FMikanXR_UEEditorStyle::Initialize();
	FMikanXR_UEEditorStyle::ReloadTextures();

	// Register UICommands.
	FMikanXR_UEEditorCommands::Register();

	// Register OnPostEngineInit delegate.
	OnPostEngineInitDelegateHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FMikanXR_UEEditorModule::OnPostEngineInit);

	// Create and initialize Editor object.
	Editor = NewObject<UMikanXR_UEEditorBase>(GetTransientPackage(), UMikanXR_UEEditor::StaticClass());
	Editor->Init();

	// Register Tab Spawner. Do not show it in menu, as it will be invoked manually by a UICommand.
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		MikanXR_UETabId,
		FOnSpawnTab::CreateRaw(this, &FMikanXR_UEEditorModule::SpawnEditor),
		FCanSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args) -> bool { return CanSpawnEditor(); })
	)
	.SetMenuType(ETabSpawnerMenuType::Hidden)
	.SetIcon(FSlateIcon(FMikanXR_UEEditorStyle::GetStyleSetName(), "MikanXR_UEEditorStyle.MenuIcon"));
}

void FMikanXR_UEEditorModule::ShutdownModule()
{
	// Unregister Tab Spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MikanXR_UETabId);

	// Cleanup the Editor object.
	Editor = nullptr;

	// Remove OnPostEngineInit delegate
	FCoreDelegates::OnPostEngineInit.Remove(OnPostEngineInitDelegateHandle);

	// Unregister UICommands.
	FMikanXR_UEEditorCommands::Unregister();

	// Unregister Styles.
	FMikanXR_UEEditorStyle::Shutdown();
}

void FMikanXR_UEEditorModule::OnPostEngineInit()
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
				FMikanXR_UEEditorCommands::Get().OpenMikanXR_UEWindow,
				FExecuteAction::CreateRaw(this, &FMikanXR_UEEditorModule::InvokeEditorSpawn),
				FCanExecuteAction::CreateRaw(this, &FMikanXR_UEEditorModule::CanSpawnEditor),
				FIsActionChecked::CreateRaw(this, &FMikanXR_UEEditorModule::IsEditorSpawned)
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
						FMikanXR_UEEditorCommands::Get().OpenMikanXR_UEWindow,
						NAME_None,
						FText::FromString(TEXT("MikanXR")),
						FText::FromString(TEXT("Opens MikanXR Window")),
						FSlateIcon(FMikanXR_UEEditorStyle::GetStyleSetName(), "MikanXR_UEEditorStyle.MenuIcon")
					);
				})
			);

			// Extend Editors menu with the created Menu Extender.
			LevelEditor->GetMenuExtensibilityManager()->AddExtender(MainMenuExtender);
		}
	}
}

void FMikanXR_UEEditorModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Prevent Editor Object from being garbage collected.
	if (Editor)
	{
		Collector.AddReferencedObject(Editor);
	}
}

bool FMikanXR_UEEditorModule::CanSpawnEditor()
{
	// Editor can be spawned only when the Editor object say that UI can be created.
	if (Editor && Editor->CanCreateEditorUI())
	{
		return true;
	}
	return false;
}

TSharedRef<SDockTab> FMikanXR_UEEditorModule::SpawnEditor(const FSpawnTabArgs& Args)
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

bool FMikanXR_UEEditorModule::IsEditorSpawned()
{
	// Checks if the editor tab is already existing in the editor
	return FGlobalTabmanager::Get()->FindExistingLiveTab(MikanXR_UETabId).IsValid();
}

void FMikanXR_UEEditorModule::InvokeEditorSpawn()
{
	// Tries to invoke opening a plugin tab
	FGlobalTabmanager::Get()->TryInvokeTab(MikanXR_UETabId);
}
