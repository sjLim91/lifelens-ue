from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source"
CONFIG = ROOT / "Config"

TEXT_SUFFIXES = {".h", ".hpp", ".cpp", ".c", ".cs"}
PRODUCTION_ROOTS = (
    SOURCE / "LifeLens",
    SOURCE / "LifeLensCore" / "include",
    SOURCE / "LifeLensCore" / "src",
)

def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)

def production_files():
    files = []
    for root in PRODUCTION_ROOTS:
        if not root.exists():
            continue
        files.extend(
            path for path in root.rglob("*")
            if path.is_file() and path.suffix.lower() in TEXT_SUFFIXES
        )
    return sorted(set(files))

files = production_files()
require(len(files) >= 130, f"unexpectedly small production source set: {len(files)}")

conflict_markers = []
marine_deferred = []
for path in files:
    text = path.read_text(encoding="utf-8", errors="strict")
    rel = path.relative_to(ROOT).as_posix()

    if any(marker in text for marker in ("<<<<<<< ", ">>>>>>> ", "||||||| ")):
        conflict_markers.append(rel)

    # Fixed prototype founder names must never become production identity.
    if re.search(r"(서윤|하린|민재|태훈|\bSeoyun\b|\bHarin\b|\bMinjae\b|\bTaehoon\b)", text):
        hardcoded_founders.append(rel)

    if "marineDeferred" in text:
        marine_deferred.append(rel)

require(not conflict_markers, f"merge conflict markers in production source: {conflict_markers}")
require(not marine_deferred, f"coast/ocean presentation still explicitly deferred: {marine_deferred}")

initial_population = (SOURCE / "LifeLensCore/include/lifelens/InitialPopulation.h").read_text(encoding="utf-8")
require("std::shuffle(maleNames.begin(),maleNames.end(),rng);" in initial_population,
        "male founder names are no longer randomized")
require("std::shuffle(femaleNames.begin(),femaleNames.end(),rng);" in initial_population,
        "female founder names are no longer randomized")
require('generateFounder(1,maleNames[0]' in initial_population
        and 'generateFounder(2,maleNames[1]' in initial_population
        and 'generateFounder(3,femaleNames[0]' in initial_population
        and 'generateFounder(4,femaleNames[1]' in initial_population,
        "production founders regressed to fixed display names")
require(not re.search(r"generateFounder\\([^,]+,\\s*\"", initial_population),
        "production founder construction contains a literal fixed name")

hierarchy = (SOURCE / "LifeLensCore/include/lifelens/WorldHierarchy.h").read_text(encoding="utf-8")
for token in (
    "PlanetIdentity",
    "SurfaceRegionIdentity",
    "ObserverWorldScale",
    "LocalSurface",
    "Regional",
    "Planetary",
    "Orbital",
    "Interplanetary",
):
    require(token in hierarchy, f"world hierarchy contract missing {token}")

macro = (SOURCE / "LifeLensCore/include/lifelens/MacroWorldGenesis.h").read_text(encoding="utf-8")
for token in ("MacroSeaLevel01", "MacroSurfaceClass::Coast", "MacroSurfaceClass::Ocean"):
    require(token in macro, f"macro Earth surface contract missing {token}")

hydrology = (SOURCE / "LifeLensCore/include/lifelens/Hydrology.h").read_text(encoding="utf-8")
for token in (
    "SurfaceWaterKind::Coast",
    "SurfaceWaterKind::Ocean",
    "WaterSalinity::Brackish",
    "WaterSalinity::Salt",
    "hasMarineNeighbour",
):
    require(token in hydrology, f"hydrology marine contract missing {token}")

natural = (SOURCE / "LifeLensCore/include/lifelens/NaturalWorldChunk.h").read_text(encoding="utf-8")
for token in (
    "NaturalSurfaceKind::Coast",
    "NaturalSurfaceKind::Ocean",
    "macroSurface.surfaceClass == MacroSurfaceClass::Ocean",
):
    require(token in natural, f"natural world marine coherence missing {token}")

water = (SOURCE / "LifeLens/WorldPresentation/LLWaterPresentationActor.cpp").read_text(encoding="utf-8")
for token in (
    "marineLocalSurface",
    "Water.bHasMarineNeighbour",
    "ELLCoreSurfaceWaterKind::Coast",
    "ELLCoreSurfaceWaterKind::Ocean",
):
    require(token in water, f"marine presentation missing {token}")

windows_ini = (CONFIG / "Windows/WindowsEngine.ini").read_text(encoding="utf-8")
require("DefaultGraphicsRHI=DefaultGraphicsRHI_DX12" in windows_ini,
        "Windows production renderer must remain DX12")
require("DefaultGraphicsRHI=DefaultGraphicsRHI_DX11" not in windows_ini,
        "Windows production config regressed to DX11")

print(
    "Full production source scan: PASS "
    f"({len(files)} files; conflict/founder-randomization/marine-defer/earth-hierarchy guards)"
)
