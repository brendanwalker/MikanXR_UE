// Copyright (c) 2023 Brendan Walker. All rights reserved.

#include "MikanXR_UEEditorStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

// Null declaration of static variable (for linker needs)
TSharedPtr<FSlateStyleSet> FMikanXR_UEEditorStyle::StyleInstance = nullptr;

void FMikanXR_UEEditorStyle::Initialize()
{
	if (StyleInstance.IsValid() == false)
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMikanXR_UEEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FMikanXR_UEEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

FName FMikanXR_UEEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("MikanXR_UEEditorStyle"));
	return StyleSetName;
}

TSharedRef<FSlateStyleSet> FMikanXR_UEEditorStyle::Create()
{
	// Create a new Style Set with a content root set to Resources directory of the plugin.
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MikanXR_UE")->GetBaseDir() / TEXT("Resources"));

	// Create a new Slate Image Brush, which is Icon16.png from Resources directory.
	FSlateImageBrush* Brush = new FSlateImageBrush(Style->RootToContentDir(TEXT("Icon16"), TEXT(".png")), { 16.f, 16.f });
	
	// Add newly created Brush to the Style Set.
	Style->Set("MikanXR_UEEditorStyle.MenuIcon", Brush);
	
	// Result is a Style Set with menu icon in it.
	return Style;
}
