using UnrealBuildTool;
public class CoastalVendorIntegration : ModuleRules
{
    public CoastalVendorIntegration(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "CoastalFoundation" });
        PrivateDependencyModuleNames.AddRange(new[] { "GameplayTags", "Json", "UMG", "EnhancedInput", "InputCore", "ApplicationCore", "AudioMixer", "Slate", "SlateCore" });
    }
}
