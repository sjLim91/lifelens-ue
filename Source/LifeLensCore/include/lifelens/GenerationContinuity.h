#pragma once

#include <algorithm>
#include <cstddef>
#include <queue>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Character.h"
#include "Death.h"
#include "Genealogy.h"
#include "Pregnancy.h"

namespace lifelens {

enum class ContinuityStatus {
    Founding,
    Transitioning,
    Stable,
    AtRisk,
    Extinct
};

struct GenerationContinuityReport {
    ContinuityStatus status=ContinuityStatus::Founding;
    PopulationContinuity population;
    std::size_t reproductiveAdults=0;
    std::size_t activePregnancies=0;
    std::size_t livingWithParents=0;
    int maxGenerationDepth=0;
    double continuityScore=0.0;

    bool adultShortage=false;
    bool reproductiveBaseLow=false;
    bool nextGenerationAbsent=false;
    bool lineageCycleDetected=false;

    bool extinct() const { return status==ContinuityStatus::Extinct; }
    bool atRisk() const { return status==ContinuityStatus::AtRisk || status==ContinuityStatus::Extinct; }
};

inline int ancestorDepth(const GenealogyBook& genealogy,CharacterId id,int maxDepth=16)
{
    if(id==0 || maxDepth<=0) return 0;
    std::queue<std::pair<CharacterId,int>> pending;
    std::unordered_set<CharacterId> seen;
    pending.push({id,0});
    seen.insert(id);
    int deepest=0;

    while(!pending.empty()){
        const auto current=pending.front();
        pending.pop();
        deepest=std::max(deepest,current.second);
        if(current.second>=maxDepth) continue;

        const GenealogyNode* node=genealogy.find(current.first);
        if(node==nullptr) continue;
        for(CharacterId parent:node->parents){
            if(parent!=0 && seen.insert(parent).second){
                pending.push({parent,current.second+1});
            }
        }
    }
    return deepest;
}

struct GenerationDepthIndex {
    std::unordered_map<CharacterId,int> depthByCharacter;
    bool cycleDetected=false;
    bool safetyLimitReached=false;
};

inline GenerationDepthIndex buildGenerationDepthIndex(
    const GenealogyBook& genealogy,
    int safetyDepth=4096)
{
    GenerationDepthIndex result;
    const int safeLimit=std::max(1,safetyDepth);

    std::unordered_map<CharacterId,const GenealogyNode*> nodes;
    nodes.reserve(genealogy.all().size());
    for(const GenealogyNode& node:genealogy.all()){
        if(node.characterId!=0) nodes[node.characterId]=&node;
    }

    // 0 = unseen, 1 = visiting, 2 = complete.
    std::unordered_map<CharacterId,std::uint8_t> state;
    state.reserve(nodes.size());

    std::function<int(CharacterId,int)> visit=
        [&](CharacterId id,int recursionDepth)->int {
            if(id==0) return 0;
            if(recursionDepth>safeLimit){
                result.safetyLimitReached=true;
                return safeLimit;
            }

            const auto memo=result.depthByCharacter.find(id);
            const auto stateIt=state.find(id);
            if(stateIt!=state.end() && stateIt->second==2
               && memo!=result.depthByCharacter.end()){
                return memo->second;
            }
            if(stateIt!=state.end() && stateIt->second==1){
                result.cycleDetected=true;
                return 0;
            }

            state[id]=1;
            int depth=0;
            const auto nodeIt=nodes.find(id);
            if(nodeIt!=nodes.end()){
                for(CharacterId parent:nodeIt->second->parents){
                    if(parent==0) continue;
                    depth=std::max(
                        depth,
                        1+visit(parent,recursionDepth+1));
                }
            }
            state[id]=2;
            result.depthByCharacter[id]=depth;
            return depth;
        };

    for(const auto& entry:nodes){
        visit(entry.first,0);
    }
    return result;
}

inline std::size_t countActivePregnancies(const PregnancyBook& pregnancies)
{
    std::size_t count=0;
    for(const auto& pregnancy:pregnancies.all()) if(pregnancy.active()) ++count;
    return count;
}

inline GenerationContinuityReport assessGenerationContinuity(
    const std::vector<Character*>& residents,
    const GenealogyBook& genealogy,
    const PregnancyBook& pregnancies,
    double reproductiveThreshold=0.15)
{
    GenerationContinuityReport report;
    report.population=summarizePopulation(residents);
    report.activePregnancies=countActivePregnancies(pregnancies);
    const GenerationDepthIndex generationDepths=
        buildGenerationDepthIndex(genealogy);
    report.lineageCycleDetected=generationDepths.cycleDetected
        || generationDepths.safetyLimitReached;

    for(const Character* resident:residents){
        if(resident==nullptr || !resident->alive) continue;

        if(isAdultStage(resident->lifeStage) &&
           resident->lifeCondition.reproductivePotential>=reproductiveThreshold){
            ++report.reproductiveAdults;
        }

        const GenealogyNode* node=genealogy.find(resident->id);
        if(node!=nullptr && !node->parents.empty()) ++report.livingWithParents;
        const auto depthIt=
            generationDepths.depthByCharacter.find(resident->id);
        const int depth=depthIt!=generationDepths.depthByCharacter.end()
            ? depthIt->second
            : 0;
        report.maxGenerationDepth=std::max(
            report.maxGenerationDepth,depth);
    }

    if(report.population.living==0){
        report.status=ContinuityStatus::Extinct;
        report.adultShortage=true;
        report.reproductiveBaseLow=true;
        report.nextGenerationAbsent=true;
        report.continuityScore=0.0;
        return report;
    }

    report.adultShortage=report.population.livingAdults<2;
    report.reproductiveBaseLow=report.reproductiveAdults<2;
    report.nextGenerationAbsent=
        report.population.livingMinors==0 && report.activePregnancies==0 && report.livingWithParents==0;

    const double adultScore=std::min(1.0,static_cast<double>(report.population.livingAdults)/2.0);
    const double reproductiveScore=std::min(1.0,static_cast<double>(report.reproductiveAdults)/2.0);
    const double nextGenerationScore=std::min(
        1.0,
        0.45*static_cast<double>(std::min<std::size_t>(2,report.population.livingMinors))+
        0.35*static_cast<double>(std::min<std::size_t>(1,report.activePregnancies))+
        0.20*static_cast<double>(std::min<std::size_t>(1,report.livingWithParents)));
    const double lineageScore=std::min(1.0,static_cast<double>(report.maxGenerationDepth)/2.0);

    report.continuityScore=
        0.30*adultScore+
        0.25*reproductiveScore+
        0.30*nextGenerationScore+
        0.15*lineageScore;

    const bool futureCohort=
        report.population.livingMinors>0 || report.activePregnancies>0 || report.livingWithParents>0;

    if((report.adultShortage && report.activePregnancies==0) ||
       (report.reproductiveBaseLow && !futureCohort)){
        report.status=ContinuityStatus::AtRisk;
    } else if(report.maxGenerationDepth>=2 && !report.adultShortage && futureCohort){
        report.status=ContinuityStatus::Stable;
    } else if(futureCohort){
        report.status=ContinuityStatus::Transitioning;
    } else {
        report.status=ContinuityStatus::Founding;
    }

    return report;
}

inline const char* continuityStatusName(ContinuityStatus status)
{
    switch(status){
        case ContinuityStatus::Founding: return "Founding";
        case ContinuityStatus::Transitioning: return "Transitioning";
        case ContinuityStatus::Stable: return "Stable";
        case ContinuityStatus::AtRisk: return "AtRisk";
        case ContinuityStatus::Extinct: return "Extinct";
    }
    return "Unknown";
}

} // namespace lifelens
