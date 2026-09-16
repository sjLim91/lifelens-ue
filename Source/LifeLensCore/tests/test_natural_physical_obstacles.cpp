#include <cstdint>
#include <iostream>
#include <unordered_set>

#include "lifelens/NaturalPhysicalObstacle.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

int main()
{
    GeneratedNaturalChunk chunk;
    chunk.coord={0,0};
    chunk.chunkSeed=12345;
    chunk.generationVersion=CurrentWorldGenerationVersion;

    NaturalResourcePatch wood;
    wood.nodeId=101;
    wood.material=MaterialKind::Wood;
    wood.pos={6,6};
    wood.baselineQuantity=120;
    wood.maxQuantity=120;
    wood.renewable=true;
    wood.regenerationPerDay=4;
    wood.visualDensity=0.75;
    wood.detailSeed=111111;

    NaturalResourcePatch stone;
    stone.nodeId=102;
    stone.material=MaterialKind::Stone;
    stone.pos={10,10};
    stone.baselineQuantity=90;
    stone.maxQuantity=90;
    stone.visualDensity=0.80;
    stone.detailSeed=222222;

    NaturalResourcePatch fiber;
    fiber.nodeId=103;
    fiber.material=MaterialKind::Fiber;
    fiber.pos={14,14};
    fiber.baselineQuantity=80;
    fiber.maxQuantity=80;
    fiber.renewable=true;
    fiber.regenerationPerDay=7;
    fiber.visualDensity=1.0;
    fiber.detailSeed=333333;

    chunk.resourcePatches={wood,stone,fiber};

    const auto first=deriveNaturalPhysicalObstacles(chunk);
    const auto second=deriveNaturalPhysicalObstacles(chunk);
    CHECK(first.size()==8);
    CHECK(second.size()==first.size());

    int trees=0;
    int rocks=0;
    std::unordered_set<std::uint64_t> ids;
    for(std::size_t i=0;i<first.size();++i){
        const auto& a=first[i];
        const auto& b=second[i];
        CHECK(a.id==b.id);
        CHECK(a.kind==b.kind);
        CHECK(a.sourceNodeId==b.sourceNodeId);
        CHECK(a.grid.x==b.grid.x && a.grid.y==b.grid.y);
        CHECK(a.offsetXCells==b.offsetXCells);
        CHECK(a.offsetYCells==b.offsetYCells);
        CHECK(a.halfExtentXCells==b.halfExtentXCells);
        CHECK(a.halfExtentYCells==b.halfExtentYCells);
        CHECK(a.halfHeightCells==b.halfHeightCells);
        CHECK(a.styleSeed==b.styleSeed);
        CHECK(validNaturalPhysicalObstacle(a,chunk.coord));
        CHECK(ids.insert(a.id).second);
        CHECK(a.sourceNodeId!=fiber.nodeId);
        if(a.kind==NaturalPhysicalObstacleKind::Tree) ++trees;
        if(a.kind==NaturalPhysicalObstacleKind::Rock) ++rocks;
    }
    CHECK(trees==5);
    CHECK(rocks==3);

    wood.visualDensity=0.0;
    CHECK(naturalPhysicalObstacleCount(wood)==2);
    stone.visualDensity=0.20;
    CHECK(naturalPhysicalObstacleCount(stone)==0);
    CHECK(!materialCreatesNaturalPhysicalObstacle(MaterialKind::Fiber));

    std::cout << "authoritative natural physical obstacle derivation passed\n";
    return 0;
}
