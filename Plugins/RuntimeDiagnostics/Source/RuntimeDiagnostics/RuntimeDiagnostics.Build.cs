using UnrealBuildTool;

public class RuntimeDiagnostics : ModuleRules
{
    public RuntimeDiagnostics(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "EditorSubsystem" });
        PrivateDependencyModuleNames.AddRange(new[] { "Json", "InputCore", "EnhancedInput", "AnimGraphRuntime" });
    }
}
