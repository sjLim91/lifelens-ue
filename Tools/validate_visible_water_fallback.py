#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWaterPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp").read_text(encoding="utf-8")
config = (root / "Config/DefaultGame.ini").read_text(encoding="utf-8")

for token in (
    "bEnableVisibleWaterFallback = true",
    "FallbackWaterDepthBelowSurfaceUU = 6.0f",
    "FallbackRiverSegments = 4",
    "FallbackChannelInstances",
    "FallbackAreaInstances",
    "FallbackWaterMaterial",
):
    assert token in header, f"visible water fallback header contract missing: {token}"

for token in (
    "/Engine/BasicShapes/Plane.Plane",
    "/Engine/BasicShapes/Cylinder.Cylinder",
    "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial",
    "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
    "SetCanEverAffectNavigation(false)",
    "SetGenerateOverlapEvents(false)",
    "SetCastShadow(false)",
    "EnsureFallbackWaterMaterial",
    "AddFallbackWaterChannel",
    "AddFallbackWaterArea",
    "FallbackChannelInstances->ClearInstances()",
    "FallbackAreaInstances->ClearInstances()",
):
    assert token in cpp, f"visible water fallback implementation missing: {token}"

refresh_start = cpp.index("void ALLWaterPresentationActor::RefreshFromCore")
refresh = cpp[refresh_start:]
linear = refresh[
    refresh.index("if (Water.bLinearChannel && Water.bHasDownstreamTarget)"):
    refresh.index("if (bMarine)")
]
assert linear.index("AddFallbackWaterChannel(") < linear.index(
    "SpawnActor<AWaterBodyRiver>"
), "river safety surface must exist even if WaterBody spawn fails"

marine = refresh[
    refresh.index("if (bMarine)"):
    refresh.index("float RadiusCells = Water.SuggestedAreaRadiusCells")
]
assert "AddFallbackWaterChannel(" in marine
assert marine.index("AddFallbackWaterChannel(") < marine.index(
    "SpawnActor<AWaterBodyLake>"
), "marine safety surface must precede WaterBody spawn"

area = refresh[
    refresh.index("float RadiusCells = Water.SuggestedAreaRadiusCells"):
]
assert area.index("AddFallbackWaterArea(") < area.index(
    "SpawnActor<AWaterBodyLake>"
), "area safety surface must exist even if WaterBody spawn fails"

# Fallback stays below the authored/plugin surface and never becomes collision
# or hydrology authority.
assert "Center.Z -=" in cpp
assert "FallbackWaterDepthBelowSurfaceUU" in cpp
assert "WaterBody->SetActorEnableCollision(false)" in cpp
assert "GetMaterializedSurfaceWaterPresentationObservations()" in cpp

for token in (
    "[/Script/LifeLens.LLWaterPresentationActor]",
    "bEnableVisibleWaterFallback=True",
    "FallbackWaterDepthBelowSurfaceUU=6.000000",
    "FallbackRiverSegments=4",
):
    assert token in config, f"visible water fallback config missing: {token}"

print("LifeLens visible water fallback: PASS")
