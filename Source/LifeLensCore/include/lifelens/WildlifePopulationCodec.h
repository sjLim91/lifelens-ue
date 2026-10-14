#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include "WildlifeEcology.h"

namespace lifelens {

// ECO-A2: opt-in wildlife-only binary state, separate from existing Core
// World/SimulationSnapshot. No automatic world tick, inventory or AI changes.
inline constexpr std::uint32_t WildlifeBinaryFormatVersion=2;
inline constexpr std::uint32_t WildlifeMaxPersistedAnimals=1024;
inline constexpr std::size_t WildlifeMaxBinaryBytes=65536;

inline bool validWildlifePopulation(
    const WildlifePopulation& state,std::string* error=nullptr)
{
    const auto fail=[&](const char* why){
        if(error) *error=why;
        return false;
    };
    const auto& habitat=state.habitat;
    const auto valid01=[](double v){
        return std::isfinite(v) && v>=0.0 && v<=1.0;
    };
    if(state.worldSeed==0 || state.generationVersion==0
       ||state.dayIndex<0 ||state.nextSerial==0)
        return fail("invalid world or time identity");
    if(habitat.geographicallyConditioned || habitat.regionalWeatherApplied)
        return fail("habitat provenance is not real-Earth conditioned");
    if(!valid01(habitat.forage01)
       ||!valid01(habitat.shelter01)
       ||!valid01(habitat.moisture01))
        return fail("invalid habitat potential");
    // Guarantee chunk origin and upper cell bounds fit signed GridPos.
    constexpr int MaxChunk=std::numeric_limits<int>::max()/WildlifeCellsPerChunk-1;
    constexpr int MinChunk=std::numeric_limits<int>::min()/WildlifeCellsPerChunk+1;
    if(habitat.coord.x<MinChunk ||habitat.coord.x>MaxChunk
       ||habitat.coord.y<MinChunk ||habitat.coord.y>MaxChunk)
        return fail("chunk coordinate overflow");
    for(std::size_t s=0;s<WildlifeSpeciesCount;++s){
        if(habitat.carryingCapacity[s]<0 ||habitat.carryingCapacity[s]>256)
            return fail("invalid species capacity");
    }
    if(!habitat.freshwaterAvailable
       &&habitat.carryingCapacity[
         wildlifeSpeciesIndex(WildlifeSpecies::FreshwaterFish)]!=0)
        return fail("freshwater fish without water");
    if(state.animals.size()>WildlifeMaxPersistedAnimals
       ||(!habitat.available && !state.animals.empty()))
        return fail("invalid animal count");
    std::array<int,WildlifeSpeciesCount> liveCounts{};
    std::unordered_set<std::uint64_t> ids;
    const GridPos start=chunkOriginGrid(habitat.coord);
    const std::uint64_t prefix=wildlifeHash(
        state.worldSeed,habitat.coord,0,0,0x4944454e54495459ULL)
        &0xffffffff00000000ULL;
    for(const auto& animal:state.animals){
        const std::size_t species=static_cast<std::size_t>(animal.species);
        if(species>=WildlifeSpeciesCount
           ||static_cast<std::uint8_t>(animal.sex)>1
           ||!animal.alive)
            return fail("invalid species, sex or lifecycle");
        if(++liveCounts[species]>habitat.carryingCapacity[species])
            return fail("population exceeds carrying capacity");
        if(!ids.insert(animal.id).second
           ||(animal.id&0xffffffff00000000ULL)!=prefix
           ||(animal.id&0xffffffffULL)==0
           ||(animal.id&0xffffffffULL)>=state.nextSerial)
            return fail("invalid or duplicate animal identity");
        if(animal.ageDays<0
           ||animal.ageDays>wildlifeMaximumAgeDays(animal.species)
           ||!valid01(animal.hunger01) ||animal.hunger01>=1.0)
            return fail("invalid age/hunger");
        if(animal.position.x<start.x
           ||animal.position.x-start.x>=WildlifeCellsPerChunk
           ||animal.position.y<start.y
           ||animal.position.y-start.y>=WildlifeCellsPerChunk)
            return fail("animal outside owned chunk");
        if(animal.species==WildlifeSpecies::FreshwaterFish
           &&(!habitat.freshwaterAvailable
              ||animal.position.x!=start.x+WildlifeCellsPerChunk/2
              ||animal.position.y!=start.y+WildlifeCellsPerChunk/2))
            return fail("fish outside freshwater center");
    }
    return true;
}

namespace wildlife_binary_detail {

inline std::uint64_t checksum(
    const std::vector<std::uint8_t>& bytes,std::size_t size)
{
    std::uint64_t result=14695981039346656037ULL;
    for(std::size_t i=0;i<size;++i){
        result^=bytes[i];
        result*=1099511628211ULL;
    }
    return result;
}

struct Writer {
    std::vector<std::uint8_t> data;
    void byte(std::uint8_t value){data.push_back(value);}
    void u32(std::uint32_t value){
        for(int i=0;i<4;++i) byte(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void u64(std::uint64_t value){
        for(int i=0;i<8;++i) byte(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void i32(int value){u32(static_cast<std::uint32_t>(value));}
    void f64(double value){
        static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559,
            "IEEE-754 doubles required");
        std::uint64_t bits=0;
        std::memcpy(&bits,&value,sizeof(bits));
        u64(bits);
    }
};

struct Reader {
    const std::vector<std::uint8_t>& data;
    std::size_t offset=0;
    bool ok=true;
    std::uint8_t byte(){
        if(offset>=data.size()){ok=false;return 0;}
        return data[offset++];
    }
    std::uint32_t u32(){
        std::uint32_t v=0;
        for(int i=0;i<4;++i) v|=static_cast<std::uint32_t>(byte())<<(i*8);
        return v;
    }
    std::uint64_t u64(){
        std::uint64_t v=0;
        for(int i=0;i<8;++i) v|=static_cast<std::uint64_t>(byte())<<(i*8);
        return v;
    }
    int i32(){
        const std::uint32_t bits=u32();
        // Avoid implementation-defined large unsigned-to-signed conversion.
        const std::int64_t signedValue=bits<=0x7fffffffU
            ?static_cast<std::int64_t>(bits)
            :static_cast<std::int64_t>(bits)-0x100000000LL;
        return static_cast<int>(signedValue);
    }
    double f64(){
        const std::uint64_t bits=u64();
        double v=0;
        std::memcpy(&v,&bits,sizeof(v));
        return v;
    }
};
} // namespace wildlife_binary_detail

inline bool encodeWildlifePopulation(
    const WildlifePopulation& state,
    std::vector<std::uint8_t>& bytes,std::string* error=nullptr)
{
    if(!validWildlifePopulation(state,error)) return false;
    wildlife_binary_detail::Writer writer;
    for(std::uint8_t c:{'L','L','W','F'}) writer.byte(c);
    writer.u32(WildlifeBinaryFormatVersion);
    writer.u64(state.worldSeed);
    writer.u32(state.generationVersion);
    writer.u64(static_cast<std::uint64_t>(state.dayIndex));
    writer.u32(state.nextSerial);
    const auto& h=state.habitat;
    writer.i32(h.coord.x);
    writer.i32(h.coord.y);
    writer.byte(h.available?1:0);
    writer.byte(h.freshwaterAvailable?1:0);
    writer.byte(h.geographicallyConditioned?1:0);
    writer.byte(h.regionalWeatherApplied?1:0);
    writer.f64(h.forage01);
    writer.f64(h.shelter01);
    writer.f64(h.moisture01);
    for(int cap:h.carryingCapacity) writer.u32(static_cast<std::uint32_t>(cap));
    writer.u32(static_cast<std::uint32_t>(state.animals.size()));
    for(const auto& animal:state.animals){
        writer.u64(animal.id);
        writer.byte(static_cast<std::uint8_t>(animal.species));
        writer.byte(static_cast<std::uint8_t>(animal.sex));
        writer.i32(animal.position.x);
        writer.i32(animal.position.y);
        writer.i32(animal.ageDays);
        writer.f64(animal.hunger01);
        writer.byte(animal.alive?1:0);
    }
    writer.u64(wildlife_binary_detail::checksum(
        writer.data,writer.data.size()));
    if(writer.data.size()>WildlifeMaxBinaryBytes){
        if(error) *error="wildlife binary exceeds size limit";
        return false;
    }
    bytes=std::move(writer.data);
    return true;
}

inline bool decodeWildlifePopulation(
    const std::vector<std::uint8_t>& bytes,WildlifePopulation& output,
    std::string* error=nullptr)
{
    const auto fail=[&](const char* why){
        if(error) *error=why;
        return false;
    };
    if(bytes.size()<8+8 ||bytes.size()>WildlifeMaxBinaryBytes)
        return fail("wildlife binary size");
    wildlife_binary_detail::Reader reader{bytes};
    if(reader.byte()!='L'||reader.byte()!='L'
       ||reader.byte()!='W'||reader.byte()!='F')
        return fail("invalid wildlife magic");
    const std::uint32_t version=reader.u32();
    if(version!=1 && version!=WildlifeBinaryFormatVersion)
        return fail("invalid wildlife magic/version");
    WildlifePopulation result;
    result.worldSeed=reader.u64();
    result.generationVersion=reader.u32();
    const std::uint64_t day=reader.u64();
    if(day>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return fail("wildlife day out of range");
    result.dayIndex=static_cast<std::int64_t>(day);
    result.nextSerial=reader.u32();
    auto& habitat=result.habitat;
    habitat.coord.x=reader.i32();
    habitat.coord.y=reader.i32();
    const auto yesNo=[&reader](){
        const auto v=reader.byte();
        if(v>1) reader.ok=false;
        return v==1;
    };
    habitat.available=yesNo();
    habitat.freshwaterAvailable=yesNo();
    habitat.geographicallyConditioned=yesNo();
    habitat.regionalWeatherApplied=yesNo();
    habitat.forage01=reader.f64();
    habitat.shelter01=reader.f64();
    habitat.moisture01=reader.f64();
    // v1 stored exactly four capacities; v2 appends Tiger without shifting
    // any existing species ordinal. Old populations never spawn a new species.
    for(std::size_t i=0;i<(version==1?4U:WildlifeSpeciesCount);++i){
        const std::uint32_t value=reader.u32();
        if(value>256) return fail("invalid wildlife capacity");
        habitat.carryingCapacity[i]=static_cast<int>(value);
    }
    const std::uint32_t count=reader.u32();
    if(!reader.ok ||count>WildlifeMaxPersistedAnimals)
        return fail("invalid animal length");
    // Strict record length check before allocating anything based on input.
    constexpr std::size_t AnimalBytes=8+1+1+4+4+4+8+1;
    if(bytes.size()-reader.offset!=
        static_cast<std::size_t>(count)*AnimalBytes+8)
        return fail("wildlife truncated or trailing data");
    result.animals.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        WildlifeAnimal a;
        a.id=reader.u64();
        a.species=static_cast<WildlifeSpecies>(reader.byte());
        a.sex=static_cast<WildlifeSex>(reader.byte());
        a.position.x=reader.i32();
        a.position.y=reader.i32();
        a.ageDays=reader.i32();
        a.hunger01=reader.f64();
        a.alive=yesNo();
        result.animals.push_back(a);
    }
    const std::uint64_t stored=reader.u64();
    if(!reader.ok ||reader.offset!=bytes.size()
       ||stored!=wildlife_binary_detail::checksum(bytes,bytes.size()-8))
        return fail("invalid wildlife checksum");
    if(!validWildlifePopulation(result,error)) return false;
    // Atomic: invalid input never mutates the previous valid state.
    output=std::move(result);
    return true;
}

} // namespace lifelens
