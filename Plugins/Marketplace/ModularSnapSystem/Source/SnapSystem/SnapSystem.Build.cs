// Copyright 2018-2021 S.Chachkov & A.Putrino. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class SnapSystem : ModuleRules
{
	public SnapSystem(ReadOnlyTargetRules Target) : base(Target)
	{
        
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                                
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
                "ComponentVisualizers",
                "InputCore",
//				"EditorFramework",
				"UnrealEd",
				"LevelEditor",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "EditorStyle",
                "ApplicationCore","DesktopPlatform"



				// ... add private dependencies that you statically link with here ...	
			}
            );
		if (Target.Version.MajorVersion == 5)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{

				"EditorFramework",

					// ... add private dependencies that you statically link with here ...	
				}
				);
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
