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
		// Enable RTTI for Mikan Events
		bUseRTTI = true;
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
				"RenderCore",
				"ProceduralMeshComponent"
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
			// Add the import libraries
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanAPI.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanCore.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanSerialization.lib"));

			// Copy MikanCore to project binaries dir
			string MikanCoreDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanCore.dll");
			string MikanProjectCoreDLLPath = CopyToProjectBinaries(MikanCoreDLLPath, Target);
			System.Console.WriteLine("Using MikanCore DLL: " + MikanProjectCoreDLLPath);

			// Copy MikanAPI to project binaries dir
			string MikanApiDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanAPI.dll");
			string MikanProjectApiDLLPath = CopyToProjectBinaries(MikanApiDLLPath, Target);
			System.Console.WriteLine("Using MikanAPI DLL: " + MikanProjectApiDLLPath);

			// Copy MikanSerialization to project binaries dir
			string MikanSerializationDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanSerialization.dll");
			string MikanProjectSerializationDLLPath = CopyToProjectBinaries(MikanSerializationDLLPath, Target);
			System.Console.WriteLine("Using MikanSerialization DLL: " + MikanProjectSerializationDLLPath);

			// Copy Spout to project binaries dir
			string SpoutDLLPath = Path.Combine(BinariesPath, PlatformString, "SpoutLibrary.dll");
			string SpoutProjectDLLPath = CopyToProjectBinaries(SpoutDLLPath, Target);
			System.Console.WriteLine("Using Spout DLL: " + SpoutProjectDLLPath);

			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add(MikanProjectCoreDLLPath);
			RuntimeDependencies.Add(MikanProjectApiDLLPath);
			RuntimeDependencies.Add(MikanProjectSerializationDLLPath);
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

		//File exists, delete it in case it's outdated
		if (File.Exists(FullExistingPath))
		{
			File.Delete(FullExistingPath);
		}

		// Copy new dll
		if (!ValidFile)
		{
			File.Copy(Filepath, Path.Combine(FullBinariesDir, Filename), true);
		}
		return FullExistingPath;
	}
}
