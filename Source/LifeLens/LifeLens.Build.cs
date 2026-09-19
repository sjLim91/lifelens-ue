using UnrealBuildTool;
using System.IO;

public class LifeLens : ModuleRules
{
    public LifeLens(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // UE 5.6 headers require C++20. LifeLensCore itself remains engine-independent
        // and source-compatible with C++17; compiling it inside the UE module under
        // the newer language standard does not introduce Unreal dependencies.
        CppStandard = CppStandardVersion.Cpp20;

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
            "NavigationSystem",
            "Niagara"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            // FDisplayMetrics::RebuildDisplayMetrics is implemented by
            // ApplicationCore. The game target linked transitively before, but
            // LifeLensEditor requires the explicit module dependency.
            "ApplicationCore",
            // Runtime river/lake presentation consumes the enabled Unreal Water
            // plugin without giving it simulation authority.
            "Water"
        });

        // PCG is a desktop presentation dependency only. Android keeps the
        // lightweight HISM/mobile dressing path and does not link the PCG module.
        if (Target.Platform == UnrealTargetPlatform.Win64
            || Target.Platform == UnrealTargetPlatform.Mac
            || Target.Platform == UnrealTargetPlatform.Linux)
        {
            PrivateDependencyModuleNames.Add("PCG");
        }
    }
}
