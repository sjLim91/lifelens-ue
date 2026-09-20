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

world_presentation = (SOURCE / "LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
require("NaturalChunkPresentationSignature" in world_presentation,
        "world presentation no longer tracks authoritative materialized chunk identity/state")
require("CurrentNaturalChunkSignature != BuiltNaturalChunkSignature" in world_presentation,
        "world presentation can miss same-count materialized chunk changes")
require("BuildGround(World, MaterializedChunks)" in world_presentation,
        "broad ground sizing regressed to count-only materialized chunk inference")

desktop_terrain = (SOURCE / "LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp").read_text(encoding="utf-8")
require("static_cast<uint32>(Facility.GridX)" in desktop_terrain
        and "static_cast<uint32>(Facility.GridY)" in desktop_terrain,
        "desktop terrain signature no longer follows facility positions used for flattening")

resident_character = (SOURCE / "LifeLens/Characters/LLResidentCharacter.cpp").read_text(encoding="utf-8")
world_director = (SOURCE / "LifeLens/World/LLWorldDirector.cpp").read_text(encoding="utf-8")
world_collision = (SOURCE / "LifeLens/World/LLWorldObstacleCollisionProxyActor.cpp").read_text(encoding="utf-8")
require("MaterializedSurfaceCollision" in world_collision,
        "materialized land no longer projects an invisible resident support surface")
require('Chunk.Surface != FName(TEXT("Ocean"))' in world_collision,
        "ocean chunks must not receive walkable resident floor collision")

require("SetMovementPath" in resident_character,
        "resident locomotion lost multi-waypoint route support")
for token in (
    "BuildLocalAStarPath",
    "LLGridOctileHeuristic",
    "No diagonal corner cutting",
    "GetMaterializedNaturalChunkObservations",
    "GetMaterializedSurfaceWaterPresentationObservations",
    "ShoreOffsetCells",
    "bHasMarineNeighbour",
    "MoveResidentToward",
):
    require(token in world_director,
            f"local A* physical executor missing {token}")

core_read_types = (SOURCE / "LifeLens/Simulation/LLCoreReadTypes.h").read_text(encoding="utf-8")
core_bridge = (SOURCE / "LifeLens/Simulation/LLCoreBridgeSubsystem.cpp").read_text(encoding="utf-8")
appearance_profile = (SOURCE / "LifeLens/Simulation/LLAppearanceProfile.cpp").read_text(encoding="utf-8")
appearance_component = (SOURCE / "LifeLens/Characters/LLResidentAppearanceComponent.cpp").read_text(encoding="utf-8")
require("FLLCoreGeneticsSnapshot" in core_read_types,
        "Core inherited genetics are not exposed to presentation DTOs")
for token in (
    "genetics.faceShape",
    "genetics.eyePigment",
    "genetics.hairPigment",
    "genetics.skinTone",
    "genetics.heightPotential",
    "genetics.buildPotential",
):
    require(token in core_bridge,
            f"Core genetic phenotype missing from bridge projection: {token}")
require("MakeGeneticAppearanceProfile" in appearance_profile,
        "appearance still ignores authoritative inherited genetics")
require("ResolveWithGenetics" in appearance_component,
        "resident appearance is not consuming Core genetic phenotype")

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
