// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShardsOfVeyara : ModuleRules
{
	public ShardsOfVeyara(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"Enemy",
			"Pet",
			"LogData",
			"Niagara",
			"NavigationSystem",
			"PhysicsCore",
			"MotionWarping",
			"GeometryCollectionEngine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] 
		{
			"Niagara",
			"LevelSequence",
			"MovieScene",
			"HTTP",
			"Json",
			"JsonUtilities"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        PublicIncludePaths.AddRange(new string[] {
			"ShardsOfVeyara",
			"ShardsOfVeyara/Tutorial",
			"ShardsOfVeyara/Character",
			"ShardsOfVeyara/Character/Components",
			"ShardsOfVeyara/Inventory",
			"ShardsOfVeyara/Inventory/UI",
			"ShardsOfVeyara/FarmingSystem",
			"ShardsOfVeyara/Variant_Platforming",
			"ShardsOfVeyara/Variant_Platforming/Animation",
			"ShardsOfVeyara/Variant_Combat",
			"ShardsOfVeyara/Variant_Combat/AI",
			"ShardsOfVeyara/Variant_Combat/Animation",
			"ShardsOfVeyara/Variant_Combat/Gameplay",
			"ShardsOfVeyara/Variant_Combat/Interfaces",
			"ShardsOfVeyara/Variant_Combat/UI",
			"ShardsOfVeyara/Variant_SideScrolling",
			"ShardsOfVeyara/Variant_SideScrolling/AI",
			"ShardsOfVeyara/Variant_SideScrolling/Gameplay",
			"ShardsOfVeyara/Variant_SideScrolling/Interfaces",
			"ShardsOfVeyara/Variant_SideScrolling/UI"
		});
	}
}
