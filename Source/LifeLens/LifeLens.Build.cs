using UnrealBuildTool;

public class LifeLens : ModuleRules
{
    public LifeLens(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // LifeLens currently keeps public headers in feature folders directly
        // under the module root (Core/, AI/, World/, UI/, ...). UE 5.6 does not
        // add that root as a legacy include path, so expose it explicitly until
        // the module is migrated to Public/Private directories.
        PublicIncludePaths.Add(ModuleDirectory);
        PrivateIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "AIModule",
            "NavigationSystem",
            "LifeLensCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });
    }
}
