// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AGSD : ModuleRules
{
	public AGSD(ReadOnlyTargetRules Target) : base(Target)
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
			"MotionWarping"
		});

		PrivateDependencyModuleNames.AddRange(new string[] 
		{
			"Niagara"
        });

        // 에디터 빌드일 때만 추가되도록 조건문을 사용합니다.
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        PublicIncludePaths.AddRange(new string[] {
			"AGSD",
			"AGSD/Character",
			"AGSD/Character/Components",
			"AGSD/Inventory",
			"AGSD/Inventory/UI",
			"AGSD/FarmingSystem",
			"AGSD/Variant_Platforming",
			"AGSD/Variant_Platforming/Animation",
			"AGSD/Variant_Combat",
			"AGSD/Variant_Combat/AI",
			"AGSD/Variant_Combat/Animation",
			"AGSD/Variant_Combat/Gameplay",
			"AGSD/Variant_Combat/Interfaces",
			"AGSD/Variant_Combat/UI",
			"AGSD/Variant_SideScrolling",
			"AGSD/Variant_SideScrolling/AI",
			"AGSD/Variant_SideScrolling/Gameplay",
			"AGSD/Variant_SideScrolling/Interfaces",
			"AGSD/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
