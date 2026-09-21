using UnrealBuildTool;

public class RuntimeDiagnosticsMCP : ModuleRules
{
    public RuntimeDiagnosticsMCP(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "EditorSubsystem", "RuntimeDiagnostics", "ToolsetRegistry" });
    }
}
