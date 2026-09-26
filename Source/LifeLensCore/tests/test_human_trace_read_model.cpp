#include <cassert>
#include <string>
#include "lifelens/HumanTraceReadModel.h"

using namespace lifelens;

int main()
{
    World world(42);
    world.resourceNodes.clear();
    world.generatedNaturalChunks.clear();
    world.facilities.clear();
    world.environmentalResidues.clear();
    assert(buildHumanTraceWindowObservation(world, {0,0}, 1).entries.empty());

    // Fresh natural baselines and compatibility fixtures do not imply use.
    GeneratedNaturalChunk natural;
    natural.coord = {-1,-1};
    natural.resourcePatches.push_back({9007199254740993ULL, MaterialKind::Wood, {-1,-2}, 10, 10, true, 2});
    world.generatedNaturalChunks.push_back(natural);
    world.resourceNodes.push_back({9007199254740993ULL, MaterialKind::Wood, 10, 10, true, 2, {-1,-2}});
    world.resourceNodes.push_back({55, MaterialKind::Stone, 2, 10, false, 0, {-1,-2}});
    assert(buildHumanTraceWindowObservation(world, {-1,-1}, 0).entries.empty());

    Inventory collected;
    world.resourceNodes.front().harvest(4, collected);
    const auto used = buildHumanTraceWindowObservation(world, {-1,-1}, 0);
    assert(used.total == 1 && used.entries.size() == 1);
    assert(used.entries.front().id == "resource:9007199254740993");
    assert(used.entries.front().quantity == 6 && used.entries.front().baselineQuantity == 10);
    assert(used.entries.front().pos.x == -1 && used.entries.front().pos.y == -2);
    assert(buildHumanTraceWindowObservation(world, {0,0}, 0).entries.empty());
    assert(world.resourceNodes.front().quantity == 6); // observing cannot consume resources
    world.resourceNodes.front().regenerateDay();
    world.resourceNodes.front().regenerateDay();
    assert(buildHumanTraceWindowObservation(world, {-1,-1}, 0).entries.empty());

    world.environmentalResidues.deposit(EnvironmentalResidueKind::HumanWaste, {-2,-2}, 7, 480);
    auto residues = buildHumanTraceWindowObservation(world, {-1,-1}, 0);
    assert(residues.total == 1);
    assert(residues.entries.front().kind == HumanTraceKind::Residue);
    assert(residues.entries.front().sourceCharacter == 7);
    world.environmentalResidues.advanceToMinute(20000);
    assert(buildHumanTraceWindowObservation(world, {-1,-1}, 0).entries.empty());

    auto facility = makeFacilityConstructionSite(1, FacilityKind::PrimitiveStorage, {-3,-3}, 7, 480);
    world.facilities.push_back(facility);
    assert(buildHumanTraceWindowObservation(world, {-1,-1}, 0).entries.empty());
    world.facilities.front().requirements.front().delivered = 1;
    const auto materials = buildHumanTraceWindowObservation(world, {-1,-1}, 0);
    assert(materials.total == 1);
    assert(materials.entries.front().deliveredMaterialUnits == 1);
    assert(!materials.entries.front().active && !materials.entries.front().lit);
    world.facilities.front().constructionWork = 4;
    world.facilities.front().state = FacilityState::UnderConstruction;
    const auto building = buildHumanTraceWindowObservation(world, {-1,-1}, 0);
    assert(building.entries.front().progress01 == 0.5);

    world.facilities.clear();
    for (int i = 0; i < 80; ++i) {
        world.environmentalResidues.deposit(EnvironmentalResidueKind::HumanWaste, {1000+i,1000}, 1, 480);
        world.environmentalResidues.deposit(EnvironmentalResidueKind::HumanWaste, {i%16,i/16}, 2, 480);
    }
    const auto bounded = buildHumanTraceWindowObservation(world, {0,0}, 0);
    assert(bounded.total == 80 && bounded.entries.size() == 64);
    for (const auto& trace : bounded.entries) {
        assert(trace.sourceCharacter == 2);
        assert(trace.pos.x >= 0 && trace.pos.x < 32);
    }
    const auto repeat = buildHumanTraceWindowObservation(world, {0,0}, 0);
    for (std::size_t i = 0; i < bounded.entries.size(); ++i)
        assert(bounded.entries[i].id == repeat.entries[i].id);
    return 0;
}
