#pragma once

#include <algorithm>

namespace lifelens {

constexpr int LifeMinutesPerDay=24*60;
constexpr int LifeDaysPerYear=365;
constexpr int LifeMinutesPerYear=LifeDaysPerYear*LifeMinutesPerDay;

enum class LifeStage {
    Baby,
    Toddler,
    Child,
    Teen,
    YoungAdult,
    Adult,
    MiddleAge,
    Elderly
};

struct LifeStageProfile {
    double metabolismMultiplier=1.0;
    double sleepTendencyMultiplier=1.0;
    double movementScale=1.0;
    double skillLearningRate=1.0;
    double responsibility=0.0;
    double autonomy=0.0;
    bool canAttendSchool=false;
    bool canWork=false;
    bool canLiveIndependently=false;
    bool canFormRomance=false;
    bool canMarry=false;
    bool canParent=false;
};

inline int ageYearsFromMinutes(int birthMinute,int currentMinute)
{
    if(currentMinute<=birthMinute) return 0;
    return (currentMinute-birthMinute)/LifeMinutesPerYear;
}

inline LifeStage lifeStageForAgeYears(int ageYears)
{
    const int age=std::max(0,ageYears);
    if(age<2) return LifeStage::Baby;
    if(age<5) return LifeStage::Toddler;
    if(age<13) return LifeStage::Child;
    if(age<18) return LifeStage::Teen;
    if(age<25) return LifeStage::YoungAdult;
    if(age<45) return LifeStage::Adult;
    if(age<65) return LifeStage::MiddleAge;
    return LifeStage::Elderly;
}

inline LifeStageProfile lifeStageProfile(LifeStage stage)
{
    switch(stage){
        case LifeStage::Baby:
            return {1.18,1.55,0.20,1.30,0.00,0.02,false,false,false,false,false,false};
        case LifeStage::Toddler:
            return {1.16,1.35,0.55,1.45,0.03,0.20,false,false,false,false,false,false};
        case LifeStage::Child:
            return {1.12,1.15,0.82,1.40,0.18,0.48,true,false,false,false,false,false};
        case LifeStage::Teen:
            return {1.08,1.10,0.95,1.28,0.42,0.72,true,false,false,true,false,false};
        case LifeStage::YoungAdult:
            return {1.04,1.00,1.00,1.18,0.76,0.96,false,true,true,true,true,true};
        case LifeStage::Adult:
            return {1.00,1.00,1.00,1.00,0.92,1.00,false,true,true,true,true,true};
        case LifeStage::MiddleAge:
            return {0.96,1.04,0.96,0.88,0.94,1.00,false,true,true,true,true,true};
        case LifeStage::Elderly:
            return {0.88,1.18,0.78,0.72,0.74,0.92,false,false,true,true,true,false};
    }
    return {};
}

inline const char* lifeStageName(LifeStage stage)
{
    switch(stage){
        case LifeStage::Baby: return "Baby";
        case LifeStage::Toddler: return "Toddler";
        case LifeStage::Child: return "Child";
        case LifeStage::Teen: return "Teen";
        case LifeStage::YoungAdult: return "YoungAdult";
        case LifeStage::Adult: return "Adult";
        case LifeStage::MiddleAge: return "MiddleAge";
        case LifeStage::Elderly: return "Elderly";
    }
    return "Unknown";
}

} // namespace lifelens
