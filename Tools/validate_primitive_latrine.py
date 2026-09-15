from pathlib import Path

root = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = root / path
    assert p.exists(), f"Missing file: {path}"
    return p.read_text(encoding="utf-8")

civilization = read("Source/LifeLensCore/include/lifelens/Civilization.h")
for token in (
    "DugSanitationPit",
    "DigSanitationPit",
    "sanitationPitCandidateAvailable",
    "TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible",
):
    assert token in civilization, f"Missing dug-pit discovery contract: {token}"
assert "LatrineUnlocked" not in civilization

sanitation = read("Source/LifeLensCore/include/lifelens/PrimitiveSanitation.h")
for token in (
    "PrimitiveSanitationSiteKind::DugPit",
    "DugSanitationPitWorkRequired",
    "evaluateDugSanitationPitOpportunity",
    "workOnDugSanitationPit",
    "site->kind=PrimitiveSanitationSiteKind::DugPit",
    "field.containHumanWasteAt",
    "recordPrimitiveSanitationSiteUse",
    "primitiveSanitationResidueIntensity",
    "primitiveSanitationResidueRadiusTiles",
):
    assert token in sanitation, f"Missing dug-pit facility contract: {token}"
assert "LatrineUnlocked" not in sanitation

environment = read("Source/LifeLensCore/include/lifelens/EnvironmentalResidue.h")
for token in (
    "containHumanWasteAt",
    "record.intensity=std::max(0.01,record.intensity*intensityFactor)",
    "record.radiusTiles=std::min(record.radiusTiles,maxRadiusTiles)",
):
    assert token in environment, f"Missing waste-containment consequence: {token}"

decision = read("Source/LifeLensCore/include/lifelens/CivilizationDecision.h")
for token in (
    "ExperimentKind::DigSanitationPit",
    "TechniqueId::DugSanitationPit",
    "canWorkOnDugSanitationPit",
    "workOnDugSanitationPit",
    "sanitationImprovementCompleted",
):
    assert token in decision, f"Missing dug-pit decision/execution wiring: {token}"

simulation = read("Source/LifeLensCore/include/lifelens/Simulation.h")
for token in (
    "validPrimitiveSanitationSiteKind",
    "primitiveSanitationUseEffectPerTick",
    "recordPrimitiveSanitationSiteUse",
    "primitiveSanitationResidueIntensity",
    "primitiveSanitationResidueRadiusTiles",
):
    assert token in simulation, f"Missing primitive sanitation physical ACK contract: {token}"

snapshot_header = read("Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h")
assert "SimulationSnapshotBinaryFormatVersion=5" in snapshot_header, "Outer snapshot format must remain v5"

sanitation_snapshot = read("Source/LifeLensCore/include/lifelens/PrimitiveSanitationSnapshotCodec.h")
for token in (
    "PrimitiveSanitationSnapshotExtensionVersion=2",
    "MinimumPrimitiveSanitationSnapshotExtensionVersion=1",
    "site.improvementWork",
    "site.improvedBy",
    "site.improvedMinute",
    "extensionVersion>=2",
):
    assert token in sanitation_snapshot, f"Missing sanitation extension compatibility: {token}"

civilization_snapshot = read("Source/LifeLensCore/include/lifelens/CivilizationSnapshotCodec.h")
assert "TechniqueId::DugSanitationPit" in civilization_snapshot

transmission = read("Source/LifeLensCore/include/lifelens/CivilizationKnowledgeTransmission.h")
assert "raw<=static_cast<int>(TechniqueId::DugSanitationPit)" in transmission
assert "TechniqueId::DugSanitationPit" in transmission

observer = read("Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h")
assert "TechniqueId::DugSanitationPit" in observer

unreal_types = read("Source/LifeLens/Simulation/LLCivilizationReadTypes.h")
assert "DesignatedSanitationArea" in unreal_types
assert "DugSanitationPit" in unreal_types

unreal_bridge = read("Source/LifeLens/Simulation/LLCoreBridgeCivilization.cpp")
assert "ELLCoreTechniqueId::DesignatedSanitationArea" in unreal_bridge
assert "ELLCoreTechniqueId::DugSanitationPit" in unreal_bridge

cmake = read("Source/LifeLensCore/CMakeLists.txt")
assert "lifelens_add_test(test_primitive_latrine_progression)" in cmake
read("Source/LifeLensCore/tests/test_primitive_latrine_progression.cpp")

print("Primitive latrine progression structural validation: PASS")
