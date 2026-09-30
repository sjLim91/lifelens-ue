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

    // Progression/discovery is covered by test_primitive_latrine_progression.
    // This regression isolates the autonomous physical-runtime contract by
    // placing one already-authoritative operational DugPit at the resident's
    // current Core position.
    GridPos initialPos{};
    assert(simulation.runtimePosition(actorId,initialPos));
    PrimitiveSanitationSite builtPit;
    builtPit.id=930201;
    builtPit.kind=PrimitiveSanitationSiteKind::DugPit;
    builtPit.pos=initialPos;
    builtPit.establishedBy=actorId;
    builtPit.establishedMinute=simulation.world().minute;
    builtPit.active=true;
    builtPit.useCount=0;
    builtPit.improvementWork=DugSanitationPitWorkRequired;
    builtPit.improvedBy=actorId;
    builtPit.improvedMinute=simulation.world().minute;
    assert(validPrimitiveSanitationSite(builtPit));
    simulation.world().primitiveSanitationSites.push_back(builtPit);

    PrimitiveSanitationSite* pit=findPrimitiveSanitationSite(
        simulation.world().primitiveSanitationSites,
        builtPit.id);
    assert(pit!=nullptr);
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
        builtPit.id);
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
    const double pitIntensity=
        primitiveSanitationResidueIntensity(
            PrimitiveSanitationSiteKind::DugPit);
    // The simulation advances the environmental field after the action minute,
    // so the freshly deposited authoritative intensity may already have one
    // minute of deterministic decay applied.
    assert(residue->intensity<=pitIntensity+1e-9);
    assert(residue->intensity>=pitIntensity-0.001);

    // Two use ticks add 0.008 hygiene burden and the contained completion adds
    // another 0.008. Allow a small margin for environmental exposure applied
    // after the authoritative minute completes.
    assert(actor.needs.hygiene<hygieneBefore+0.04);
    assert(actor.needs.bladder<0.75);

    std::cout<<"headless dug-pit sanitation effects PASS\n";
    return 0;
}
