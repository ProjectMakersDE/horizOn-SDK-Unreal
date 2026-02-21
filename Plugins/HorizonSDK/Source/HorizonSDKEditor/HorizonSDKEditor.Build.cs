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
            "InputCore",
            "HorizonSDK"
        });
    }
}
