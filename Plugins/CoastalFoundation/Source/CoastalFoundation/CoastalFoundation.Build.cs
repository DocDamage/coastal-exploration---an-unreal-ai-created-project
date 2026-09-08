using UnrealBuildTool;
public class CoastalFoundation : ModuleRules
{
    public CoastalFoundation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "InputCore", "EnhancedInput", "ApplicationCore" });
    }
}
