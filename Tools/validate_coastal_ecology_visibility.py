#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

# Ocean remains explicitly vegetation-free; coast is terrestrial and must not be
# hard-zeroed solely because it borders marine water.
for token in (
    'const bool bOceanSurface = Chunk.Surface == FName(TEXT("Ocean"));',
    'const bool bCoastSurface = Chunk.Surface == FName(TEXT("Coast"));',
    'CoastTreeDensityScale = bCoastSurface ? 0.32f : 1.0f',
    'CoastShrubDensityScale = bCoastSurface ? 0.55f : 1.0f',
    'CoastGrassDensityScale = bCoastSurface ? 0.72f : 1.0f',
):
    assert token in cpp, f"coastal ecology presentation contract missing: {token}"

ocean_start = cpp.index('if (bOceanSurface)')
ocean_end = cpp.index('const FVector ChunkOrigin', ocean_start)
ocean_block = cpp[ocean_start:ocean_end]
assert "return;" in ocean_block, "ocean chunks must remain free of terrestrial dressing"

for forbidden in (
    'const int32 TreeCount = bCoastSurface ? 0',
    'const int32 ShrubCount = bCoastSurface ? 0',
    'const int32 GrassCount = bCoastSurface ? 0',
):
    assert forbidden not in cpp, f"coast must not hard-zero ambient ecology: {forbidden}"

resource_start = cpp.index(
    'for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)'
)
resource_end = cpp.index(
    '// Decorative ecology is budgeted only after every authoritative obstacle',
    resource_start,
)
resource_block = cpp[resource_start:resource_end]
assert 'if (bCoastSurface' not in resource_block, (
    "coast must not hide authoritative resource patches; decorative thinning is separate"
)
for token in (
    'Material.Contains(TEXT("wood"))',
    'Material.Contains(TEXT("stone"))',
    'Material.Contains(TEXT("berry"))',
    'Component->AddInstance',
):
    assert token in resource_block, f"authoritative resource projection missing: {token}"

print("LifeLens coastal ecology visibility: PASS")
