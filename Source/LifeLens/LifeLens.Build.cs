using UnrealBuildTool;
using System.IO;

public class LifeLens : ModuleRules
{
    public LifeLens(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;

        // LifeLens currently keeps public headers in feature folders directly
        // under the module root (Core/, AI/, World/, UI/, ...). UE 5.6 does not
        // add that root as a legacy include path, so expose it explicitly until
        // the module is migrated to Public/Private directories.
        PublicIncludePaths.Add(ModuleDirectory);
        PrivateIncludePaths.Add(ModuleDirectory);

        // Keep LifeLensCore as a pure C++17 library. Unreal consumes its public
        // headers through this include path; one adapter compile unit includes
        // the core implementation without adding Engine dependencies to the core.
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "LifeLensCore", "include"));

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "AIModule",
            "NavigationSystem"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });
    }
}
