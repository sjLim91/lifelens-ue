#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Health.h"
#include "World.h"

namespace lifelens {

constexpr char HealthSnapshotExtensionMagic[]={'L','L','H','L'};
constexpr std::uint32_t HealthSnapshotExtensionVersion=1;

inline bool validHealthState(const HealthState& state)
{
    const auto valid01=[](double value){
        return std::isfinite(value) && value>=0.0 && value<=1.0;
    };
    return valid01(state.pathogenLoad)
        && valid01(state.illnessSeverity)
        && valid01(state.immunity01)
        && valid01(state.injurySeverity)
        && valid01(state.environmentalStress01)
        && valid01(state.careKnowledge01)
        && valid01(state.pendingWaterContaminationDose)
        && state.infectionEpisodes>=0
        && state.recoveryEpisodes>=0
        && state.accidentEpisodes>=0
        && state.lastExposureMinute>=-1
        && state.lastIllnessMinute>=-1
        && state.lastRecoveryMinute>=-1
        && state.lastAccidentMinute>=-1;
}

template<typename WriterT>
void writeHealthState(WriterT& w,const HealthState& state)
{
    w.real(state.pathogenLoad);
    w.real(state.illnessSeverity);
    w.real(state.immunity01);
    w.real(state.injurySeverity);
    w.real(state.environmentalStress01);
    w.real(state.careKnowledge01);
    w.real(state.pendingWaterContaminationDose);
    w.i32(state.infectionEpisodes);
    w.i32(state.recoveryEpisodes);
    w.i32(state.accidentEpisodes);
    w.i32(state.lastExposureMinute);
    w.i32(state.lastIllnessMinute);
    w.i32(state.lastRecoveryMinute);
    w.i32(state.lastAccidentMinute);
}

template<typename ReaderT>
bool readHealthState(ReaderT& r,HealthState& state)
{
    if(!r.real(state.pathogenLoad)
       || !r.real(state.illnessSeverity)
       || !r.real(state.immunity01)
       || !r.real(state.injurySeverity)
       || !r.real(state.environmentalStress01)
       || !r.real(state.careKnowledge01)
       || !r.real(state.pendingWaterContaminationDose)
       || !r.i32(state.infectionEpisodes)
       || !r.i32(state.recoveryEpisodes)
       || !r.i32(state.accidentEpisodes)
       || !r.i32(state.lastExposureMinute)
       || !r.i32(state.lastIllnessMinute)
       || !r.i32(state.lastRecoveryMinute)
       || !r.i32(state.lastAccidentMinute)){
        return false;
    }
    return validHealthState(state);
}

template<typename WriterT>
void writeHealthSnapshotExtension(WriterT& w,const World& world)
{
    w.raw(HealthSnapshotExtensionMagic,sizeof(HealthSnapshotExtensionMagic));
    w.u32(HealthSnapshotExtensionVersion);
    w.u32(static_cast<std::uint32_t>(world.characters.size()));
    for(const Character& character:world.characters){
        w.u64(character.id);
        writeHealthState(w,character.health);
    }
}

template<typename ReaderT>
bool readHealthSnapshotExtension(ReaderT& r,World& world)
{
    char magic[sizeof(HealthSnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    std::uint32_t count=0;
    if(!r.raw(magic,sizeof(magic))
       || !std::equal(
           magic,magic+sizeof(magic),HealthSnapshotExtensionMagic)
       || !r.u32(version)
       || version!=HealthSnapshotExtensionVersion
       || !r.count(count)
       || count!=world.characters.size()){
        return false;
    }

    std::vector<CharacterId> seen;
    seen.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        CharacterId id=0;
        HealthState state;
        if(!r.u64(id)
           || id==0
           || std::find(seen.begin(),seen.end(),id)!=seen.end()
           || !readHealthState(r,state)){
            return false;
        }
        auto it=std::find_if(
            world.characters.begin(),world.characters.end(),
            [id](const Character& character){return character.id==id;});
        if(it==world.characters.end()) return false;
        it->health=state;
        seen.push_back(id);
    }
    return true;
}

} // namespace lifelens
