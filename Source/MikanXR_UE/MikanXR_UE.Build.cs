// Copyright (c) 2023 Brendan Walker. All rights reserved.

using UnrealBuildTool;

public class MikanXR_UE : ModuleRules
{
	public MikanXR_UE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add("MikanXR_UE/Private");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine"
			}
		);
	}
}
