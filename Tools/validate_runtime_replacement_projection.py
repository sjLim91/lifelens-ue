from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIRECTOR_H = (ROOT / "Source/LifeLens/World/LLWorldDirector.h").read_text(encoding="utf-8")
DIRECTOR_CPP = (ROOT / "Source/LifeLens/World/LLWorldDirector.cpp").read_text(encoding="utf-8")
SIM_H = (ROOT / "Source/LifeLens/Simulation/LLSimulationSubsystem.h").read_text(encoding="utf-8")
SIM_CPP = (ROOT / "Source/LifeLens/Simulation/LLSimulationSubsystem.cpp").read_text(encoding="utf-8")
BRIDGE_H = (ROOT / "Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h").read_text(encoding="utf-8")

def require(blob: str, token: str, label: str) -> None:
    if token not in blob:
        raise AssertionError(f"{label}: missing {token}")

require(BRIDGE_H, "GetRuntimeGeneration() const", "Core runtime generation")
for token in (
    "ObservedCoreRuntimeGeneration",
    "SynchronizeAfterCoreRuntimeReplacement",
):
    require(DIRECTOR_H, token, "WorldDirector runtime replacement state")

for token in (
    "CurrentRuntimeGeneration != ObservedCoreRuntimeGeneration",
    "SynchronizeAfterCoreRuntimeReplacement();",
    "ReleasePhysicalReservation",
    "Character->Destroy();",
    "SpawnedResidents.Reset();",
    "SimulationClockAccumulator = 0.0f;",
    "EnvironmentalVisualRefreshAccumulator = 0.0f;",
    "Simulation->SynchronizeProjectionFromCore();",
    "RefreshCorePresentationOrigin();",
    "SpawnResidents();",
    "EnvironmentalResidueVisualizer->ClearInstances();",
    "RefreshFromCore(",
):
    require(DIRECTOR_CPP, token, "WorldDirector replacement rebuild")

require(SIM_H, "bool SynchronizeProjectionFromCore();", "Simulation projection API")
for token in (
    "bCoreAuthoritativeRuntime = RefreshProjectionFromCore();",
    "Residents.Reset();",
    "Relationships.Reset();",
    "OnSimulationStateChanged.Broadcast();",
):
    require(SIM_CPP, token, "Simulation projection synchronization")

print("LifeLens runtime replacement projection validation: PASS")
