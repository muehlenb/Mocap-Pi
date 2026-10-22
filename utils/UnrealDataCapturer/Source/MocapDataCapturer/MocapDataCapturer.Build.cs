// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MocapDataCapturer : ModuleRules
{
	public MocapDataCapturer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "HeadMountedDisplay", "NavigationSystem", "Niagara", "RenderCore", "RHI" });
		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
