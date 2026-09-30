#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

void learnSanitationTechniques(Character& character)
{
    character.civilization.knowledge.learn(
        TechniqueId::DesignatedSanitationArea,
        KnowledgeLevel::Reproducible,
        0.95);
    character.civilization.knowledge.learn(
        TechniqueId::DugSanitationPit,
        KnowledgeLevel::Reproducible,
        0.95);
    character.civilization.craftingSkill=0.85;
    character.personality.patience=0.90;
    character.personality.conscientiousness=0.90;
}

bool containsLog(const Simulation& simulation,const std::string& token)
{
    for(const auto& line:simulation.logs()){
        if(line.find(token)!=std::string::npos) return true;
    }
    return false;
}

const EnvironmentalResidueRecord* newestWasteAt(
    const EnvironmentalResidueField& field,
    GridPos pos)
{
    const EnvironmentalResidueRecord* found=nullptr;
    for(const auto& residue:field.all()){
        if(residue.kind!=EnvironmentalResidueKind::HumanWaste
           || residue.pos.x!=pos.x
           || residue.pos.y!=pos.y){
            continue;
        }
        if(found==nullptr || residue.id>found->id) found=&residue;
    }
    return found;
}

}

int main()
{
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;

    Simulation simulation(
        9302,
        0,
        CurrentWorldGenerationVersion,
        rules);
    simulation.setupNewGame();
    simulation.world().characters.resize(1);

    Character& actor=simulation.world().characters.front();
    const CharacterId actorId=actor.id;
    learnSanitationTechniques(actor);

    const PrimitiveSanitationSiteCreationResult created=
        establishDesignatedSanitationArea(
            simulation.world().seed,
            actor,
            simulation.world().environmentalResidues,
            simulation.world().primitiveSanitationSites,
            simulation.world().minute);
    assert(created.established);

    bool pitCompleted=false;
    for(int i=0;i<12 && !pitCompleted;++i){
        const DugSanitationPitWorkResult work=workOnDugSanitationPit(
            actor,
            simulation.world().environmentalResidues,
            simulation.world().primitiveSanitationSites,
            simulation.world().minute);
        assert(work.worked);
        pitCompleted=work.completed;
    }
    assert(pitCompleted);

    PrimitiveSanitationSite* pit=findPrimitiveSanitationSite(
        simulation.world().primitiveSanitationSites,
        created.siteId);
    assert(pit!=nullptr);
    assert(pit->kind==PrimitiveSanitationSiteKind::DugPit);
    const int usesBefore=pit->useCount;

    actor.needs={0.01,0.01,0.01,0.99,0.10};
    const double hygieneBefore=actor.needs.hygiene;

    bool completed=false;
    for(int minute=0;minute<80 && !completed;++minute){
        simulation.step();
        completed=containsLog(
            simulation,
            actor.name+" completed UseToilet via sanitation site");
    }
    assert(completed);

    pit=findPrimitiveSanitationSite(
        simulation.world().primitiveSanitationSites,
        created.siteId);
    assert(pit!=nullptr);
    assert(pit->useCount==usesBefore+1);

    GridPos residentPos{};
    assert(simulation.runtimePosition(actorId,residentPos));
    assert(residentPos.x==pit->pos.x);
    assert(residentPos.y==pit->pos.y);

    const EnvironmentalResidueRecord* residue=
        newestWasteAt(simulation.world().environmentalResidues,pit->pos);
    assert(residue!=nullptr);
    assert(residue->radiusTiles==
        primitiveSanitationResidueRadiusTiles(
            PrimitiveSanitationSiteKind::DugPit));
    assert(std::abs(
        residue->intensity
        -primitiveSanitationResidueIntensity(
            PrimitiveSanitationSiteKind::DugPit))<1e-9);

    // Two use ticks add 0.008 hygiene burden and the contained completion adds
    // another 0.008. Allow a small margin for environmental exposure applied
    // after the authoritative minute completes.
    assert(actor.needs.hygiene<hygieneBefore+0.04);
    assert(actor.needs.bladder<0.75);

    std::cout<<"headless dug-pit sanitation effects PASS\n";
    return 0;
}
