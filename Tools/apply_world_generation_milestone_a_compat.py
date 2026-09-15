#!/usr/bin/env python3
from pathlib import Path


def replace_once(path, old, new, required=True):
    p = Path(path)
    s = p.read_text(encoding='utf-8')
    if old not in s:
        if required:
            raise SystemExit(f'anchor not found in {path}: {old[:100]!r}')
        return
    p.write_text(s.replace(old, new, 1), encoding='utf-8')

# The main apply script intentionally mirrors the canonical source shape at the
# point Milestone A was started. These tiny adaptations keep old validators and
# regression fixtures meaningful while the product patch advances their contract.
replace_once(
    'Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h',
    '''inline bool validNaturalResourcePatch(const NaturalResourcePatch& patch, ChunkCoord ownerCoord)\n{\n    return patch.nodeId != 0\n        && validMaterialKind(patch.material)\n''',
    '''inline bool validNaturalResourceMaterial(MaterialKind material)\n{\n    switch(material){\n        case MaterialKind::Water:\n        case MaterialKind::Wood:\n        case MaterialKind::Stone:\n        case MaterialKind::Flint:\n        case MaterialKind::Fiber:\n        case MaterialKind::Clay:\n        case MaterialKind::PlantFood:\n            return true;\n        default:\n            return false;\n    }\n}\n\ninline bool validNaturalResourcePatch(const NaturalResourcePatch& patch, ChunkCoord ownerCoord)\n{\n    return patch.nodeId != 0\n        && validNaturalResourceMaterial(patch.material)\n''')

replace_once(
    'Source/LifeLensCore/tests/test_world_generation_milestone_a.cpp',
    '#include "lifelens/SimulationSnapshotCodec.h"\n',
    '#include "lifelens/SimulationSnapshotCodec.h"\n#include "lifelens/WorldGenerationSnapshotCodec.h"\n')

replace_once(
    'Source/LifeLensCore/tests/test_civilization_persistence.cpp',
    'static ResourceNode* findResource(World& world,ResourceNodeId id)\n{\n    for(auto& node:world.resourceNodes) if(node.id==id) return &node;\n    return nullptr;\n}\n',
    'static ResourceNode* findResource(World& world,MaterialKind material)\n{\n    for(auto& node:world.resourceNodes) if(node.material==material) return &node;\n    return nullptr;\n}\n')
replace_once(
    'Source/LifeLensCore/tests/test_civilization_persistence.cpp',
    '    CHECK(source.world().resourceNodes.size()==7);\n    CHECK(source.world().storageSites.size()==1);\n',
    '    CHECK(!source.world().resourceNodes.empty());\n    CHECK(source.world().generatedNaturalChunks.size()==1);\n    CHECK(source.world().storageSites.empty());\n')
replace_once(
    'Source/LifeLensCore/tests/test_civilization_persistence.cpp',
    '    ResourceNode* flint=findResource(source.world(),2);\n',
    '    ResourceNode* flint=findResource(source.world(),MaterialKind::Flint);\n')
replace_once(
    'Source/LifeLensCore/tests/test_civilization_persistence.cpp',
    '    source.world().storageSites[0].inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,7,0.52,0.91});\n',
    '    StorageSite persistedStorage;\n    persistedStorage.id=7001;\n    source.world().storageSites.push_back(persistedStorage);\n    source.world().storageSites[0].inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,7,0.52,0.91});\n')
# Deliberately keep the legacy-v1 migration expectation at 7 resources / 1
# storage site. v1 never serialized civilization resources, so its decoder
# reconstructs the historical compatibility baseline rather than v6 chunks.

replace_once(
    'Source/LifeLensCore/tests/test_civilization_observer_read_model.cpp',
    '''    sim.world().storageSites[0].inventory.add(\n        {ItemKind::RawMaterial,MaterialKind::Wood,3,0.5,1.0});\n''',
    '''    StorageSite observedStorage;\n    observedStorage.id=7002;\n    sim.world().storageSites.push_back(observedStorage);\n    sim.world().storageSites[0].inventory.add(\n        {ItemKind::RawMaterial,MaterialKind::Wood,3,0.5,1.0});\n''')
replace_once(
    'Source/LifeLensCore/tests/test_civilization_observer_read_model.cpp',
    '    CHECK(worldRead.resourceNodeCount==7);\n',
    '    CHECK(worldRead.resourceNodeCount==static_cast<int>(sim.world().resourceNodes.size()));\n')

replace_once(
    'Source/LifeLensCore/tests/test_primitive_latrine_progression.cpp',
    '    // The outer snapshot format remains v5; only the sanitation extension\n    // advances to v2. Roundtrip must preserve the same facility identity and\n',
    '    // The outer snapshot format may advance as new authoritative extensions\n    // are added. Roundtrip must preserve the same facility identity and\n')
replace_once(
    'Source/LifeLensCore/tests/test_primitive_latrine_progression.cpp',
    '    assert(bytes[8]==5 && bytes[9]==0 && bytes[10]==0 && bytes[11]==0);\n',
    '    assert(bytes[8]==static_cast<std::uint8_t>(SimulationSnapshotBinaryFormatVersion)\n        && bytes[9]==0 && bytes[10]==0 && bytes[11]==0);\n')

p = Path('Source/LifeLens/World/LLWorldDirector.cpp')
s = p.read_text(encoding='utf-8')
s = s.replace(
    'EnvironmentalResidueVisualizer->RefreshFromCore(\n            *CoreBridge, CoreGridCellSizeUU, true,\n',
    'EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, true,\n',
    1)
s = s.replace(
    'EnvironmentalResidueVisualizer->RefreshFromCore(\n            *CoreBridge, CoreGridCellSizeUU, false,\n',
    'EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, false,\n',
    1)
p.write_text(s, encoding='utf-8')

replace_once(
    'Tools/validate_bootstrap.py',
    "assert 'SimulationSnapshotBinaryFormatVersion=5' in snapshot_h, 'Designated sanitation site persistence requires snapshot binary format v5'",
    "snapshot_binary_version = int(snapshot_h.split('SimulationSnapshotBinaryFormatVersion=', 1)[1].split(';', 1)[0])\nassert snapshot_binary_version >= 5, 'Designated sanitation site persistence requires snapshot binary format v5 or newer'")

print('Milestone A compatibility adjustments: PASS')
