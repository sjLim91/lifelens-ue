param(
    [string]$Project = "D:\\LifeLens\\LifeLens.uproject",
    [string]$UnrealEditor = "D:\\Epic Games\\UE_5.6\\UE_5.6\\Engine\\Binaries\\Win64\\UnrealEditor.exe",
    [switch]$LegacyD3D11
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $Project)) { throw "LifeLens project not found: $Project" }
if (!(Test-Path $UnrealEditor)) { throw "UnrealEditor not found: $UnrealEditor" }

# Compatibility profile for low-end Windows GPUs.
#
# Keep DX12 as the default because the production Windows renderer is authored
# for DX12/SM6. A previous helper always forced D3D11, which could put testing on
# a different RHI than production and matched the D3D11RHI GPU-crash reports.
# D3D11 remains available only as an explicit diagnostic/legacy fallback.
$RHIArgs = @("-dx12")
if ($LegacyD3D11) {
    Write-Warning "Launching legacy D3D11 fallback. This is diagnostic-only and not the production renderer path."
    $RHIArgs = @("-d3d11", "-NoRHIThread")
}

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

& $UnrealEditor $Project @RHIArgs -ResX=1280 -ResY=720 -Windowed "-ExecCmds=$ExecCmds"
