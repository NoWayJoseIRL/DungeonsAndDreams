// Copyright (c) Extra Life Studios, LLC. All rights reserved.

namespace UnrealBuildTool.Rules
{
	public class AbleCore : ModuleRules
	{
		public AbleCore(ReadOnlyTargetRules Target) : base (Target)
        {
            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
                    System.IO.Path.Combine(ModuleDirectory, "Classes"),
                    System.IO.Path.Combine(ModuleDirectory, "Public"),
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
                    System.IO.Path.Combine(ModuleDirectory, "Private"),
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"AIModule",				
					"Core",
					"CoreUObject",
					"Engine",
					"EnhancedInput",
					"GameplayCameras",
					"GameplayTags",
					"GameplayTasks", // AIModule requires this...					
					"InputCore",
					"NavigationSystem",
					"Niagara"
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

            if (Target.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
                PrivateDependencyModuleNames.Add("Slate");
                PrivateDependencyModuleNames.Add("SlateCore");
                PrivateDependencyModuleNames.Add("SequenceRecorder");
                PrivateDependencyModuleNames.Add("GameplayTagsEditor");
            }
        }
	}
}