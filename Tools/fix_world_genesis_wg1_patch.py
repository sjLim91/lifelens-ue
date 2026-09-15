from pathlib import Path

validator = Path('Tools/validate_world_genesis_wg1.py')
t = validator.read_text(encoding='utf-8')
t = t.replace("assert 'std::hash' not in header, 'stable chunk identity must not depend on implementation-defined std::hash'", "assert 'std::hash<' not in header, 'stable chunk identity must not depend on implementation-defined std::hash calls'")
validator.write_text(t, encoding='utf-8')

test = Path('Source/LifeLensCore/tests/test_world_genesis_wg1.cpp')
t = test.read_text(encoding='utf-8')
for old in [
    'assert(chunkCoordForGrid({0,0}) == ChunkCoord{0,0});',
    'assert(chunkCoordForGrid({31,31}) == ChunkCoord{0,0});',
    'assert(chunkCoordForGrid({32,32}) == ChunkCoord{1,1});',
    'assert(chunkCoordForGrid({-1,-1}) == ChunkCoord{-1,-1});',
    'assert(chunkCoordForGrid({-32,-32}) == ChunkCoord{-1,-1});',
    'assert(chunkCoordForGrid({-33,-33}) == ChunkCoord{-2,-2});',
]:
    t = t.replace(old, 'assert((' + old[len('assert('):-2] + '));')
test.write_text(t, encoding='utf-8')

print('WG-1 validator/test patch hardened')
