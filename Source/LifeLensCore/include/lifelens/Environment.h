#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Character.h"
#include "SmartObject.h"

namespace lifelens {

using EnvironmentalResidueId=std::uint64_t;

enum class EnvironmentalResidueType : std::uint8_t {
    SanitationWaste=0
};

struct EnvironmentalResidue {
    EnvironmentalResidueId id=0;
    EnvironmentalResidueType type=EnvironmentalResidueType::SanitationWaste;
    GridPos location{};
    double amount=0.0;
    double intensity=0.0;
    double influenceRadius=1.0;
    int createdMinute=0;
    int lastUpdatedMinute=0;
    CharacterId sourceResident=0;
    double decayPerDay=0.08;
    double contaminationRisk=0.0;
    double discomfort=0.0;
};

struct EnvironmentalExposure {
    double contaminationRisk=0.0;
    double discomfort=0.0;
    double amount=0.0;
};

inline bool sameResidueSite(const EnvironmentalResidue& residue,EnvironmentalResidueType type,GridPos location)
{
    return residue.type==type && residue.location.x==location.x && residue.location.y==location.y;
}

inline void refreshResidueDerivedValues(EnvironmentalResidue& residue)
{
    residue.amount=std::max(0.0,residue.amount);
    residue.intensity=std::clamp(residue.amount/3.0,0.0,1.0);
    residue.influenceRadius=1.0+residue.intensity*3.0;
    residue.contaminationRisk=std::clamp(residue.amount/5.0,0.0,1.0);
    residue.discomfort=std::clamp(residue.amount/4.0,0.0,1.0);
}

inline EnvironmentalResidueId nextEnvironmentalResidueId(const std::vector<EnvironmentalResidue>& residues)
{
    EnvironmentalResidueId result=1;
    for(const auto& residue:residues) result=std::max(result,residue.id+1);
    return result;
}

inline EnvironmentalResidue& addOrAccumulateResidue(
    std::vector<EnvironmentalResidue>& residues,
    EnvironmentalResidueType type,
    GridPos location,
    double amount,
    int minute,
    CharacterId sourceResident)
{
    for(auto& residue:residues){
        if(!sameResidueSite(residue,type,location)) continue;
        residue.amount+=std::max(0.0,amount);
        residue.lastUpdatedMinute=minute;
        residue.sourceResident=sourceResident;
        refreshResidueDerivedValues(residue);
        return residue;
    }

    EnvironmentalResidue residue;
    residue.id=nextEnvironmentalResidueId(residues);
    residue.type=type;
    residue.location=location;
    residue.amount=std::max(0.0,amount);
    residue.createdMinute=minute;
    residue.lastUpdatedMinute=minute;
    residue.sourceResident=sourceResident;
    refreshResidueDerivedValues(residue);
    residues.push_back(residue);
    return residues.back();
}

inline void decayEnvironmentalResidues(std::vector<EnvironmentalResidue>& residues,int minute)
{
    for(auto& residue:residues){
        const int elapsed=std::max(0,minute-residue.lastUpdatedMinute);
        if(elapsed==0) continue;
        const double days=static_cast<double>(elapsed)/(24.0*60.0);
        residue.amount=std::max(0.0,residue.amount-residue.decayPerDay*days);
        residue.lastUpdatedMinute=minute;
        refreshResidueDerivedValues(residue);
    }
    residues.erase(
        std::remove_if(residues.begin(),residues.end(),[](const EnvironmentalResidue& residue){
            return residue.amount<=0.0001;
        }),
        residues.end());
}

inline EnvironmentalExposure queryEnvironmentalExposure(
    const std::vector<EnvironmentalResidue>& residues,
    GridPos location,
    double radius=0.0)
{
    EnvironmentalExposure result;
    for(const auto& residue:residues){
        const double dx=static_cast<double>(location.x-residue.location.x);
        const double dy=static_cast<double>(location.y-residue.location.y);
        const double distance=std::sqrt(dx*dx+dy*dy);
        const double reach=residue.influenceRadius+std::max(0.0,radius);
        if(distance>reach) continue;
        const double falloff=reach<=0.0 ? 1.0 : std::clamp(1.0-distance/reach,0.0,1.0);
        result.contaminationRisk=std::clamp(result.contaminationRisk+residue.contaminationRisk*falloff,0.0,1.0);
        result.discomfort=std::clamp(result.discomfort+residue.discomfort*falloff,0.0,1.0);
        result.amount+=residue.amount*falloff;
    }
    return result;
}

} // namespace lifelens
