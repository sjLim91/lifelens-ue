#include "lifelens/EnvironmentalConsequences.h"
#include "lifelens/Planner.h"
#include "lifelens/PrimitiveFireProgression.h"

#include <cassert>
#include <cmath>

using namespace lifelens;

namespace
{
bool near(double a,double b,double epsilon=1e-12)
{
    return std::abs(a-b)<=epsilon;
}
}

int main()
{
    DynamicEnvironmentObservation mild;
    mild.airTemperatureC = 19.0;
    mild.precipitationIntensity01 = 0.0;
    mild.surfaceWetness01 = 0.10;
    mild.windIntensity01 = 0.10;
    mild.humidity01 = 0.45;
    mild.visibility01 = 1.0;
    const EnvironmentalConsequenceProfile mildProfile = deriveEnvironmentalConsequences(mild);
    assert(mildProfile.heatStress01 == 0.0);
    assert(mildProfile.coldStress01 == 0.0);
    assert(mildProfile.fireReliability01 > 0.80);

    DynamicEnvironmentObservation hot = mild;
    hot.airTemperatureC = 42.0;
    const EnvironmentalConsequenceProfile hotProfile = deriveEnvironmentalConsequences(hot);
    assert(hotProfile.heatStress01 > 0.80);
    assert(hotProfile.perMinuteNeedsDelta.thirst > mildProfile.perMinuteNeedsDelta.thirst);

    DynamicEnvironmentObservation coldWet = mild;
    coldWet.airTemperatureC = -10.0;
    coldWet.precipitationIntensity01 = 0.90;
    coldWet.surfaceWetness01 = 0.95;
    coldWet.windIntensity01 = 0.80;
    coldWet.humidity01 = 0.95;
    coldWet.visibility01 = 0.25;
    const EnvironmentalConsequenceProfile coldWetProfile = deriveEnvironmentalConsequences(coldWet);
    assert(coldWetProfile.coldStress01 > 0.95);
    assert(coldWetProfile.travelFriction01 > mildProfile.travelFriction01);
    assert(coldWetProfile.outdoorWorkFriction01 > mildProfile.outdoorWorkFriction01);
    assert(coldWetProfile.fireReliability01 < mildProfile.fireReliability01);
    assert(coldWetProfile.perMinuteNeedsDelta.hunger > 0.0);
    assert(coldWetProfile.perMinuteNeedsDelta.sleep > 0.0);

    DynamicEnvironmentObservation rainy = mild;
    rainy.precipitationIntensity01 = 0.85;
    rainy.surfaceWetness01 = 0.90;
    rainy.humidity01 = 0.95;
    const EnvironmentalConsequenceProfile rainyProfile = deriveEnvironmentalConsequences(rainy);
    assert(rainyProfile.waterReplenishmentMultiplier > mildProfile.waterReplenishmentMultiplier);

    const int baselineWater = 20;
    const int rainyWater = environmentalRegenerationUnits(MaterialKind::Water, baselineWater, rainyProfile);
    const int dryWater = environmentalRegenerationUnits(MaterialKind::Water, baselineWater, mildProfile);
    assert(rainyWater > dryWater);

    const int plantFood = environmentalRegenerationUnits(MaterialKind::PlantFood, 10, mildProfile);
    assert(plantFood >= 0);

    Needs needs{};
    applyEnvironmentalNeedPressure(needs, hotProfile);
    assert(needs.thirst > 0.0);

    // Integration: a production-style natural world uses the same authoritative
    // climate field for daily renewable resource regeneration.
    World world(20260917);
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.clearGeneratedNaturalWorld();
    const InitialStartRegionSelection selected = world.establishInitialStartRegion();
    world.materializeNaturalChunk(selected.region.coord);
    world.minute = SimulationMinutesPerDay;

    prepareEnvironmentalResourceRegeneration(world);
    bool sawRenewable = false;
    for(const auto& node : world.resourceNodes){
        if(!node.renewable) continue;
        const int baseline = generatedBaselineRegenerationPerDay(world,node.id);
        assert(baseline >= 0);
        const EnvironmentalConsequenceProfile profile = environmentalConsequencesAt(world,node.pos);
        assert(node.regenerationPerDay == environmentalRegenerationUnits(node.material,baseline,profile));
        sawRenewable = true;
    }
    assert(sawRenewable);

    // Integration: environmental pressure is applied to resident Needs by the
    // authoritative minute hook rather than being a visual-only readout.
    Character resident;
    resident.id = 77;
    resident.alive = true;
    resident.needs = {};
    world.characters = {resident};
    const EnvironmentalConsequenceProfile startProfile = deriveEnvironmentalConsequences(
        deriveDynamicEnvironment(world.genesisIdentity(),selected.region.coord,world.minute));
    applyStartRegionEnvironmentalNeedPressure(world);
    assert(near(world.characters.front().needs.hunger,startProfile.perMinuteNeedsDelta.hunger));
    assert(near(world.characters.front().needs.thirst,startProfile.perMinuteNeedsDelta.thirst));
    assert(near(world.characters.front().needs.sleep,startProfile.perMinuteNeedsDelta.sleep));
    assert(near(world.characters.front().needs.hygiene,startProfile.perMinuteNeedsDelta.hygiene));

    // Integration: bad weather increases Core travel ticks deterministically.
    const GridPos from = world.initialStartRegionCenterGrid();
    const GridPos to{from.x+12,from.y};
    const int baseTravel = manhattan(from,to);
    const int adjustedTravel = environmentAdjustedTravelTicks(world,from,to);
    assert(adjustedTravel >= baseTravel);
    assert(adjustedTravel == environmentAdjustedTravelTicks(world,from,to));

    return 0;
}
