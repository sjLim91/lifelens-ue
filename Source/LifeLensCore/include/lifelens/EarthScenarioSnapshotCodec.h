#pragma once

#include <algorithm>
#include <cstdint>
#include "World.h"

namespace lifelens {
inline constexpr char EarthScenarioSnapshotMagic[]=
    {'L','L','E','A','R','T','0','1'};
inline constexpr std::uint32_t EarthScenarioSnapshotVersion=1;

template<class W>
void writeEarthScenarioSnapshot(W& w,const World& world)
{
    const auto& s=world.earthScenario;
    w.raw(EarthScenarioSnapshotMagic,sizeof(EarthScenarioSnapshotMagic));
    w.u32(EarthScenarioSnapshotVersion);
    w.u64(s.worldSeed);
    w.u32(s.generationVersion);
    w.real(s.geographicOrigin.latitudeDeg);
    w.real(s.geographicOrigin.longitudeDeg);
    w.i32(s.localGridOrigin.x);
    w.i32(s.localGridOrigin.y);
    w.real(s.metersPerGridCell);
    w.string(s.earthDataVersion);
    w.string(s.geographySource);
}

template<class R>
bool readEarthScenarioSnapshot(R& r,World& world)
{
    char magic[sizeof(EarthScenarioSnapshotMagic)]{};
    std::uint32_t version=0;
    EarthScenarioState s;
    s.available=true;
    if(!r.raw(magic,sizeof(magic))
       ||!std::equal(magic,magic+sizeof(magic),EarthScenarioSnapshotMagic)
       ||!r.u32(version)||version!=EarthScenarioSnapshotVersion
       ||!r.u64(s.worldSeed)||!r.u32(s.generationVersion)
       ||!r.real(s.geographicOrigin.latitudeDeg)
       ||!r.real(s.geographicOrigin.longitudeDeg)
       ||!r.i32(s.localGridOrigin.x)||!r.i32(s.localGridOrigin.y)
       ||!r.real(s.metersPerGridCell)
       ||!r.string(s.earthDataVersion)||!r.string(s.geographySource)
       ||!validEarthScenarioState(s,world.genesisIdentity())) return false;
    world.earthScenario=std::move(s);
    return true;
}
} // namespace lifelens
