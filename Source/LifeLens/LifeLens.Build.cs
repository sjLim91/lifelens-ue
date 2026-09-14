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

        // LifeLensCore stays engine-independent. Unreal consumes only its C++17
        // public headers here; the implementation is pulled in by the dedicated
        // LLCoreCompileUnit.cpp adapter inside the LifeLens module.
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
