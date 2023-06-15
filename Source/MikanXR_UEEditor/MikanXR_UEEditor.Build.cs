// Copyright (c) 2023 Brendan Walker. All rights reserved.

using UnrealBuildTool;

public class MikanXR_UEEditor : ModuleRules
{
	public MikanXR_UEEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add("MikanXR_UEEditor/Private");
		PrivateIncludePaths.Add("MikanXR_UE/Private");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"MikanXR_UE",
                "Engine",
				"CoreUObject",
                "Slate",
                "SlateCore",
                "UnrealEd",
				"WorkspaceMenuStructure",
				"DesktopPlatform",
                "Blutility",
                "UMG",
                "UMGEditor",
				"EditorStyle",
				"Projects"
			}
		);
	}
}
