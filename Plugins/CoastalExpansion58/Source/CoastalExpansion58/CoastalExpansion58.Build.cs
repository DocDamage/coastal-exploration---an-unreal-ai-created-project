using UnrealBuildTool;

public class CoastalExpansion58 : ModuleRules
{
    public CoastalExpansion58(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "CoastalFoundation", "AdvancedShooterSystem"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "EnhancedInput", "UMG", "Slate", "SlateCore"
        });
    }
}
