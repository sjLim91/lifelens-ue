#pragma once

#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <utility>
#include <vector>

#include "PrimitiveSanitation.h"

namespace lifelens {

constexpr char PrimitiveSanitationSnapshotExtensionMagic[]={'L','L','S','A','N','0','0','1'};
constexpr std::uint32_t PrimitiveSanitationSnapshotExtensionVersion=1;

template<typename WriterT>
void writePrimitiveSanitationSite(WriterT& w,const PrimitiveSanitationSite& site)
{
    w.u64(site.id);
    w.enumeration(site.kind);
    w.i32(site.pos.x);
    w.i32(site.pos.y);
    w.u64(site.establishedBy);
    w.i32(site.establishedMinute);
    w.boolean(site.active);
    w.i32(site.useCount);
}

template<typename ReaderT>
bool readPrimitiveSanitationSite(ReaderT& r,PrimitiveSanitationSite& site)
{
    return r.u64(site.id)
        && r.enumeration(site.kind)
        && r.i32(site.pos.x)
        && r.i32(site.pos.y)
        && r.u64(site.establishedBy)
        && r.i32(site.establishedMinute)
        && r.boolean(site.active)
        && r.i32(site.useCount)
        && validPrimitiveSanitationSite(site);
}

inline bool restorePrimitiveSanitationSites(
    std::vector<PrimitiveSanitationSite>& target,
    std::vector<PrimitiveSanitationSite> restored)
{
    std::unordered_set<SanitationSiteId> ids;
    for(const auto& site:restored){
        if(!validPrimitiveSanitationSite(site) || !ids.insert(site.id).second) return false;
    }
    target=std::move(restored);
    return true;
}

template<typename WriterT>
void writePrimitiveSanitationSnapshotExtension(
    WriterT& w,
    const std::vector<PrimitiveSanitationSite>& sites)
{
    w.raw(
        PrimitiveSanitationSnapshotExtensionMagic,
        sizeof(PrimitiveSanitationSnapshotExtensionMagic));
    w.u32(PrimitiveSanitationSnapshotExtensionVersion);
    w.u32(static_cast<std::uint32_t>(sites.size()));
    for(const auto& site:sites) writePrimitiveSanitationSite(w,site);
}

template<typename ReaderT>
bool readPrimitiveSanitationSnapshotExtension(
    ReaderT& r,
    std::vector<PrimitiveSanitationSite>& sites)
{
    char magic[sizeof(PrimitiveSanitationSnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    if(!r.raw(magic,sizeof(magic))
       || std::memcmp(
           magic,PrimitiveSanitationSnapshotExtensionMagic,sizeof(magic))!=0
       || !r.u32(version)
       || version!=PrimitiveSanitationSnapshotExtensionVersion) return false;

    std::uint32_t count=0;
    if(!r.count(count)) return false;
    std::vector<PrimitiveSanitationSite> restored;
    restored.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        PrimitiveSanitationSite site;
        if(!readPrimitiveSanitationSite(r,site)) return false;
        restored.push_back(std::move(site));
    }
    return restorePrimitiveSanitationSites(sites,std::move(restored));
}

} // namespace lifelens
