#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"

namespace {

using namespace lifelens;

struct ResidentMetrics {
    CharacterId id=0;
    std::string name;
    std::array<double,5> needSum{};
    std::array<double,5> needMax{};
    std::array<std::uint64_t,5> saturatedMinutes{};
    std::array<std::uint64_t,5> saturatedStreak{};
    std::array<std::uint64_t,5> longestSaturatedStreak{};
    std::uint64_t currentIdleStreak=0;
    std::uint64_t longestIdleStreak=0;
    std::uint64_t physicalMinutes=0;
    std::uint64_t socialMinutes=0;
    std::uint64_t civilizationMinutes=0;
    std::uint64_t parentingMinutes=0;
    std::uint64_t teachingMinutes=0;
    std::uint64_t idleMinutes=0;
    std::array<std::uint64_t,6> physicalGoalMinutes{};
};

std::array<double,5> needsArray(const Needs& n)
{
    return {n.hunger,n.thirst,n.sleep,n.bladder,n.hygiene};
}

std::size_t goalIndex(Goal goal)
{
    switch(goal){
        case Goal::Eat: return 0;
        case Goal::Drink: return 1;
        case Goal::Sleep: return 2;
        case Goal::UseToilet: return 3;
        case Goal::Wash: return 4;
        case Goal::Idle:
        default: return 5;
    }
}

int naturalUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& node:world.resourceNodes){
        if(node.material==material) total+=std::max(0,node.quantity);
    }
    return total;
}

int carriedUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& resident:world.characters){
        total+=resident.civilization.inventory.count(
            ItemKind::RawMaterial,material);
    }
    return total;
}

int storedUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& storage:world.storageSites){
        total+=storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

int facilityCount(const World& world,FacilityKind kind)
{
    int total=0;
    for(const auto& facility:world.facilities){
        if(facility.kind==kind && facility.active) ++total;
    }
    return total;
}

const Character* findResident(const World& world,CharacterId id)
{
    for(const auto& c:world.characters) if(c.id==id) return &c;
    return nullptr;
}

}

int main(int argc,char** argv)
{
    int days=30;
    std::uint64_t seed=874213954;
    for(int i=1;i<argc;++i){
        const std::string arg=argv[i];
        if(arg=="--days" && i+1<argc){
            days=std::max(1,std::atoi(argv[++i]));
        }else if(arg=="--seed" && i+1<argc){
            seed=std::strtoull(argv[++i],nullptr,10);
        }
    }

    Simulation sim(seed);
    sim.setupNewGame();

    std::uint64_t routeFailures=0;
    std::uint64_t timeouts=0;
    std::uint64_t preemptions=0;
    std::uint64_t sleepInterruptions=0;
    std::uint64_t socialEvents=0;
    std::uint64_t civilizationEvents=0;
    std::array<std::uint64_t,5> physicalStarts{};
    std::array<std::uint64_t,5> physicalCompletions{};
    sim.onEvent([&](const std::string& line){
        if(line.find("route failed")!=std::string::npos) ++routeFailures;
        if(line.find("timed out")!=std::string::npos) ++timeouts;
        if(line.find("preempted current activity")!=std::string::npos) ++preemptions;
        if(line.find("woke from Sleep")!=std::string::npos) ++sleepInterruptions;
        if(line.find(" -> Civilization ")!=std::string::npos) ++civilizationEvents;
        if(line.find(" -> Approach ")!=std::string::npos
           || line.find(" -> Avoid ")!=std::string::npos
           || line.find(" -> Repair ")!=std::string::npos
           || line.find(" -> Comfort ")!=std::string::npos){
            ++socialEvents;
        }

        const std::array<const char*,5> physicalNames={
            "Eat","Drink","Sleep","UseToilet","Wash"
        };
        for(std::size_t i=0;i<physicalNames.size();++i){
            const std::string startToken=
                std::string(" -> ")+physicalNames[i]+" (need ";
            const std::string completeToken=
                std::string(" completed ")+physicalNames[i];
            if(line.find(startToken)!=std::string::npos) ++physicalStarts[i];
            if(line.find(completeToken)!=std::string::npos) ++physicalCompletions[i];
        }
    });

    std::vector<ResidentMetrics> metrics;
    metrics.reserve(sim.world().characters.size());
    for(const auto& resident:sim.world().characters){
        ResidentMetrics m;
        m.id=resident.id;
        m.name=resident.name;
        metrics.push_back(m);
    }

    const int totalMinutes=days*24*60;
    for(int minute=0;minute<totalMinutes;++minute){
        sim.step();

        for(auto& m:metrics){
            const Character* resident=findResident(sim.world(),m.id);
            if(resident==nullptr || !resident->alive) continue;

            const auto values=needsArray(resident->needs);
            for(std::size_t i=0;i<values.size();++i){
                m.needSum[i]+=values[i];
                m.needMax[i]=std::max(m.needMax[i],values[i]);
                if(values[i]>=0.999){
                    ++m.saturatedMinutes[i];
                    ++m.saturatedStreak[i];
                    m.longestSaturatedStreak[i]=std::max(
                        m.longestSaturatedStreak[i],m.saturatedStreak[i]);
                }else{
                    m.saturatedStreak[i]=0;
                }
            }

            const ResidentPresentationObservation p=
                sim.observeResidentPresentation(m.id);
            if(!p.active || p.phase==PresentationActionPhase::Idle){
                ++m.idleMinutes;
                ++m.currentIdleStreak;
                m.longestIdleStreak=std::max(
                    m.longestIdleStreak,m.currentIdleStreak);
                continue;
            }
            m.currentIdleStreak=0;
            switch(p.kind){
                case PresentationActionKind::Physical:
                    ++m.physicalMinutes;
                    ++m.physicalGoalMinutes[goalIndex(p.physicalGoal)];
                    break;
                case PresentationActionKind::Social:
                    ++m.socialMinutes;
                    break;
                case PresentationActionKind::Civilization:
                    ++m.civilizationMinutes;
                    break;
                case PresentationActionKind::Parenting:
                    ++m.parentingMinutes;
                    break;
                case PresentationActionKind::KnowledgeTeaching:
                    ++m.teachingMinutes;
                    break;
                case PresentationActionKind::None:
                default:
                    ++m.idleMinutes;
                    break;
            }
        }
    }

    const World& world=sim.world();
    std::cout<<"BALANCE_AUDIT seed="<<seed<<" days="<<days
             <<" minute="<<world.minute<<"\n";
    std::cout<<"WORLD naturalWater="<<naturalUnits(world,MaterialKind::Water)
             <<" naturalFood="<<naturalUnits(world,MaterialKind::PlantFood)
             <<" carriedWater="<<carriedUnits(world,MaterialKind::Water)
             <<" carriedFood="<<carriedUnits(world,MaterialKind::PlantFood)
             <<" storedWater="<<storedUnits(world,MaterialKind::Water)
             <<" storedFood="<<storedUnits(world,MaterialKind::PlantFood)
             <<" chunks="<<world.generatedNaturalChunks.size()
             <<" facilities="<<world.facilities.size()
             <<" primitiveStorageFacilities="<<facilityCount(world,FacilityKind::PrimitiveStorage)
             <<" firePits="<<facilityCount(world,FacilityKind::FirePit)
             <<" workSurfaces="<<facilityCount(world,FacilityKind::WorkSurface)
             <<" sleepingPlaces="<<facilityCount(world,FacilityKind::SleepingPlace)
             <<" shelters="<<facilityCount(world,FacilityKind::Shelter)
             <<" furnaces="<<facilityCount(world,FacilityKind::Furnace)
             <<" plots="<<facilityCount(world,FacilityKind::CultivatedPlot)
             <<" storages="<<world.storageSites.size()
             <<" routeFailures="<<routeFailures
             <<" timeouts="<<timeouts
             <<" preemptions="<<preemptions
             <<" sleepInterruptions="<<sleepInterruptions
             <<" socialEvents="<<socialEvents
             <<" civilizationEvents="<<civilizationEvents
             <<" physicalStartsEat="<<physicalStarts[0]
             <<" physicalStartsDrink="<<physicalStarts[1]
             <<" physicalStartsSleep="<<physicalStarts[2]
             <<" physicalStartsToilet="<<physicalStarts[3]
             <<" physicalStartsWash="<<physicalStarts[4]
             <<" physicalDoneEat="<<physicalCompletions[0]
             <<" physicalDoneDrink="<<physicalCompletions[1]
             <<" physicalDoneSleep="<<physicalCompletions[2]
             <<" physicalDoneToilet="<<physicalCompletions[3]
             <<" physicalDoneWash="<<physicalCompletions[4]
             <<"\n";

    const std::array<const char*,5> needNames={
        "hunger","thirst","sleep","bladder","hygiene"
    };
    const std::array<const char*,6> goalNames={
        "eat","drink","sleep","toilet","wash","idle"
    };

    for(const auto& m:metrics){
        const Character* resident=findResident(world,m.id);
        int reproducibleTechniques=0;
        bool knowsDiggingStick=false;
        bool knowsCultivation=false;
        int diggingStickTools=0;
        if(resident!=nullptr){
            for(const auto& record:resident->civilization.knowledge.all()){
                if(static_cast<int>(record.level)>=
                   static_cast<int>(KnowledgeLevel::Reproducible)){
                    ++reproducibleTechniques;
                }
            }
            knowsDiggingStick=resident->civilization.knowledge.knowsAtLeast(
                TechniqueId::DiggingStick,KnowledgeLevel::Reproducible);
            knowsCultivation=resident->civilization.knowledge.knowsAtLeast(
                TechniqueId::Cultivation,KnowledgeLevel::Reproducible);
            diggingStickTools=resident->civilization.inventory.count(
                ItemKind::DiggingStick,MaterialKind::Unknown,true);
        }
        std::cout<<"RESIDENT id="<<m.id<<" name="<<m.name
                 <<" reproducibleTechniques="<<reproducibleTechniques
                 <<" knowsDiggingStick="<<(knowsDiggingStick?1:0)
                 <<" knowsCultivation="<<(knowsCultivation?1:0)
                 <<" diggingStickTools="<<diggingStickTools;
        const double denom=static_cast<double>(std::max(1,totalMinutes));
        for(std::size_t i=0;i<needNames.size();++i){
            std::cout<<" "<<needNames[i]<<"Avg="<<std::fixed<<std::setprecision(4)
                     <<(m.needSum[i]/denom)
                     <<" "<<needNames[i]<<"Max="<<m.needMax[i]
                     <<" "<<needNames[i]<<"SatMin="<<m.saturatedMinutes[i]
                     <<" "<<needNames[i]<<"LongestSat="<<m.longestSaturatedStreak[i];
        }
        std::cout<<" longestIdle="<<m.longestIdleStreak
                 <<" physicalMin="<<m.physicalMinutes
                 <<" socialMin="<<m.socialMinutes
                 <<" civilizationMin="<<m.civilizationMinutes
                 <<" parentingMin="<<m.parentingMinutes
                 <<" teachingMin="<<m.teachingMinutes
                 <<" idleMin="<<m.idleMinutes;
        for(std::size_t i=0;i<goalNames.size();++i){
            std::cout<<" "<<goalNames[i]<<"Min="<<m.physicalGoalMinutes[i];
        }
        std::cout<<"\n";
    }

    return 0;
}
