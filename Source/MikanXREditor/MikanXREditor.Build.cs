// Copyright (c) 2023 Brendan Walker. All rights reserved.

using UnrealBuildTool;
using System.IO;

public class MikanXREditor : ModuleRules
{
	private string ModulePath
	{
		get { return ModuleDirectory; }
	}

	private string ThirdPartyPath
	{
		get
		{
			// MikanXREditor is at Source/MikanXREditor — ThirdParty is at Plugins/MikanXR_UE/ThirdParty
			return Path.GetFullPath(Path.Combine(ModulePath, "..", "..", "ThirdParty"));
		}
	}

	public MikanXREditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(new string[]
		{
			"MikanXREditor/Private",
			"MikanXR/Private",                                         // MikanScene, UMikanEngineSubsystem, etc.
			Path.Combine(ThirdPartyPath, "MikanXR", "include"),        // MikanAPI.h, MikanAPITypes.h, etc.
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"MikanXR",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"UnrealEd",
			"PropertyEditor",
		});
	}
}
