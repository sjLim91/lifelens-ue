#pragma once
#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include "EarthSurfaceCoordinates.h"
#include "World.h"

namespace lifelens {

// EarthData identity must be bound by an explicit scenario/data-pack.
// Never infer a real-Earth position for an existing procedural Core region.
// A binding does not add an origin resident, alter World, or mutate RNG.
struct EarthRegionScenarioBinding {
    WorldSeed worldSeed=0;
    WorldGenerationVersion worldGenerationVersion=0;
    std::string earthDataVersion;
    std::string geographySource;
    SurfaceRegionCoord region{};
    SurfaceRegionId regionId=0;
    EarthGeoPoint geographicOrigin{};
    GridPos localGridOrigin{};
    double metersPerGridCell=0.0;
};

enum class EarthScenarioBindingStatus {
    Accepted,
    InvalidWorld,
    InvalidEarthDataVersion,
    InvalidGeoAnchor,
    InvalidRegionIdentity,
    DuplicateRegion
};

inline bool earthValidVersionToken(const std::string& value)
{
    if(value.empty()||value.size()>128) return false;
    return std::all_of(value.begin(),value.end(),[](unsigned char c){
        return (c>='0'&&c<='9')||(c>='A'&&c<='Z')
            ||(c>='a'&&c<='z')||c=='_'||c=='-'||c=='.'||c==':';
    });
}

inline bool earthSameWorldTruth(
    const WorldGenesisIdentity& active,
    const EarthRegionScenarioBinding& binding)
{
    return binding.worldSeed==active.worldSeed
        &&binding.worldGenerationVersion==active.generationVersion;
}

inline EarthScenarioBindingStatus validateEarthRegionScenarioBinding(
    const WorldGenesisIdentity& identity,
    const EarthRegionScenarioBinding& binding)
{
    if(!earthSameWorldTruth(identity,binding))
        return EarthScenarioBindingStatus::InvalidWorld;
    if(!earthValidVersionToken(binding.earthDataVersion)
       ||!earthValidVersionToken(binding.geographySource))
        return EarthScenarioBindingStatus::InvalidEarthDataVersion;
    const SurfaceRegionIdentity derived=
        deriveSurfaceRegionIdentity(identity,binding.region);
    if(derived.id!=binding.regionId||binding.regionId==0)
        return EarthScenarioBindingStatus::InvalidRegionIdentity;

    const EarthRegionGeoAnchor geo={
        binding.regionId,binding.geographicOrigin,
        binding.localGridOrigin,binding.metersPerGridCell
    };
    if(!earthValidGeoAnchor(geo))
        return EarthScenarioBindingStatus::InvalidGeoAnchor;
    // Anchor must actually be inside the declared Core surface region. An
    // arbitrary remote grid origin can mislabel an unrelated settlement.
    const ChunkCoord chunk=chunkCoordForGrid(binding.localGridOrigin);
    if(surfaceRegionCoordForChunk(chunk)!=binding.region)
        return EarthScenarioBindingStatus::InvalidGeoAnchor;
    return EarthScenarioBindingStatus::Accepted;
}

// The entire catalog refers to a single version-pinned natural Earth pack.
// Spatially separate origins can have different coordinates, but no duplicate
// Core RegionId or invented inferred placement is permitted.
class EarthScenarioGeoCatalog {
public:
    EarthScenarioGeoCatalog(
        WorldGenesisIdentity identity,
        std::string earthDataVersion,
        std::string geographySource)
        : identity_(identity),
          dataVersion_(std::move(earthDataVersion)),
          source_(std::move(geographySource)) {}

    EarthScenarioBindingStatus add(EarthRegionScenarioBinding binding)
    {
        if(binding.earthDataVersion!=dataVersion_
           ||binding.geographySource!=source_)
            return EarthScenarioBindingStatus::InvalidEarthDataVersion;
        const auto status=validateEarthRegionScenarioBinding(identity_,binding);
        if(status!=EarthScenarioBindingStatus::Accepted) return status;
        if(std::any_of(bindings_.begin(),bindings_.end(),
            [&](const EarthRegionScenarioBinding& old){
                return old.regionId==binding.regionId
                    ||old.region==binding.region;
            }))
            return EarthScenarioBindingStatus::DuplicateRegion;
        const auto pos=std::lower_bound(
            bindings_.begin(),bindings_.end(),binding.regionId,
            [](const EarthRegionScenarioBinding& b,SurfaceRegionId id){
                return b.regionId<id;
            });
        bindings_.insert(pos,std::move(binding));
        return EarthScenarioBindingStatus::Accepted;
    }

    const EarthRegionScenarioBinding* forRegion(
        const World& world,SurfaceRegionId regionId) const
    {
        // Population seed is intentionally not geographic authority.
        // A different world seed/version MUST NOT inherit prior marker data.
        if(world.seed!=identity_.worldSeed
           ||world.generationVersion!=identity_.generationVersion)
            return nullptr;
        const auto it=std::lower_bound(
            bindings_.begin(),bindings_.end(),regionId,
            [](const EarthRegionScenarioBinding& b,SurfaceRegionId id){
                return b.regionId<id;
            });
        return it!=bindings_.end()&&it->regionId==regionId?&*it:nullptr;
    }

    bool geographicPositionForGrid(
        const World& world,GridPos grid,EarthGeoPoint& point) const
    {
        const SurfaceRegionId id=deriveSurfaceRegionIdentityForChunk(
            world.genesisIdentity(),chunkCoordForGrid(grid)).id;
        const auto* binding=forRegion(world,id);
        if(!binding) return false;
        const EarthRegionGeoAnchor geo={
            binding->regionId,binding->geographicOrigin,
            binding->localGridOrigin,binding->metersPerGridCell
        };
        return earthGeoForGrid(geo,grid,point);
    }

    const std::vector<EarthRegionScenarioBinding>& bindings() const
    {return bindings_;}
    const std::string& earthDataVersion() const{return dataVersion_;}

private:
    WorldGenesisIdentity identity_{};
    std::string dataVersion_;
    std::string source_;
    std::vector<EarthRegionScenarioBinding> bindings_;
};

inline EarthRegionScenarioBinding makeEarthRegionScenarioBinding(
    WorldGenesisIdentity identity,SurfaceRegionCoord coord,
    EarthGeoPoint geographicOrigin,GridPos gridOrigin,
    double metersPerGridCell,const std::string& packVersion,
    const std::string& geographySource)
{
    const auto derived=deriveSurfaceRegionIdentity(identity,coord);
    return {
        identity.worldSeed,identity.generationVersion,
        packVersion,geographySource,coord,derived.id,
        geographicOrigin,gridOrigin,metersPerGridCell
    };
}

} // namespace lifelens
