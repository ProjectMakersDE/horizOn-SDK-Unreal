using System.IO;
using UnrealBuildTool;

public class HorizonSDK : ModuleRules
{
    public HorizonSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "HTTP",
            "Json",
            "JsonUtilities",
            "UMG",
            "Slate",
            "SlateCore",
            "RHI"
        });

        // iOS-only -- link AuthenticationServices for Sign in with Apple, and inject the
        // entitlement via UPL so the customer does not have to hand-edit project files.
        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicFrameworks.AddRange(new string[] { "AuthenticationServices" });

            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("IOSPlugin", Path.Combine(PluginPath, "Private", "iOS", "HorizonSDK_UPL.xml"));
        }
    }
}
