// (C) Gaslight Games Ltd, 2019-2020.  All rights reserved.

using UnrealBuildTool;
using System.IO;

public class UEOS : ModuleRules
{
    public UEOS( ReadOnlyTargetRules Target ) : base( Target )
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDefinitions.Add( "EOS_LIB=1" );

        if( Target.Configuration != UnrealTargetConfiguration.Shipping )
        {
            OptimizeCode = CodeOptimization.Never;
        }

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine( ModuleDirectory, "Public" )
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine( ModuleDirectory, "Private" )
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "EOSSDK",
                "OnlineSubsystem",
                "OnlineSubsystemSteam"
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Projects",
                "OnlineSubsystem",
                "OnlineSubsystemSteam"
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
