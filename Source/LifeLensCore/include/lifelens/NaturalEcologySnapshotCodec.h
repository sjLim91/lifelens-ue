#pragma once
#include "World.h"
#include "WildlifePopulationCodec.h"
#include <unordered_set>
namespace lifelens {
inline constexpr char NaturalEcologyMagic[]={'L','L','E','C','O','L','0','1'};
template<class W> void writeNaturalEcology(W& w,const World& world)
{
    w.raw(NaturalEcologyMagic,sizeof(NaturalEcologyMagic)); w.u32(world.wildlifeEnabled ? 2 : 1);
    w.u32(static_cast<std::uint32_t>(world.resourceNodes.size()));
    for(const auto& n:world.resourceNodes){
        w.u64(n.id); w.i32(n.ecologyMinute); w.real(n.flower01); w.real(n.fruit01);
        w.u32(static_cast<std::uint32_t>(n.trees.size()));
        for(const auto& t:n.trees){w.i32(t.ordinal); w.i32(t.bornMinute); w.i32(t.remainingWood); w.i32(t.capacity); w.i32(t.felledMinute); w.i32(t.generation);}
    }
    w.u32(static_cast<std::uint32_t>(world.snowpacks.size()));
    for(const auto& p:world.snowpacks){w.i32(p.coord.x); w.i32(p.coord.y); w.real(p.depthMeters); w.i32(p.minute);}
    if(world.wildlifeEnabled){
        w.u32(static_cast<std::uint32_t>(world.wildlifePopulations.size()));
        for(const auto& p:world.wildlifePopulations){
            std::vector<std::uint8_t> bytes; encodeWildlifePopulation(p,bytes);
            w.u32(static_cast<std::uint32_t>(bytes.size()));
            w.raw(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        }
    }
}
template<class R> bool readNaturalEcology(R& r,World& world)
{
    char magic[8]{}; std::uint32_t version=0,count=0;
    if(!r.raw(magic,8) || !std::equal(magic,magic+8,NaturalEcologyMagic) || !r.u32(version) || (version!=1 && version!=2) || !r.count(count) || count!=world.resourceNodes.size()) return false;
    std::unordered_set<ResourceNodeId> ids;
    for(std::uint32_t i=0;i<count;++i){
        ResourceNodeId id=0; if(!r.u64(id) || !ids.insert(id).second) return false;
        auto it=std::find_if(world.resourceNodes.begin(),world.resourceNodes.end(),[&](const ResourceNode& n){return n.id==id;});
        if(it==world.resourceNodes.end()) return false;
        auto& n=*it; std::uint32_t trees=0;
        if(!r.i32(n.ecologyMinute) || n.ecologyMinute<0 || n.ecologyMinute>world.minute || !r.real(n.flower01) || !r.real(n.fruit01) || !std::isfinite(n.flower01) || !std::isfinite(n.fruit01) || n.flower01<0 || n.flower01>1 || n.fruit01<0 || n.fruit01>1 || !r.count(trees) || (trees!=0 && (trees!=8 || n.material!=MaterialKind::Wood))) return false;
        n.trees.clear(); std::int64_t stock=0,capacity=0;
        for(std::uint32_t j=0;j<trees;++j){
            NaturalTreeState t;
            if(!r.i32(t.ordinal) || !r.i32(t.bornMinute) || !r.i32(t.remainingWood) || !r.i32(t.capacity) || !r.i32(t.felledMinute) || !r.i32(t.generation) || t.ordinal!=static_cast<int>(j) || t.bornMinute>world.minute || t.remainingWood<0 || t.capacity<0 || t.remainingWood>t.capacity || t.felledMinute<-1 || t.felledMinute>world.minute || t.generation<0) return false;
            stock+=t.remainingWood; capacity+=t.capacity; n.trees.push_back(t);
        }
        if(trees && (stock!=n.quantity || capacity!=n.maxQuantity)) return false;
    }
    if(!r.count(count) || count>world.generatedNaturalChunks.size()) return false;
    world.snowpacks.clear();
    for(std::uint32_t i=0;i<count;++i){
        SnowpackState p;
        if(!r.i32(p.coord.x) || !r.i32(p.coord.y) || !r.real(p.depthMeters) || !r.i32(p.minute) || !std::isfinite(p.depthMeters) || p.depthMeters<0 || p.depthMeters>2 || p.minute<0 || p.minute>world.minute || !world.findGeneratedNaturalChunk(p.coord) || (!world.snowpacks.empty() && !(world.snowpacks.back().coord<p.coord))) return false;
        world.snowpacks.push_back(p);
    }
    world.wildlifeEnabled=version==2;
    world.wildlifePopulations.clear();
    if(version==2){
        if(!r.count(count) || count>64) return false;
        for(std::uint32_t i=0;i<count;++i){
            std::uint32_t size=0;
            if(!r.count(size) || size>WildlifeMaxBinaryBytes) return false;
            std::vector<std::uint8_t> bytes(size);
            WildlifePopulation p;
            if(!r.raw(reinterpret_cast<char*>(bytes.data()),bytes.size())
               || !decodeWildlifePopulation(bytes,p) || p.worldSeed!=world.seed
               || p.generationVersion!=world.generationVersion || p.dayIndex>world.minute/(24*60)
               || !world.findGeneratedNaturalChunk(p.habitat.coord)
               || (!world.wildlifePopulations.empty() && !(world.wildlifePopulations.back().habitat.coord<p.habitat.coord))) return false;
            world.wildlifePopulations.push_back(std::move(p));
        }
    }
    return true;
}
}

