#include "lifelens/EarthNaturalLandMask.h"
#include "lifelens/EarthRegionScenario.h"
#include <cassert>
#include <cstring>
#include <limits>

using namespace lifelens;

int main()
{
    static_assert(EarthLandMaskLongitudeCells==360);
    static_assert(EarthLandMaskLatitudeCells==180);
    static_assert(sizeof(EarthLandMaskHex)==16201);
    assert(std::strcmp(EarthLandMaskUpstreamCommit,
        "693f11422f4e08d2da4566b854dda53eb7c39fb3")==0);

    // Known coarse inland cells on several continents: not invented random land.
    assert(earthNaturalLandClassAt({40.0,-100.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({48.0,2.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({37.0,127.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({-34.0,18.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({30.0,30.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({20.0,100.0})
           ==EarthNaturalLandClass::Land);

    // Open-ocean cells. Land ≠ freshwater: neither terrain nor drinking
    // resources are inferred from this reference layer.
    assert(earthNaturalLandClassAt({0.0,-30.0})
           ==EarthNaturalLandClass::Ocean);
    assert(earthNaturalLandClassAt({0.0,150.0})
           ==EarthNaturalLandClass::Ocean);
    assert(earthNaturalLandClassAt({0.0,0.0})
           ==EarthNaturalLandClass::Ocean);

    // The Antarctic polygon winds completely around the globe: it must
    // include the South Pole, not omit the continent at the antimeridian.
    assert(earthNaturalLandClassAt({-90.0,0.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({-78.0,0.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({-80.0,170.0})
           ==EarthNaturalLandClass::Land);
    assert(earthNaturalLandClassAt({90.0,0.0})
           ==EarthNaturalLandClass::Ocean);

    // Seam is periodic; the exact longitude +180 is canonicalized to -180.
    for(int latitude=-90;latitude<=90;latitude+=10){
        assert(earthNaturalLandClassAt({static_cast<double>(latitude),180.0})
               ==earthNaturalLandClassAt(
                   {static_cast<double>(latitude),-180.0}));
    }

    assert(earthNaturalLandClassAt({91.0,0.0})
           ==EarthNaturalLandClass::Unknown);
    assert(earthNaturalLandClassAt({0.0,181.0})
           ==EarthNaturalLandClass::Unknown);
    assert(earthNaturalLandClassAt(
        {std::numeric_limits<double>::quiet_NaN(),0.0})
        ==EarthNaturalLandClass::Unknown);
    assert(earthNaturalLandClassAt(
        {std::numeric_limits<double>::infinity(),0.0})
        ==EarthNaturalLandClass::Unknown);

    // The current LifeLens procedural world must remain unchanged when the
    // offline Earth reference layer is queried, even if an opt-in catalog
    // specifies an Earth site for a future scenario.
    const WorldGenesisIdentity identity=makeWorldGenesisIdentity(4242001,123,2);
    World world(identity.worldSeed,identity.populationSeed,2);
    const auto rng=world.rng;
    const auto minute=world.minute;
    const auto beforeChunks=world.generatedNaturalChunks.size();
    const auto region=deriveSurfaceRegionIdentity(identity,{0,0});
    EarthScenarioGeoCatalog catalog(identity,EarthLandMaskVersion,"natural-earth");
    const auto binding=makeEarthRegionScenarioBinding(
        identity,{0,0},{37.0,127.0},{0,0},25.0,
        EarthLandMaskVersion,"natural-earth");
    assert(binding.regionId==region.id);
    assert(catalog.add(binding)==EarthScenarioBindingStatus::Accepted);
    EarthGeoPoint p{};
    assert(catalog.geographicPositionForGrid(world,{0,0},p));
    assert(earthNaturalLandClassAt(p)==EarthNaturalLandClass::Land);
    assert(world.minute==minute);
    assert(world.rng==rng);
    assert(world.generatedNaturalChunks.size()==beforeChunks);
    return 0;
}
