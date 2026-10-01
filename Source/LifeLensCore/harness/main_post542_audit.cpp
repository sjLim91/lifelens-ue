#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SocialUtility.h"

namespace {
using namespace lifelens;

struct Metrics {
    CharacterId id=0;
    std::string name;
    std::array<double,5> needSum{};
    std::array<double,5> needMax{};
    std::array<std::uint64_t,5> saturatedMinutes{};
    std::uint64_t physicalMinutes=0;
    std::uint64_t socialMinutes=0;
    std::uint64_t civilizationMinutes=0;
    std::uint64_t parentingMinutes=0;
    std::uint64_t teachingMinutes=0;
    std::uint64_t idleMinutes=0;
    std::array<std::uint64_t,6> physicalGoalMinutes{};
    std::uint64_t socialCandidateSamples=0;
    std::uint64_t socialAboveMinimumSamples=0;
    std::uint64_t socialViableSamples=0;
    double socialUtilitySum=0.0;
    double physicalUtilitySum=0.0;
    std::uint64_t containerExperimentBest=0;
    std::uint64_t containerCivilizationBest=0;
    std::uint64_t containerUnifiedWinner=0;
};

const Character* findResident(const World& world,CharacterId id){
    for(const auto& c:world.characters) if(c.id==id) return &c;
    return nullptr;
}

std::array<double,5> needsArray(const Needs& n){
    return {n.hunger,n.thirst,n.sleep,n.bladder,n.hygiene};
}

std::size_t goalIndex(Goal goal){
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

int naturalUnits(const World& world,MaterialKind material){
    int total=0;
    for(const auto& node:world.resourceNodes){
        if(node.material==material) total+=std::max(0,node.quantity);
    }
    return total;
}

int carriedUnits(const World& world,MaterialKind material){
    int total=0;
    for(const auto& c:world.characters){
        total+=material==MaterialKind::Water
            ? portableWaterCount(c.civilization.inventory)
            : c.civilization.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

int storedUnits(const World& world,MaterialKind material){
    int total=0;
    for(const auto& s:world.storageSites){
        total+=material==MaterialKind::Water
            ? portableWaterCount(s.inventory)
            : s.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

}

int main(int argc,char** argv){
    int days=100;
    std::uint64_t seed=874213954;
    for(int i=1;i<argc;++i){
        const std::string arg=argv[i];
        if(arg=="--days" && i+1<argc) days=std::max(1,std::atoi(argv[++i]));
        else if(arg=="--seed" && i+1<argc) seed=std::strtoull(argv[++i],nullptr,10);
    }

    Simulation sim(seed);
    sim.setupNewGame();

    std::vector<Metrics> metrics;
    for(const auto& c:sim.world().characters){
        Metrics m; m.id=c.id; m.name=c.name; metrics.push_back(m);
    }

    std::uint64_t preemptions=0;
    std::uint64_t sleepInterruptions=0;
    std::uint64_t socialEvents=0;
    std::uint64_t civilizationEvents=0;
    std::uint64_t routeFailures=0;
    std::uint64_t timeouts=0;
    std::array<std::uint64_t,5> starts{};
    std::array<std::uint64_t,5> done{};

    sim.onEvent([&](const std::string& line){
        if(line.find("preempted current activity")!=std::string::npos) ++preemptions;
        if(line.find("woke from Sleep")!=std::string::npos) ++sleepInterruptions;
        if(line.find("route failed")!=std::string::npos) ++routeFailures;
        if(line.find("timed out")!=std::string::npos) ++timeouts;
        if(line.find(" -> Civilization ")!=std::string::npos) ++civilizationEvents;
        if(line.find(" -> Approach ")!=std::string::npos
           || line.find(" -> Avoid ")!=std::string::npos
           || line.find(" -> Repair ")!=std::string::npos
           || line.find(" -> Comfort ")!=std::string::npos) ++socialEvents;

        const std::array<const char*,5> names={"Eat","Drink","Sleep","UseToilet","Wash"};
        for(std::size_t i=0;i<names.size();++i){
            if(line.find(std::string(" -> ")+names[i]+" (need ")!=std::string::npos) ++starts[i];
            if(line.find(std::string(" completed ")+names[i])!=std::string::npos) ++done[i];
        }
    });

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
                if(values[i]>=0.999) ++m.saturatedMinutes[i];
            }

            const ResidentPresentationObservation p=sim.observeResidentPresentation(m.id);
            if(!p.active || p.phase==PresentationActionPhase::Idle){
                ++m.idleMinutes;
            }else{
                switch(p.kind){
                    case PresentationActionKind::Physical:
                        ++m.physicalMinutes;
                        ++m.physicalGoalMinutes[goalIndex(p.physicalGoal)];
                        break;
                    case PresentationActionKind::Social: ++m.socialMinutes; break;
                    case PresentationActionKind::Civilization: ++m.civilizationMinutes; break;
                    case PresentationActionKind::Parenting: ++m.parentingMinutes; break;
                    case PresentationActionKind::KnowledgeTeaching: ++m.teachingMinutes; break;
                    case PresentationActionKind::None:
                    default: ++m.idleMinutes; break;
                }
            }

            if(sim.world().minute%5!=0) continue;
            GridPos pos{};
            if(!sim.runtimePosition(m.id,pos)) continue;

            const SocialUtilityDecision social=
                chooseSocialUtilityDecision(sim.world(),*resident,sim.relationships());
            const auto physical=bestPhysicalUtility(sim.world(),*resident);
            if(social.intent!=SocialIntent::None){
                ++m.socialCandidateSamples;
                m.socialUtilitySum+=social.utility;
                m.physicalUtilitySum+=physical.second;
                if(social.utility>=0.18) ++m.socialAboveMinimumSamples;
                if(social.utility>=0.18 && social.utility>physical.second*1.05)
                    ++m.socialViableSamples;
            }

            const CivilizationUtilityDecision experiment=
                bestExperimentDecisionAtPosition(sim.world(),*resident,pos,nullptr);
            if(experiment.intent==CivilizationIntent::Experiment
               && experiment.technique==TechniqueId::SimpleContainer){
                ++m.containerExperimentBest;
            }

            const CivilizationUtilityDecision civilization=
                chooseDispositionAwareCivilizationDecisionAtPosition(
                    sim.world(),*resident,pos,nullptr);
            if(civilization.intent==CivilizationIntent::Experiment
               && civilization.technique==TechniqueId::SimpleContainer){
                ++m.containerCivilizationBest;
            }

            const UnifiedUtilityDecision unified=
                chooseUnifiedUtilityDecisionAtPosition(
                    sim.world(),*resident,sim.relationships(),pos,0.18,0.14,nullptr);
            if(unified.kind==UnifiedDecisionKind::Civilization
               && unified.civilization.technique==TechniqueId::SimpleContainer){
                ++m.containerUnifiedWinner;
            }
        }
    }

    const World& world=sim.world();
    std::cout<<"MAIN_POST542_AUDIT seed="<<seed<<" days="<<days
             <<" minute="<<world.minute<<"\n";
    std::cout<<"WORLD"
             <<" naturalWater="<<naturalUnits(world,MaterialKind::Water)
             <<" naturalFood="<<naturalUnits(world,MaterialKind::PlantFood)
             <<" naturalClay="<<naturalUnits(world,MaterialKind::Clay)
             <<" carriedWater="<<carriedUnits(world,MaterialKind::Water)
             <<" carriedFood="<<carriedUnits(world,MaterialKind::PlantFood)
             <<" storedWater="<<storedUnits(world,MaterialKind::Water)
             <<" storedFood="<<storedUnits(world,MaterialKind::PlantFood)
             <<" chunks="<<world.generatedNaturalChunks.size()
             <<" facilities="<<world.facilities.size()
             <<" storages="<<world.storageSites.size()
             <<" preemptions="<<preemptions
             <<" sleepInterruptions="<<sleepInterruptions
             <<" socialEvents="<<socialEvents
             <<" civilizationEvents="<<civilizationEvents
             <<" routeFailures="<<routeFailures
             <<" timeouts="<<timeouts;
    const std::array<const char*,5> actionNames={"Eat","Drink","Sleep","Toilet","Wash"};
    for(std::size_t i=0;i<actionNames.size();++i){
        std::cout<<" starts"<<actionNames[i]<<"="<<starts[i]
                 <<" done"<<actionNames[i]<<"="<<done[i];
    }
    std::cout<<"\n";

    const std::array<const char*,5> needNames={"hunger","thirst","sleep","bladder","hygiene"};
    const std::array<const char*,6> goalNames={"eat","drink","sleep","toilet","wash","idle"};
    const double denom=static_cast<double>(std::max(1,totalMinutes));

    for(const auto& m:metrics){
        const Character* resident=findResident(world,m.id);
        const int simpleLevel=resident
            ? static_cast<int>(resident->civilization.knowledge.level(TechniqueId::SimpleContainer))
            : 0;
        const int clay=resident
            ? resident->civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Clay)
            : 0;
        const int containers=resident
            ? simpleContainerCount(resident->civilization.inventory)
            : 0;

        std::cout<<"RESIDENT id="<<m.id<<" name="<<m.name
                 <<" techSimpleContainer="<<simpleLevel
                 <<" carriedClay="<<clay
                 <<" simpleContainers="<<containers;
        for(std::size_t i=0;i<needNames.size();++i){
            std::cout<<" "<<needNames[i]<<"Avg="
                     <<std::fixed<<std::setprecision(4)<<(m.needSum[i]/denom)
                     <<" "<<needNames[i]<<"Max="<<m.needMax[i]
                     <<" "<<needNames[i]<<"SatMin="<<m.saturatedMinutes[i];
        }
        std::cout<<" physicalMin="<<m.physicalMinutes
                 <<" socialMin="<<m.socialMinutes
                 <<" civilizationMin="<<m.civilizationMinutes
                 <<" parentingMin="<<m.parentingMinutes
                 <<" teachingMin="<<m.teachingMinutes
                 <<" idleMin="<<m.idleMinutes
                 <<" socialCandidateSamples="<<m.socialCandidateSamples
                 <<" socialAboveMinimumSamples="<<m.socialAboveMinimumSamples
                 <<" socialViableSamples="<<m.socialViableSamples
                 <<" socialCandidateUtilityAvg="
                 <<(m.socialCandidateSamples
                    ? m.socialUtilitySum/static_cast<double>(m.socialCandidateSamples)
                    : 0.0)
                 <<" physicalUtilityAtSocialAvg="
                 <<(m.socialCandidateSamples
                    ? m.physicalUtilitySum/static_cast<double>(m.socialCandidateSamples)
                    : 0.0)
                 <<" containerExperimentBest="<<m.containerExperimentBest
                 <<" containerCivilizationBest="<<m.containerCivilizationBest
                 <<" containerUnifiedWinner="<<m.containerUnifiedWinner;
        for(std::size_t i=0;i<goalNames.size();++i)
            std::cout<<" "<<goalNames[i]<<"Min="<<m.physicalGoalMinutes[i];
        std::cout<<"\n";
    }
    return 0;
}
