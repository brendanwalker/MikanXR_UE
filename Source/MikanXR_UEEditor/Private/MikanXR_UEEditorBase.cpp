// Copyright (c)  2023 Brendan Walker. All rights reserved.

#include "MikanXR_UEEditorBase.h"
#include "MikanXR_UEEditorWidget.h"

#include "AssetRegistryModule.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"

void UMikanXR_UEEditorBase::Init()
{
	// Empty virtual function - to be overridden
}

void UMikanXR_UEEditorBase::InitializeTheWidget()
{
	// Empty virtual function - to be overridden
}

void UMikanXR_UEEditorBase::SetEditorTab(const TSharedRef<SDockTab>& NewEditorTab)
{
	EditorTab = NewEditorTab;
}

UEditorUtilityWidgetBlueprint* UMikanXR_UEEditorBase::GetUtilityWidgetBlueprint()
{
	// Get the Editor Utility Widget Blueprint from the content directory.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath("/MikanXR_UE/MikanXRWidget_BP.MikanXRWidget_BP");
	return Cast<UEditorUtilityWidgetBlueprint>(AssetData.GetAsset());
}

bool UMikanXR_UEEditorBase::CanCreateEditorUI()
{
	// Editor UI can be created only when we have proper Editor Utility Widget Blueprint available.
	return GetUtilityWidgetBlueprint() != nullptr;
}

TSharedRef<SWidget> UMikanXR_UEEditorBase::CreateEditorUI()
{
	// Register OnMapChanged event so we can properly handle Tab and Widget when changing levels.
	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().AddUObject(this, &UMikanXR_UEEditorBase::ChangeTabWorld);

	// Create the Widget
	return CreateEditorWidget();
}

TSharedRef<SWidget> UMikanXR_UEEditorBase::CreateEditorWidget()
{
	TSharedRef<SWidget> CreatedWidget = SNullWidget::NullWidget;
	if (UEditorUtilityWidgetBlueprint* UtilityWidgetBP = GetUtilityWidgetBlueprint())
	{
		// Create Widget from the Editor Utility Widget BP.
		CreatedWidget = UtilityWidgetBP->CreateUtilityWidget();

		// Save the pointer to the created Widget and initialize it.
		EditorWidget = Cast<UMikanXR_UEEditorWidget>(UtilityWidgetBP->GetCreatedWidget());
		if (EditorWidget)
		{
			InitializeTheWidget();
		}
	}

	// Returned Widget will be docked into the Editor Tab.
	return CreatedWidget;
}

void UMikanXR_UEEditorBase::ChangeTabWorld(UWorld* World, EMapChangeType MapChangeType)
{
	// Handle the event when editor map changes.
	if (MapChangeType == EMapChangeType::TearDownWorld)
	{
		// If the world is destroyed - set the Tab content to null and null the Widget.
		if (EditorTab.IsValid())
		{
			EditorTab.Pin()->SetContent(SNullWidget::NullWidget);
		}
		if (EditorWidget)
		{
			EditorWidget->Rename(nullptr, GetTransientPackage());
			EditorWidget = nullptr;
		}
	}
	else if (MapChangeType == EMapChangeType::NewMap || MapChangeType == EMapChangeType::LoadMap)
	{
		// If the map has been created or loaded and the Tab is valid - put a new Widget into this Tab.
		if (EditorTab.IsValid())
		{
			EditorTab.Pin()->SetContent(CreateEditorWidget());
		}
	}
}
