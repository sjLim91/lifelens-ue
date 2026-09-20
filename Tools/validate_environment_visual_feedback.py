from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def require(blob: str, token: str, where: str) -> None:
    if token not in blob:
        raise SystemExit(f"missing {token!r} in {where}")


h = text("Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.h")
cpp = text("Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp")
director_h = text("Source/LifeLens/World/LLWorldDirector.h")
director_cpp = text("Source/LifeLens/World/LLWorldDirector.cpp")
read_types = text("Source/LifeLens/Simulation/LLEnvironmentReadTypes.h")
doc = text("docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md")
dynamic_h = text("Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.h")
dynamic_cpp = text("Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.cpp")

for token in [
    "UHierarchicalInstancedStaticMeshComponent",
    "MaxResidueInstances = 128",
    "RefreshFromCore",
    "GetVisualInstanceCount",
]:
    require(h, token, "residue visualizer header")

for token in [
    "GetEnvironmentObservation",
    "ClearInstances()",
    "AddInstance(InstanceTransform, true)",
    "SetCustomDataValue",
    "LineTraceSingleByChannel",
    "ELLCoreEnvironmentalResidueKind::HumanWaste",
    "GetTypeHash(CoreOriginGridX)",
    "GetTypeHash(CoreOriginGridY)",
    "SurfaceOffsetUU * 1000.0f",
    "Owner->GetActorLocation()",
    "static_cast<uint8>(Residue.Kind)",
    "Environment, CellSize, CoreOriginGridX, CoreOriginGridY",
]:
    require(cpp, token, "residue visualizer implementation")

for forbidden in ["deposit(", "containHumanWasteAt(", "CompleteResidentPhysicalAction("]:
    if forbidden in cpp:
        raise SystemExit(f"presentation must not mutate simulation: found {forbidden!r}")

for token in ["ResidueId", "GridX", "GridY", "Amount", "Intensity", "RadiusTiles"]:
    require(read_types, token, "environment read DTO")

for token in [
    "EnvironmentalResidueVisualizer",
    "EnvironmentalVisualRefreshIntervalSeconds",
    "RefreshFromCore(*CoreBridge",
]:
    require(director_h + director_cpp, token, "WorldDirector integration")

for token in [
    "0.42",
    "0.16",
    "radius 3",
    "radius 1",
    "HISM",
]:
    require(doc, token, "environment visual feedback canonical doc")

for token in [
    "LastObservedRuntimeGeneration",
]:
    require(dynamic_h, token, "dynamic environment runtime reset header")

for token in [
    "Bridge->GetRuntimeGeneration()",
    "RuntimeGeneration != LastObservedRuntimeGeneration",
    "bRuntimeReplaced",
    "bForce || bRuntimeReplaced",
]:
    require(dynamic_cpp, token, "dynamic environment runtime reset implementation")

print("Environmental visual feedback structural validation: PASS")
