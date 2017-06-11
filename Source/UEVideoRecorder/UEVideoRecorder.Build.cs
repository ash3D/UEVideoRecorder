// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	using System.IO;

	public class UEVideoRecorder : ModuleRules
	{
        public UEVideoRecorder(TargetInfo Target)
        {
            bEnableExceptions = true;
			//UEBuildConfiguration.bUseLoggingInShipping = true;

			switch (Target.Configuration)
			{
				case UnrealTargetConfiguration.Debug:
				case UnrealTargetConfiguration.DebugGame:
					Definitions.Add("DEBUG");
					break;
			}

			switch (Target.Configuration)
			{
				case UnrealTargetConfiguration.DebugGame:
				case UnrealTargetConfiguration.Development:
				case UnrealTargetConfiguration.Shipping:
					Definitions.Add("GAME");
					break;
			}

			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Engine",
					"VideoRecorder",
					"boost",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
                    "RenderCore",
					"RHI",
					// ... add private dependencies that you statically link with here ...
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