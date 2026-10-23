// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

using UnrealBuildTool;

public class HorizonSDKEditor : ModuleRules
{
    public HorizonSDKEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "DesktopPlatform",
            "HTTP",
            "Json",
            "InputCore",
            "HorizonSDK"
        });
    }
}
