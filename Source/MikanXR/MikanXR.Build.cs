// Copyright (c) 2023 Brendan Walker. All rights reserved.

using UnrealBuildTool;
using System.IO;

public class MikanXR : ModuleRules
{
	private string ModulePath
	{
		get
		{
			return ModuleDirectory;
		}
	}

	private string UProjectPath
	{
		get
		{
			return Path.GetFullPath(Path.Combine(ModulePath, "..", "..", "..", ".."));
		}
	}

	private string ThirdPartyPath
	{
		get
		{
			return Path.GetFullPath(Path.Combine(ModulePath, "..", "..", "ThirdParty"));
		}
	}
	private string BinariesPath
	{
		get
		{
			return Path.GetFullPath(Path.Combine(ThirdPartyPath, "MikanXR", "bin"));
		}
	}
	private string LibraryPath
	{
		get
		{
			return Path.GetFullPath(Path.Combine(ThirdPartyPath, "MikanXR", "lib"));
		}
	}

	public MikanXR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add("MikanXR/Private");
		PrivateIncludePaths.Add(Path.Combine(ThirdPartyPath, "MikanXR", "include"));

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
				"Engine",
				"RHI",
				"RenderCore"
			}
		);

		LoadMikanApi(Target);
	}

	public bool LoadMikanApi(ReadOnlyTargetRules Target)
	{
		bool isLibrarySupported = false;
		string PlatformString = "win64";

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "Mikan_CAPI.lib"));

			// Copy Mikan CAPI to project binaries dir
			string MikanDLLPath = Path.Combine(BinariesPath, PlatformString, "Mikan_CAPI.dll");
			string MikanProjectDLLPath = CopyToProjectBinaries(MikanDLLPath, Target);
			System.Console.WriteLine("Using Mikan_CAPI DLL: " + MikanProjectDLLPath);

			// Copy Spout to project binaries dir
			string SpoutDLLPath = Path.Combine(BinariesPath, PlatformString, "SpoutLibrary.dll");
			string SpoutProjectDLLPath = CopyToProjectBinaries(SpoutDLLPath, Target);
			System.Console.WriteLine("Using Spout DLL: " + SpoutProjectDLLPath);

			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add(MikanProjectDLLPath);
			RuntimeDependencies.Add(SpoutProjectDLLPath);

			isLibrarySupported = true;
		}

		return isLibrarySupported;
	}

	// Implemented this method for copying DLL to packaged project's Binaries folder
	// https://answers.unrealengine.com/questions/842286/specify-dll-location-using-plugin-in-cooked-projec.html
	private string CopyToProjectBinaries(string Filepath, ReadOnlyTargetRules Target)
	{
		string BinariesDir = Path.Combine(UProjectPath, "Binaries", Target.Platform.ToString());
		string Filename = Path.GetFileName(Filepath);

		//convert relative path 
		string FullBinariesDir = Path.GetFullPath(BinariesDir);

		if (!Directory.Exists(FullBinariesDir))
		{
			Directory.CreateDirectory(FullBinariesDir);
		}

		string FullExistingPath = Path.Combine(FullBinariesDir, Filename);
		bool ValidFile = false;

		//File exists, check if they're the same
		if (File.Exists(FullExistingPath))
		{
			ValidFile = true;
		}

		//No valid existing file found, copy new dll
		if (!ValidFile)
		{
			File.Copy(Filepath, Path.Combine(FullBinariesDir, Filename), true);
		}
		return FullExistingPath;
	}
}
