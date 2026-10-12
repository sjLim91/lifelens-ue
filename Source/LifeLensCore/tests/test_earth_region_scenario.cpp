#include "lifelens/EarthRegionScenario.h"
#include <cassert>
#include <cmath>
#include <limits>

using namespace lifelens;
namespace {
bool near(double a,double b,double tolerance=2e-6)
{return std::abs(a-b)<tolerance;}
}

int main()
{
    constexpr WorldSeed seed=874213954ULL;
    const WorldGenesisIdentity identity=makeWorldGenesisIdentity(seed,111,2);
    World world(seed,111,2);
    EarthScenarioGeoCatalog registry(identity,"ne110m-land-v1","natural-earth");

    const SurfaceRegionCoord a={0,0};
    const SurfaceRegionCoord b={200,-70};
    auto first=makeEarthRegionScenarioBinding(
        identity,a,{35.1796,129.0756},{0,0},25.0,
        "ne110m-land-v1","natural-earth");
    auto second=makeEarthRegionScenarioBinding(
        identity,b,{-33.9249,18.4241},{200*32*32,-70*32*32},25.0,
        "ne110m-land-v1","natural-earth");

    // The engine has no geographical position unless the scenario explicitly
    // declares the region. Unknown parts of the world are *not* silently
    // projected to a synthetic Earth coordinate.
    EarthGeoPoint p{};
    assert(!registry.geographicPositionForGrid(world,{0,0},p));
    assert(validateEarthRegionScenarioBinding(identity,first)
           ==EarthScenarioBindingStatus::Accepted);
    assert(registry.add(first)==EarthScenarioBindingStatus::Accepted);
    assert(registry.add(second)==EarthScenarioBindingStatus::Accepted);
    assert(registry.bindings().size()==2);
    assert(registry.add(first)==EarthScenarioBindingStatus::DuplicateRegion);

    assert(registry.geographicPositionForGrid(world,{0,0},p));
    assert(near(p.latitudeDeg,35.1796));
    assert(near(p.longitudeDeg,129.0756));
    assert(registry.geographicPositionForGrid(
        world,{200*32*32,-70*32*32},p));
    assert(near(p.latitudeDeg,-33.9249));
    assert(near(p.longitudeDeg,18.4241));

    // Boundaries do not spill into adjacent regions. No fake globe markers.
    assert(!registry.geographicPositionForGrid(world,{-1,0},p));
    assert(!registry.geographicPositionForGrid(world,{32*32,0},p));

    // Same WorldSeed and WorldGenerationVersion, another population seed:
    // geographical truth still belongs to the planet, not the founder roster.
    World anotherPopulation(seed,999,2);
    assert(registry.geographicPositionForGrid(anotherPopulation,{0,0},p));
    // Another world, or a legacy WorldGenerationVersion, is not the same Earth.
    World otherSeed(seed+1,111,2);
    World otherVersion(seed,111,1);
    assert(!registry.geographicPositionForGrid(otherSeed,{0,0},p));
    assert(!registry.geographicPositionForGrid(otherVersion,{0,0},p));

    first.regionId+=1;
    assert(registry.add(first)
        ==EarthScenarioBindingStatus::InvalidRegionIdentity);
    first.regionId-=1;
    first.localGridOrigin={-5,0};
    assert(registry.add(first)
        ==EarthScenarioBindingStatus::InvalidGeoAnchor);
    first.localGridOrigin={0,0};
    first.geographicOrigin.latitudeDeg=95.0;
    assert(registry.add(first)
        ==EarthScenarioBindingStatus::InvalidGeoAnchor);
    first.geographicOrigin.latitudeDeg=35.1796;
    first.earthDataVersion="unapproved-set";
    assert(registry.add(first)
        ==EarthScenarioBindingStatus::InvalidEarthDataVersion);
    first.earthDataVersion="ne110m-land-v1";
    first.metersPerGridCell=0.0;
    assert(registry.add(first)
        ==EarthScenarioBindingStatus::InvalidGeoAnchor);
    first.metersPerGridCell=25.0;

    // One source crossing ±180 is legal; its coordinates stay spherical.
    auto seam=makeEarthRegionScenarioBinding(
        identity,{3,3},{0.0,179.999},{3*32*32,3*32*32},25.0,
        "ne110m-land-v1","natural-earth");
    assert(registry.add(seam)==EarthScenarioBindingStatus::Accepted);
    assert(registry.geographicPositionForGrid(
        world,{3*32*32+45,3*32*32},p));
    assert(p.longitudeDeg<0);
    assert(earthValidGeo(p));

    // No RNG draws or mutation of world runtime state during geographic reads.
    const auto initialRng=world.rng;
    const int initialMinute=world.minute;
    assert(registry.geographicPositionForGrid(world,{0,0},p));
    assert(world.minute==initialMinute);
    assert(world.rng==initialRng);
    return 0;
}
