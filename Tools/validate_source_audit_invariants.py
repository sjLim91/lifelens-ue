from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)

# AUDIT-0A: one canonical production time/weather/speed control surface.
require(
    not (ROOT / "Source/LifeLens/UI/LLRuntimeObserverHUD.cpp").exists(),
    "AUDIT-0A regression: duplicate LLRuntimeObserverHUD.cpp returned",
)
require(
    not (ROOT / "Source/LifeLens/UI/LLRuntimeObserverHUD.h").exists(),
    "AUDIT-0A regression: duplicate LLRuntimeObserverHUD.h returned",
)

game_mode = read("Source/LifeLens/Core/LLLifeLensGameMode.cpp")
require(
    '#include "UI/LLSocialObserverHUD.h"' in game_mode
    and "HUDClass = ALLSocialObserverHUD::StaticClass();" in game_mode,
    "AUDIT-0A regression: production GameMode must use ALLSocialObserverHUD",
)

ui_root = ROOT / "Source/LifeLens/UI"
speed_mutators = []
for path in sorted(ui_root.glob("*.cpp")):
    text = path.read_text(encoding="utf-8")
    if "SetSimulationSpeedPreset(" in text:
        speed_mutators.append(path.name)

require(
    speed_mutators == ["LLObserverTimeWeatherOverlay.cpp"],
    f"AUDIT-0A regression: speed control must have one UI mutation path, got {speed_mutators}",
)

overlay = read("Source/LifeLens/UI/LLObserverTimeWeatherOverlay.cpp")
require(
    "ULLObserverTimeWeatherPresentationSubsystem::Tick" in overlay
    and "OverlayWidget->AddToViewport(65);" in overlay,
    "AUDIT-0A regression: canonical observer time/weather overlay is not active",
)

# AUDIT-0B: environmental Need pressure follows authoritative resident position.
fire = read("Source/LifeLensCore/include/lifelens/PrimitiveFireProgression.h")
simulation = read("Source/LifeLensCore/src/Simulation.cpp")
require(
    "applyStartRegionEnvironmentalNeedPressure" not in fire
    and "applyStartRegionEnvironmentalNeedPressure" not in simulation,
    "AUDIT-0B regression: shared start-region environmental Need pressure returned",
)
require(
    "applyResidentEnvironmentalNeedPressure" in fire,
    "AUDIT-0B regression: resident-local environmental pressure helper missing",
)
require(
    "runtimeIt->second.pos" in simulation
    and "applyResidentEnvironmentalNeedPressure(world_,character,position);" in simulation,
    "AUDIT-0B regression: Simulation::step is not using authoritative resident runtime position",
)

# Direct observed Social activities bypass pending ContextAction, so WorldDirector
# must open/close the motion presentation gate explicitly at the resolved target.
world_director = read("Source/LifeLens/World/LLWorldDirector.cpp")
for token in (
    'Characters/LLResidentMotionComponent.h',
    'SetSocialInteractionActive(false);',
    'SetWorkPresentationMode(',
    'ELLResidentWorkPresentationMode::None',
    'SetHeldToolPresentation(',
    'ELLResidentHeldToolPresentation::None',
    'SetSocialInteractionActive(true);',
):
    require(
        token in world_director,
        f"direct social/context presentation reset contract missing {token}",
    )

print("Whole-source audit invariants: PASS")
