// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class EOSBasic : ModuleRules
{
    public EOSBasic(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //Expose Pack_Scope of Steam to Public
        PublicDefinitions.Add("ONLINESUBSYSTEMSTEAM_PACKAGE=1");

        //Required for some private headers needed for steam sockets support.
        //var EngineDir = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);
        var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(EngineDir, @"Plugins\Online\OnlineSubsystemSteam\Source\Private")
            });


        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "Party", "Lobby", "CoreUObject", "Engine", "InputCore", "OnlineSubsystem", "OnlineSubsystemUtils",
            "OnlineSubsystemSteam",
            "Sockets",
            "Steamworks",
            "SteamParty",
            "UEOS",
            "EOSSDK"
            //"CoreUObject",
            //"Engine",
            // ... add other public dependencies that you statically link with here ...
        });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                //"Slate",
                //"SlateCore",
                // ... add private dependencies that you statically link with here ...	
            }
        );


        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        PrivateDependencyModuleNames.Add("OnlineSubsystem");
        PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");

        DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[]
            {

            }
        );

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
