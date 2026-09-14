#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Ids.h"
#include "SmartObject.h"

namespace lifelens {

using EnvironmentalResidueId = std::uint64_t;

enum class EnvironmentalResidueKind {
    HumanWaste
};

struct EnvironmentalResidueRecord {
    EnvironmentalResidueId id=0;
    EnvironmentalResidueKind kind=EnvironmentalResidueKind::HumanWaste;
    GridPos pos{};
    CharacterId sourceCharacter=0;
    int createdMinute=0;
    int lastUpdatedMinute=0;
    double amount=0.0;
    double intensity=0.0;
    int radiusTiles=1;
};

inline bool validEnvironmentalResidueRecord(const EnvironmentalResidueRecord& record)
{
    return record.id!=0
        && record.sourceCharacter!=0
        && record.createdMinute>=0
        && record.lastUpdatedMinute>=record.createdMinute
        && record.amount>0.0 && record.amount<=100.0
        && record.intensity>0.0 && record.intensity<=1.0
        && record.radiusTiles>=1 && record.radiusTiles<=16;
}

inline std::uint64_t environmentalMix(std::uint64_t value)
{
    value+=0x9e3779b97f4a7c15ULL;
    value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
    value=(value^(value>>27))*0x94d049bb133111ebULL;
    return value^(value>>31);
}

inline GridPos deterministicOutdoorReliefPosition(
    std::uint64_t worldSeed,
    CharacterId character,
    GridPos settlementOrigin={})
{
    const std::uint64_t mixed=environmentalMix((worldSeed?worldSeed:1)^environmentalMix(character));
    static const GridPos directions[8]={
        {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
    };
    const GridPos direction=directions[mixed%8ULL];
    const int distance=5+static_cast<int>((mixed>>8)%3ULL);
    return {
        settlementOrigin.x+direction.x*distance,
        settlementOrigin.y+direction.y*distance
    };
}

class EnvironmentalResidueField {
public:
    const std::vector<EnvironmentalResidueRecord>& all() const { return records_; }

    void clear() { records_.clear(); }

    bool restoreState(std::vector<EnvironmentalResidueRecord> records)
    {
        std::unordered_set<EnvironmentalResidueId> ids;
        for(const auto& record:records){
            if(!validEnvironmentalResidueRecord(record)) return false;
            if(!ids.insert(record.id).second) return false;
        }
        records_=std::move(records);
        return true;
    }

    void advanceToMinute(int minute)
    {
        if(minute<0) return;
        for(auto& record:records_){
            if(minute<=record.lastUpdatedMinute) continue;
            const int elapsed=minute-record.lastUpdatedMinute;
            record.amount=std::max(0.0,record.amount-static_cast<double>(elapsed)*0.00035);
            record.intensity=std::max(0.0,record.intensity-static_cast<double>(elapsed)*0.00020);
            record.lastUpdatedMinute=minute;
        }
        records_.erase(std::remove_if(records_.begin(),records_.end(),[](const auto& record){
            return record.amount<=0.01 || record.intensity<=0.01;
        }),records_.end());
    }

    EnvironmentalResidueRecord& deposit(
        EnvironmentalResidueKind kind,
        GridPos pos,
        CharacterId sourceCharacter,
        int minute,
        double amount=1.0,
        double intensity=0.42,
        int radiusTiles=3)
    {
        advanceToMinute(minute);
        amount=std::max(0.01,std::min(10.0,amount));
        intensity=std::max(0.01,std::min(1.0,intensity));
        radiusTiles=std::max(1,std::min(16,radiusTiles));

        for(auto& record:records_){
            if(record.kind==kind && record.pos.x==pos.x && record.pos.y==pos.y){
                record.amount=std::min(100.0,record.amount+amount);
                record.intensity=std::min(1.0,record.intensity+intensity*0.35);
                record.radiusTiles=std::max(record.radiusTiles,radiusTiles);
                record.sourceCharacter=sourceCharacter;
                record.lastUpdatedMinute=minute;
                return record;
            }
        }

        EnvironmentalResidueId nextId=1;
        for(const auto& record:records_) nextId=std::max(nextId,record.id+1);
        records_.push_back({
            nextId,kind,pos,sourceCharacter,minute,minute,amount,intensity,radiusTiles
        });
        return records_.back();
    }

    double exposureAt(GridPos pos) const
    {
        double total=0.0;
        for(const auto& record:records_){
            const int distance=manhattan(pos,record.pos);
            if(distance>record.radiusTiles) continue;
            const double falloff=1.0-static_cast<double>(distance)/static_cast<double>(record.radiusTiles+1);
            total+=record.intensity*std::max(0.0,falloff);
        }
        return std::max(0.0,std::min(1.0,total));
    }

private:
    std::vector<EnvironmentalResidueRecord> records_;
};

struct EnvironmentalResidueObservation {
    EnvironmentalResidueId id=0;
    EnvironmentalResidueKind kind=EnvironmentalResidueKind::HumanWaste;
    GridPos pos{};
    CharacterId sourceCharacter=0;
    int ageMinutes=0;
    double amount=0.0;
    double intensity=0.0;
    int radiusTiles=1;
};

struct EnvironmentObservation {
    int minute=0;
    std::size_t totalResidues=0;
    std::size_t humanWasteResidues=0;
    double aggregateAmount=0.0;
    double peakIntensity=0.0;
    std::vector<EnvironmentalResidueObservation> residues;
};

inline EnvironmentObservation buildEnvironmentObservation(
    int currentMinute,
    const EnvironmentalResidueField& field,
    std::size_t maxResidues=64)
{
    EnvironmentObservation result;
    result.minute=currentMinute;
    result.totalResidues=field.all().size();
    for(const auto& record:field.all()){
        if(record.kind==EnvironmentalResidueKind::HumanWaste) ++result.humanWasteResidues;
        result.aggregateAmount+=record.amount;
        result.peakIntensity=std::max(result.peakIntensity,record.intensity);
        if(result.residues.size()>=maxResidues) continue;
        result.residues.push_back({
            record.id,
            record.kind,
            record.pos,
            record.sourceCharacter,
            std::max(0,currentMinute-record.createdMinute),
            record.amount,
            record.intensity,
            record.radiusTiles
        });
    }
    return result;
}

} // namespace lifelens
