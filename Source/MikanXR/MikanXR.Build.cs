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
				// Model stencils build one runtime UStaticMesh shared by every capture pass; the
				// shape actor still uses ProceduralMeshComponent.
				"MeshDescription",
				"StaticMeshDescription",
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
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanClientAPI.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanClientCore.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanCoreApp.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanSerialization.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanSharedTexture.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(LibraryPath, PlatformString, "MikanUtility.lib"));

			// Copy MikanCore to project binaries dir
			string MikanCoreDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanClientCore.dll");
			string MikanProjectCoreDLLPath = CopyToProjectBinaries(MikanCoreDLLPath, Target);
			System.Console.WriteLine("Using MikanCore DLL: " + MikanProjectCoreDLLPath);

			// Copy MikanAPI to project binaries dir
			string MikanApiDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanClientAPI.dll");
			string MikanProjectApiDLLPath = CopyToProjectBinaries(MikanApiDLLPath, Target);
			System.Console.WriteLine("Using MikanAPI DLL: " + MikanProjectApiDLLPath);

			// Copy MikanCoreApp to project binaries dir
			string MikanCoreAppDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanCoreApp.dll");
			string MikanProjectCoreAppDLLPath = CopyToProjectBinaries(MikanCoreAppDLLPath, Target);
			System.Console.WriteLine("Using MikanCoreApp DLL: " + MikanProjectCoreAppDLLPath);

			// Copy MikanSerialization to project binaries dir
			string MikanSerializationDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanSerialization.dll");
			string MikanProjectSerializationDLLPath = CopyToProjectBinaries(MikanSerializationDLLPath, Target);
			System.Console.WriteLine("Using MikanSerialization DLL: " + MikanProjectSerializationDLLPath);

			// Copy MikanSharedTexture to project binaries dir
			string MikanSharedTextureDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanSharedTexture.dll");
			string MikanProjectSharedTextureDLLPath = CopyToProjectBinaries(MikanSharedTextureDLLPath, Target);
			System.Console.WriteLine("Using MikanSharedTexture DLL: " + MikanProjectSharedTextureDLLPath);

			// Copy MikanUtility to project binaries dir
			string MikanUtilityDLLPath = Path.Combine(BinariesPath, PlatformString, "MikanUtility.dll");
			string MikanProjectUtilityDLLPath = CopyToProjectBinaries(MikanUtilityDLLPath, Target);
			System.Console.WriteLine("Using MikanUtility DLL: " + MikanProjectUtilityDLLPath);

			// Copy Refureku to project binaries dir
			string RefurekuDLLPath = Path.Combine(BinariesPath, PlatformString, "Refureku.dll");
			string RefurekuProjectDLLPath = CopyToProjectBinaries(RefurekuDLLPath, Target);
			System.Console.WriteLine("Using Refureku DLL: " + RefurekuProjectDLLPath);

			// Copy Spout to project binaries dir
			string SpoutDLLPath = Path.Combine(BinariesPath, PlatformString, "SpoutLibrary.dll");
			string SpoutProjectDLLPath = CopyToProjectBinaries(SpoutDLLPath, Target);
			System.Console.WriteLine("Using Spout DLL: " + SpoutProjectDLLPath);

            // Copy GLEW to project binaries dir
            string GLEWDLLPath = Path.Combine(BinariesPath, PlatformString, "glew32.dll");
            string GLEWProjectDLLPath = CopyToProjectBinaries(GLEWDLLPath, Target);
            System.Console.WriteLine("Using GLEW DLL: " + GLEWProjectDLLPath);

            // Ensure that the DLL is staged along with the executable
            RuntimeDependencies.Add("$(TargetOutputDir)/MikanClientCore.dll", MikanProjectCoreDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/MikanClientAPI.dll", MikanProjectApiDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/MikanCoreApp.dll", MikanProjectCoreAppDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/MikanSerialization.dll", MikanProjectSerializationDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/MikanSharedTexture.dll", MikanProjectSharedTextureDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/MikanUtility.dll", MikanProjectUtilityDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/Refureku.dll", RefurekuProjectDLLPath);
			RuntimeDependencies.Add("$(TargetOutputDir)/SpoutLibrary.dll", SpoutProjectDLLPath);
            RuntimeDependencies.Add("$(TargetOutputDir)/glew32.dll", GLEWProjectDLLPath);

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
