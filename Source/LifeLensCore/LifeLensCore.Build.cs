using UnrealBuildTool;
using System.IO;

public class LifeLensCore : ModuleRules
{
    public LifeLensCore(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.CPlusPlus;
        PCHUsage = PCHUsageMode.NoPCHs;
        CppStandard = CppStandardVersion.Cpp17;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

        // Intentionally no Unreal module dependencies.
        // Source/LifeLensCore must remain standard C++17 and buildable via CMake/Termux.
        PublicDependencyModuleNames.Clear();
        PrivateDependencyModuleNames.Clear();
    }
}
