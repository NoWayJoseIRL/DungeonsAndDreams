// Copyright (c) Extra Life Studios, LLC. All rights reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class AbleEditor : ModuleRules
	{
		public AbleEditor(ReadOnlyTargetRules Target) : base (Target)
        {
            string EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

            PublicIncludePaths.AddRange(
				new string[] {
                    "Editor/ClassViewer/Public",
                    Path.Combine(ModuleDirectory, "Public"),
                    Path.Combine(ModuleDirectory, "Classes"),
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					Path.Combine(ModuleDirectory, "Private"),
                    Path.Combine(EngineDir, @"Source/Developer/AssetTools/Private"),
					// ... add other private include paths required here ...
				}
				);
			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					//"AssetTools",
					"AnimGraph",
					"BlueprintGraph",
					"GraphEditor",
					"Kismet",
					// ... add other private include paths required here ...
				}
				); 

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"AbleCore",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "AdvancedPreviewScene",
                    "AnimGraph",
                    "ApplicationCore",
                    "AssetTools",
					"BlueprintGraph",
					"ClassViewer",
					"ContentBrowser",
					"Core",
					"CoreUObject",
					"EditorStyle",
					"EditorWidgets",
					"Engine",
					"GraphEditor",
					"InputCore",
					"Kismet",
					"KismetCompiler",
					"KismetWidgets",
					"MainFrame",
					"Projects",
					"PropertyEditor",
					"RHI",
					"Slate",
					"SlateCore",
					"SourceControl",
					"UnrealEd",
					"WorkspaceMenuStructure",
					"GameplayTagsEditor",
                }
				);

            DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}