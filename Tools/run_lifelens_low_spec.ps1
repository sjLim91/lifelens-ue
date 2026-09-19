param(
    [string]$Project = "D:\\LifeLens\\LifeLens.uproject",
    [string]$UnrealEditor = "D:\\Epic Games\\UE_5.6\\UE_5.6\\Engine\\Binaries\\Win64\\UnrealEditor.exe"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $Project)) { throw "LifeLens project not found: $Project" }
if (!(Test-Path $UnrealEditor)) { throw "UnrealEditor not found: $UnrealEditor" }

# Local verification profile for legacy/low-end Windows GPUs such as Intel HD 530.
# Project renderer defaults remain unchanged.
$ExecCmds = @(
    "r.Nanite 0",
    "r.Shadow.Virtual.Enable 0",
    "r.Lumen.DiffuseIndirect.Allow 0",
    "r.Lumen.Reflections.Allow 0",
    "r.DynamicGlobalIlluminationMethod 0",
    "r.ReflectionMethod 0",
    "r.AntiAliasingMethod 1",
    "sg.ViewDistanceQuality 0",
    "sg.ShadowQuality 0",
    "sg.GlobalIlluminationQuality 0",
    "sg.ReflectionQuality 0",
    "sg.PostProcessQuality 0",
    "sg.EffectsQuality 0",
    "sg.FoliageQuality 0",
    "sg.ShadingQuality 0",
    "r.ScreenPercentage 65"
) -join ","

& $UnrealEditor $Project -d3d11 -NoRHIThread -ResX=1280 -ResY=720 -Windowed "-ExecCmds=$ExecCmds"
