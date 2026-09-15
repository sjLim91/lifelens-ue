#pragma once

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "EnvironmentalResidue.h"

namespace lifelens {

constexpr char EnvironmentalResidueSnapshotExtensionMagic[]={'L','L','E','N','V','0','0','1'};
constexpr std::uint32_t EnvironmentalResidueSnapshotExtensionVersion=1;

template<typename WriterT>
void writeEnvironmentalResidueRecord(WriterT& w,const EnvironmentalResidueRecord& record)
{
    w.u64(record.id);
    w.enumeration(record.kind);
    w.i32(record.pos.x);
    w.i32(record.pos.y);
    w.u64(record.sourceCharacter);
    w.i32(record.createdMinute);
    w.i32(record.lastUpdatedMinute);
    w.real(record.amount);
    w.real(record.intensity);
    w.i32(record.radiusTiles);
}

template<typename ReaderT>
bool readEnvironmentalResidueRecord(ReaderT& r,EnvironmentalResidueRecord& record)
{
    return r.u64(record.id)
        && r.enumeration(record.kind)
        && r.i32(record.pos.x)
        && r.i32(record.pos.y)
        && r.u64(record.sourceCharacter)
        && r.i32(record.createdMinute)
        && r.i32(record.lastUpdatedMinute)
        && r.real(record.amount)
        && r.real(record.intensity)
        && r.i32(record.radiusTiles)
        && record.kind==EnvironmentalResidueKind::HumanWaste
        && validEnvironmentalResidueRecord(record);
}

template<typename WriterT>
void writeEnvironmentalResidueSnapshotExtension(WriterT& w,const EnvironmentalResidueField& field)
{
    w.raw(EnvironmentalResidueSnapshotExtensionMagic,sizeof(EnvironmentalResidueSnapshotExtensionMagic));
    w.u32(EnvironmentalResidueSnapshotExtensionVersion);
    w.u32(static_cast<std::uint32_t>(field.all().size()));
    for(const auto& record:field.all()) writeEnvironmentalResidueRecord(w,record);
}

template<typename ReaderT>
bool readEnvironmentalResidueSnapshotExtension(ReaderT& r,EnvironmentalResidueField& field)
{
    char magic[sizeof(EnvironmentalResidueSnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    if(!r.raw(magic,sizeof(magic))
       || std::memcmp(magic,EnvironmentalResidueSnapshotExtensionMagic,sizeof(magic))!=0
       || !r.u32(version)
       || version!=EnvironmentalResidueSnapshotExtensionVersion) return false;

    std::uint32_t count=0;
    if(!r.count(count)) return false;
    std::vector<EnvironmentalResidueRecord> records;
    records.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        EnvironmentalResidueRecord record;
        if(!readEnvironmentalResidueRecord(r,record)) return false;
        records.push_back(std::move(record));
    }
    return field.restoreState(std::move(records));
}

} // namespace lifelens
