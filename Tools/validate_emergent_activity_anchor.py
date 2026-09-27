#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

settlement = (root / "Source/LifeLensCore/include/lifelens/SettlementProgression.h").read_text(encoding="utf-8")
storage = (root / "Source/LifeLensCore/include/lifelens/PrimitiveStorageProgression.h").read_text(encoding="utf-8")
fire = (root / "Source/LifeLensCore/include/lifelens/PrimitiveFireProgression.h").read_text(encoding="utf-8")
smelting = (root / "Source/LifeLensCore/include/lifelens/PrimitiveSmeltingProgression.h").read_text(encoding="utf-8")
decision = (root / "Source/LifeLensCore/include/lifelens/CivilizationDecision.h").read_text(encoding="utf-8")
context = (root / "Source/LifeLensCore/include/lifelens/ContextAction.h").read_text(encoding="utf-8")
simulation = (root / "Source/LifeLensCore/src/Simulation.cpp").read_text(encoding="utf-8")
completion = (root / "Source/LifeLensCore/src/ContextActionRuntime.cpp").read_text(encoding="utf-8")
regression = (root / "Source/LifeLensCore/tests/test_settlement_autonomy.cpp").read_text(encoding="utf-8")

def block(text: str, start: str, end: str) -> str:
    a = text.index(start)
    b = text.index(end, a)
    return text[a:b]

site_blocks = (
    block(settlement, "inline SettlementFacilitySiteOpportunity chooseSettlementFacilitySite(", "inline ConstructedFacility* establishSettlementFacilityProject"),
    block(storage, "inline PrimitiveStorageSiteOpportunity choosePrimitiveStorageSite(", "inline ConstructedFacility* establishPrimitiveStorageProject"),
    block(fire, "inline PrimitiveFirePitSiteOpportunity choosePrimitiveFirePitSite(", "inline int primitiveFirePitMissingMaterial"),
    block(smelting, "inline PrimitiveFurnaceSiteOpportunity choosePrimitiveFurnaceSite(", "inline bool primitiveFurnaceKnowledgeReady"),
)

for candidate in site_blocks:
    assert "GridPos activityAnchor" in candidate, "facility site selection must receive an explicit activity anchor"
    assert "const GridPos center=activityAnchor;" in candidate, "facility site search must center on activity"
    assert "initialStartRegionCenterGrid()" not in candidate, "spawn regained facility-placement authority"

for token in (
    "bestExperimentDecisionAtPosition",
    "const GridPos sanitationReference=authoritativePosition;",
    "bestPrimitiveStorageConstructionDecision(\n            world,self,authoritativePosition)",
    "bestPrimitiveFirePitDecision(\n            world,self,authoritativePosition)",
    "bestPrimitiveFurnaceDecision(\n            world,self,authoritativePosition)",
    "chooseSettlementFacilitySite(\n                    world,self.id,kind,authoritativePosition)",
    "executeCivilizationDecisionAtPosition",
):
    assert token in decision, f"civilization activity-position chain missing: {token}"

resolve_block = block(
    context,
    "inline bool resolveCivilizationContextTarget(",
    "inline bool civilizationContextRequiresSpatialTarget",
)
assert "GridPos authoritativePosition" in resolve_block
assert "civilizationSanitationReferencePosition(world)" not in resolve_block
for token in (
    "world,actor.id,decision.facilityKind,\n                            authoritativePosition",
    "world,actor.id,authoritativePosition",
):
    assert token in resolve_block, f"context target still bypasses actor position: {token}"

assert "world_,c,decision.civilization,r.pos,target,sanitationSiteId" in simulation
assert "executeCivilizationDecisionAtPosition(\n                    world_,actor,decision,resolvedPosition)" in completion

# Regression must explicitly prove the emergent center can diverge from NEW GAME
# spawn and that settlement selection follows lived activity instead.
for token in (
    "const GridPos emergentCenter{layoutCenter.x+12,layoutCenter.y};",
    "FacilityKind::Shelter,emergentCenter",
    "CHECK(manhattan(clustered.pos,emergentCenter)<=16);",
    "CHECK(!facilityFootprintsConflict(",
    "CHECK(manhattan(clustered.pos,layoutCenter)>=6);",
):
    assert token in regression, f"spawn-independence regression missing: {token}"

print("LifeLens emergent activity anchor: PASS")
