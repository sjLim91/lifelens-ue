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

    Character resident;
    resident.id = 77;
    resident.alive = true;
    resident.needs = {};
    world.characters = {resident};
    const EnvironmentalConsequenceProfile startProfile = deriveEnvironmentalConsequences(
        deriveDynamicEnvironment(world.genesisIdentity(),selected.region.coord,world.minute));
    applyResidentEnvironmentalNeedPressure(
        world,
        world.characters.front(),
        world.initialStartRegionCenterGrid());
    assert(near(world.characters.front().needs.hunger,startProfile.perMinuteNeedsDelta.hunger));
    assert(near(world.characters.front().needs.thirst,startProfile.perMinuteNeedsDelta.thirst));
    assert(near(world.characters.front().needs.sleep,startProfile.perMinuteNeedsDelta.sleep));
    assert(near(world.characters.front().needs.hygiene,startProfile.perMinuteNeedsDelta.hygiene));

    const GridPos from = world.initialStartRegionCenterGrid();
    const GridPos to{from.x+12,from.y};
    const int baseTravel = manhattan(from,to);
    const int adjustedTravel = environmentAdjustedTravelTicks(world,from,to);
    assert(adjustedTravel >= baseTravel);
    assert(adjustedTravel == environmentAdjustedTravelTicks(world,from,to));

    // Integration: residents in different authoritative chunks receive local
    // environmental Need pressure during Simulation::step().
    Simulation localPressure(20260918);
    localPressure.setupNewGame();
    SimulationStateSnapshot snapshot = localPressure.captureSnapshot();
    assert(snapshot.world.characters.size() >= 2);
    snapshot.world.minute = 123;

    Character& firstResident = snapshot.world.characters[0];
    Character& secondResident = snapshot.world.characters[1];
    firstResident.needs = {};
    secondResident.needs = {};
    firstResident.metabolism = 1.0;
    secondResident.metabolism = 1.0;
    firstResident.sleepTendency = 1.0;
    secondResident.sleepTendency = 1.0;
    snapshot.runtime[firstResident.id].plan.clear();
    snapshot.runtime[secondResident.id].plan.clear();

    const WorldGenesisIdentity identity = snapshot.world.genesisIdentity();
    ChunkCoord firstCoord = snapshot.world.initialStartRegionCoord;
    ChunkCoord secondCoord = firstCoord;
    EnvironmentalConsequenceProfile firstProfile{};
    EnvironmentalConsequenceProfile secondProfile{};
    bool foundDifferentClimate = false;

    for(int radius=4; radius<=48 && !foundDifferentClimate; radius+=4){
        const std::array<ChunkCoord,8> candidates = {{
            {firstCoord.x+radius,firstCoord.y},
            {firstCoord.x-radius,firstCoord.y},
            {firstCoord.x,firstCoord.y+radius},
            {firstCoord.x,firstCoord.y-radius},
            {firstCoord.x+radius,firstCoord.y+radius},
            {firstCoord.x-radius,firstCoord.y+radius},
            {firstCoord.x+radius,firstCoord.y-radius},
            {firstCoord.x-radius,firstCoord.y-radius}
        }};
        firstProfile = deriveEnvironmentalConsequences(
            deriveDynamicEnvironment(identity,firstCoord,snapshot.world.minute+1));
        for(const ChunkCoord candidate : candidates){
            const EnvironmentalConsequenceProfile candidateProfile =
                deriveEnvironmentalConsequences(
                    deriveDynamicEnvironment(identity,candidate,snapshot.world.minute+1));
            const double delta =
                std::abs(firstProfile.perMinuteNeedsDelta.hunger-candidateProfile.perMinuteNeedsDelta.hunger)
                + std::abs(firstProfile.perMinuteNeedsDelta.thirst-candidateProfile.perMinuteNeedsDelta.thirst)
                + std::abs(firstProfile.perMinuteNeedsDelta.sleep-candidateProfile.perMinuteNeedsDelta.sleep)
                + std::abs(firstProfile.perMinuteNeedsDelta.hygiene-candidateProfile.perMinuteNeedsDelta.hygiene);
            if(delta > 1e-8){
                secondCoord = candidate;
                secondProfile = candidateProfile;
                foundDifferentClimate = true;
                break;
            }
        }
    }
    assert(foundDifferentClimate);

    snapshot.runtime[firstResident.id].pos = chunkOriginGrid(firstCoord);
    snapshot.runtime[secondResident.id].pos = chunkOriginGrid(secondCoord);

    std::string restoreError;
    assert(localPressure.restoreSnapshot(snapshot,&restoreError));
    assert(restoreError.empty());
    localPressure.step();

    const Character& firstAfter = localPressure.world().characters[0];
    const Character& secondAfter = localPressure.world().characters[1];
    const double postDelta =
        std::abs(firstAfter.needs.hunger-secondAfter.needs.hunger)
        + std::abs(firstAfter.needs.thirst-secondAfter.needs.thirst)
        + std::abs(firstAfter.needs.sleep-secondAfter.needs.sleep)
        + std::abs(firstAfter.needs.hygiene-secondAfter.needs.hygiene);
    assert(postDelta > 1e-8);

    return 0;
}
