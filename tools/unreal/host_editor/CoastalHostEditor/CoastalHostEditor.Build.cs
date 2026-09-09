using UnrealBuildTool;
public class CoastalHostEditor : ModuleRules {
 public CoastalHostEditor(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine" });
  PrivateDependencyModuleNames.AddRange(new [] { "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "Json", "CoastalFoundation", "CoastalVendorIntegration", "AssetRegistry", "UMG", "InputCore", "ApplicationCore", "CoastalExpansion58", "AdvancedShooterSystem" });
 }
}
