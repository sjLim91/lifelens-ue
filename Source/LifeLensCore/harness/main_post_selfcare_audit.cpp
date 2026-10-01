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

using namespace lifelens;

namespace {

struct ResidentMetrics {
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

    std::uint64_t pressureSamples=0;
    std::uint64_t urgentPhysicalSamples=0;
    std::uint64_t socialCandidateSamples=0;
    std::uint64_t socialAboveMinimumSamples=0;
    std::uint64_t socialPhysicalDominatedSamples=0;
    std::uint64_t socialViableSamples=0;
    std::uint64_t socialViableUrgentSamples=0;
    double socialUtilitySum=0.0;
    double socialUtilityMax=0.0;
    double physicalUtilitySum=0.0;
};

std::array<double,5> needsArray(const Needs& needs)
{
    return {
        needs.hunger,
        needs.thirst,
        needs.sleep,
        needs.bladder,
        needs.hygiene
    };
}

const Character* findResident(const World& world, CharacterId id)
{
    for(const auto& resident:world.characters){
        if(resident.id==id) return &resident;
    }
    return nullptr;
}

bool containsAny(const std::string& line,const std::array<const char*,4>& tokens)
{
    for(const char* token:tokens){
        if(line.find(token)!=std::string::npos) return true;
    }
    return false;
}

}

int main(int argc,char** argv)
{
    WorldSeed seed=874213954;
    int days=100;
    for(int i=1;i<argc;++i){
        const std::string arg=argv[i];
        if(arg=="--seed" && i+1<argc){
            seed=static_cast<WorldSeed>(std::strtoull(argv[++i],nullptr,10));
        }else if(arg=="--days" && i+1<argc){
            days=std::max(1,std::atoi(argv[++i]));
        }
    }

    Simulation sim(seed,0,CurrentWorldGenerationVersion);
    sim.setupNewGame();

    std::vector<ResidentMetrics> metrics;
    metrics.reserve(sim.world().characters.size());
    for(const auto& resident:sim.world().characters){
        ResidentMetrics metric;
        metric.id=resident.id;
        metric.name=resident.name;
        metrics.push_back(metric);
    }

    std::uint64_t preemptions=0;
    std::uint64_t sleepInterruptions=0;
    std::uint64_t socialEvents=0;
    std::uint64_t civilizationEvents=0;
    std::array<std::uint64_t,5> physicalStarts{};
    std::array<std::uint64_t,5> physicalCompletions{};
    const std::array<const char*,5> physicalNames={
        "Eat","Drink","Sleep","UseToilet","Wash"
    };
    const std::array<const char*,4> socialTokens={
        " -> Approach "," -> Avoid "," -> Repair "," -> Comfort "
    };

    sim.onEvent([&](const std::string& line){
        if(line.find("preempted current activity")!=std::string::npos){
            ++preemptions;
        }
        if(line.find("woke from Sleep")!=std::string::npos){
            ++sleepInterruptions;
        }
        if(line.find(" -> Civilization ")!=std::string::npos){
            ++civilizationEvents;
        }
        if(containsAny(line,socialTokens)){
            ++socialEvents;
        }

        for(std::size_t i=0;i<physicalNames.size();++i){
            const std::string startToken=
                std::string(" -> ")+physicalNames[i]+" (need ";
            const std::string completeToken=
                std::string(" completed ")+physicalNames[i];
            if(line.find(startToken)!=std::string::npos){
                ++physicalStarts[i];
            }
            if(line.find(completeToken)!=std::string::npos){
                ++physicalCompletions[i];
            }
        }
    });

    const int totalMinutes=days*24*60;
    const double minimumSocialUtility=0.18;

    for(int minute=0;minute<totalMinutes;++minute){
        sim.step();

        for(auto& metric:metrics){
            const Character* resident=findResident(sim.world(),metric.id);
            if(resident==nullptr || !resident->alive) continue;

            const auto values=needsArray(resident->needs);
            for(std::size_t i=0;i<values.size();++i){
                metric.needSum[i]+=values[i];
                metric.needMax[i]=std::max(metric.needMax[i],values[i]);
                if(values[i]>=0.999){
                    ++metric.saturatedMinutes[i];
                }
            }

            const ResidentPresentationObservation presentation=
                sim.observeResidentPresentation(metric.id);
            if(!presentation.active){
                ++metric.idleMinutes;
            }else{
                switch(presentation.kind){
                    case PresentationActionKind::Physical:
                        ++metric.physicalMinutes; break;
                    case PresentationActionKind::Social:
                        ++metric.socialMinutes; break;
                    case PresentationActionKind::Civilization:
                        ++metric.civilizationMinutes; break;
                    case PresentationActionKind::Parenting:
                        ++metric.parentingMinutes; break;
                    case PresentationActionKind::KnowledgeTeaching:
                        ++metric.teachingMinutes; break;
                    case PresentationActionKind::None:
                    default:
                        ++metric.idleMinutes; break;
                }
            }

            if(sim.world().minute%5!=0) continue;

            ++metric.pressureSamples;
            const double urgent=sim.ruleset().utilityAI.urgentThreshold;
            const bool hasUrgentPhysical=
                resident->needs.hunger>=urgent
                || resident->needs.thirst>=urgent
                || resident->needs.sleep>=urgent
                || resident->needs.bladder>=urgent
                || resident->needs.hygiene>=urgent;
            if(hasUrgentPhysical){
                ++metric.urgentPhysicalSamples;
            }

            const SocialUtilityDecision social=
                chooseSocialUtilityDecision(
                    sim.world(),*resident,sim.relationships());
            const auto physical=
                bestPhysicalUtility(sim.world(),*resident);

            if(social.intent==SocialIntent::None) continue;

            ++metric.socialCandidateSamples;
            metric.socialUtilitySum+=social.utility;
            metric.socialUtilityMax=std::max(
                metric.socialUtilityMax,social.utility);
            metric.physicalUtilitySum+=physical.second;

            if(social.utility<minimumSocialUtility) continue;
            ++metric.socialAboveMinimumSamples;

            if(social.utility<=physical.second*1.05){
                ++metric.socialPhysicalDominatedSamples;
                continue;
            }

            ++metric.socialViableSamples;
            if(hasUrgentPhysical){
                ++metric.socialViableUrgentSamples;
            }
        }
    }

    std::cout
        <<"SUMMARY seed="<<seed
        <<" days="<<days
        <<" preemptions="<<preemptions
        <<" sleepInterruptions="<<sleepInterruptions
        <<" socialEvents="<<socialEvents
        <<" civilizationEvents="<<civilizationEvents
        <<" facilities="<<sim.world().facilities.size()
        <<" sanitationSites="<<sim.world().primitiveSanitationSites.size()
        <<" storageSites="<<sim.world().storageSites.size()
        <<" cultivatedPlots="<<sim.world().cultivatedPlots.size();

    for(std::size_t i=0;i<physicalNames.size();++i){
        std::cout
            <<" physicalStarts"<<physicalNames[i]<<"="<<physicalStarts[i]
            <<" physicalDone"<<physicalNames[i]<<"="<<physicalCompletions[i];
    }
    std::cout<<"\n";

    const std::array<const char*,5> needNames={
        "hunger","thirst","sleep","bladder","hygiene"
    };
    const double denom=static_cast<double>(std::max(1,totalMinutes));

    for(const auto& metric:metrics){
        const Character* resident=findResident(sim.world(),metric.id);
        double outgoingBondSum=0.0;
        double outgoingBondMax=0.0;
        int outgoingRelations=0;
        if(resident!=nullptr){
            for(const auto& other:sim.world().characters){
                if(other.id==resident->id) continue;
                const Relationship* relation=
                    sim.relationships().find(resident->id,other.id);
                if(relation==nullptr) continue;
                const double bond=relation->socialBond();
                outgoingBondSum+=bond;
                outgoingBondMax=std::max(outgoingBondMax,bond);
                ++outgoingRelations;
            }
        }

        std::cout
            <<"RESIDENT id="<<metric.id
            <<" name="<<metric.name;
        for(std::size_t i=0;i<needNames.size();++i){
            std::cout
                <<" "<<needNames[i]<<"Avg="
                <<std::fixed<<std::setprecision(4)
                <<(metric.needSum[i]/denom)
                <<" "<<needNames[i]<<"Max="<<metric.needMax[i]
                <<" "<<needNames[i]<<"SatMin="<<metric.saturatedMinutes[i];
        }

        std::cout
            <<" physicalMin="<<metric.physicalMinutes
            <<" socialMin="<<metric.socialMinutes
            <<" civilizationMin="<<metric.civilizationMinutes
            <<" parentingMin="<<metric.parentingMinutes
            <<" teachingMin="<<metric.teachingMinutes
            <<" idleMin="<<metric.idleMinutes
            <<" pressureSamples="<<metric.pressureSamples
            <<" urgentPhysicalSamples="<<metric.urgentPhysicalSamples
            <<" socialCandidateSamples="<<metric.socialCandidateSamples
            <<" socialAboveMinimumSamples="<<metric.socialAboveMinimumSamples
            <<" socialPhysicalDominatedSamples="<<metric.socialPhysicalDominatedSamples
            <<" socialViableSamples="<<metric.socialViableSamples
            <<" socialViableUrgentSamples="<<metric.socialViableUrgentSamples
            <<" socialUtilityAvg="
            <<(metric.socialCandidateSamples>0
                ? metric.socialUtilitySum/static_cast<double>(metric.socialCandidateSamples)
                : 0.0)
            <<" socialUtilityMax="<<metric.socialUtilityMax
            <<" physicalUtilityAvg="
            <<(metric.socialCandidateSamples>0
                ? metric.physicalUtilitySum/static_cast<double>(metric.socialCandidateSamples)
                : 0.0)
            <<" outgoingBondAvg="
            <<(outgoingRelations>0
                ? outgoingBondSum/static_cast<double>(outgoingRelations)
                : 0.0)
            <<" outgoingBondMax="<<outgoingBondMax
            <<"\n";
    }

    return 0;
}
