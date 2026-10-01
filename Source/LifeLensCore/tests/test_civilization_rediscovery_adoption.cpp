#include <iostream>
#include <string>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/CivilizationKnowledgeTransmission.h"
#include "lifelens/CivilizationObserverReadModel.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character resident(CharacterId id,double acceptanceTraits)
{
    Character c;
    c.id=id;
    c.name="Resident"+std::to_string(id);
    c.alive=true;
    c.civilization.character=id;
    c.personality.openness=acceptanceTraits;
    c.personality.curiosity=acceptanceTraits;
    c.personality.adaptability=acceptanceTraits;
    c.personality.conscientiousness=acceptanceTraits;
    return c;
}

static void makeSharpFlakeOperational(Character& c,int successfulUses)
{
    c.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.92);
    for(int i=0;i<successfulUses;++i){
        c.civilization.knowledge.recordSuccessfulUse(
            TechniqueId::SharpFlake);
    }
    c.civilization.inventory.add(
        {ItemKind::SharpFlake,MaterialKind::Flint,1,0.8,1.0});
}

static const CivilizationDiscoveryObservation* findDiscovery(
    const CivilizationWorldObservation& observation,
    CharacterId discoverer)
{
    for(const auto& discovery:observation.recentDiscoveries){
        if(discovery.discovererId==discoverer) return &discovery;
    }
    return nullptr;
}

int main()
{
    constexpr std::uint64_t seed=919191;

    World historyWorld(seed);
    historyWorld.characters.clear();
    historyWorld.facilities.clear();
    historyWorld.storageSites.clear();
    historyWorld.resourceNodes.clear();

    Character rediscoverer=resident(1,0.8);
    makeSharpFlakeOperational(rediscoverer,1);
    historyWorld.characters.push_back(rediscoverer);

    Character formerDiscoverer=resident(90,0.8);
    formerDiscoverer.alive=false;
    formerDiscoverer.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Mastered,0.99);

    SocialKnowledgeBook history;
    CHECK(registerTechniqueOrigin(
        history,formerDiscoverer,TechniqueId::SharpFlake,100,
        CivilizationEventType::Discovered,seed)!=nullptr);

    CHECK(classifyCivilizationDiscoveryEvent(
        history,historyWorld,rediscoverer.id,TechniqueId::SharpFlake,
        CivilizationEventType::Discovered)
        ==CivilizationEventType::Rediscovered);

    CHECK(registerTechniqueOrigin(
        history,historyWorld.characters[0],TechniqueId::SharpFlake,500,
        CivilizationEventType::Rediscovered,seed)!=nullptr);

    const CivilizationWorldObservation recovered=
        buildCivilizationWorldObservation(historyWorld,history);
    const CivilizationDiscoveryObservation* rediscovery=
        findDiscovery(recovered,rediscoverer.id);
    CHECK(rediscovery!=nullptr);
    CHECK(rediscovery->rediscovery);
    CHECK(rediscovery->discoveryOrdinal==2);

    Character livingKeeper=resident(2,0.8);
    livingKeeper.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Observed,0.6);
    historyWorld.characters.push_back(livingKeeper);
    CHECK(classifyCivilizationDiscoveryEvent(
        history,historyWorld,rediscoverer.id,TechniqueId::SharpFlake,
        CivilizationEventType::Discovered)
        ==CivilizationEventType::Discovered);

    SocialKnowledgeBook blankHistory;
    CHECK(classifyCivilizationDiscoveryEvent(
        blankHistory,historyWorld,rediscoverer.id,TechniqueId::SharpFlake,
        CivilizationEventType::Discovered)
        ==CivilizationEventType::Discovered);

    World adoptionWorld(seed+1);
    adoptionWorld.characters.clear();
    adoptionWorld.facilities.clear();
    adoptionWorld.storageSites.clear();
    adoptionWorld.resourceNodes.clear();

    Character established=resident(10,0.8);
    makeSharpFlakeOperational(established,2);
    Character adopting=resident(11,0.8);
    makeSharpFlakeOperational(adopting,1);
    Character resistant=resident(12,0.0);
    makeSharpFlakeOperational(resistant,0);
    Character evaluating=resident(13,1.0);
    makeSharpFlakeOperational(evaluating,0);
    adoptionWorld.characters={
        established,adopting,resistant,evaluating};

    CHECK(technologyAdoptionDisposition(
        adoptionWorld,adoptionWorld.characters[0],
        TechnologyId::SharpFlake)
        ==TechnologyAdoptionDisposition::Established);
    CHECK(technologyAdoptionDisposition(
        adoptionWorld,adoptionWorld.characters[1],
        TechnologyId::SharpFlake)
        ==TechnologyAdoptionDisposition::Adopting);
    CHECK(technologyAdoptionDisposition(
        adoptionWorld,adoptionWorld.characters[2],
        TechnologyId::SharpFlake)
        ==TechnologyAdoptionDisposition::Resistant);
    CHECK(technologyAdoptionDisposition(
        adoptionWorld,adoptionWorld.characters[3],
        TechnologyId::SharpFlake)
        ==TechnologyAdoptionDisposition::Evaluating);

    CivilizationTechnologyPopulationStatus contested=
        observeTechnologyPopulationStatus(
            adoptionWorld,TechnologyId::SharpFlake,true);
    CHECK(contested.adoptedResidentCount==1);
    CHECK(contested.adoptingResidentCount==1);
    CHECK(contested.resistantResidentCount==1);
    CHECK(contested.adoptionState
        ==TechnologySocialAdoptionState::Contested);

    adoptionWorld.characters[1].civilization.knowledge.recordSuccessfulUse(
        TechniqueId::SharpFlake);
    const CivilizationTechnologyPopulationStatus sociallyEstablished=
        observeTechnologyPopulationStatus(
            adoptionWorld,TechnologyId::SharpFlake,true);
    CHECK(sociallyEstablished.adoptedResidentCount==2);
    CHECK(sociallyEstablished.adoptionState
        ==TechnologySocialAdoptionState::Established);

    World resistedWorld(seed+2);
    resistedWorld.characters.clear();
    resistedWorld.facilities.clear();
    resistedWorld.storageSites.clear();
    resistedWorld.resourceNodes.clear();
    for(CharacterId id=20;id<24;++id){
        Character c=resident(id,0.0);
        makeSharpFlakeOperational(c,0);
        resistedWorld.characters.push_back(c);
    }
    const CivilizationTechnologyPopulationStatus sociallyResisted=
        observeTechnologyPopulationStatus(
            resistedWorld,TechnologyId::SharpFlake,true);
    CHECK(sociallyResisted.resistantResidentCount==4);
    CHECK(sociallyResisted.adoptedResidentCount==0);
    CHECK(sociallyResisted.adoptionState
        ==TechnologySocialAdoptionState::Resisted);

    CivilizationUtilityDecision craft;
    craft.intent=CivilizationIntent::Craft;
    craft.technique=TechniqueId::SharpFlake;
    craft.utility=0.50;
    const CivilizationUtilityDecision resistedCraft=
        applyTechnologyAdoptionUtility(
            resistedWorld,resistedWorld.characters[0],craft);
    const CivilizationUtilityDecision evaluatingCraft=
        applyTechnologyAdoptionUtility(
            adoptionWorld,adoptionWorld.characters[3],craft);
    CHECK(resistedCraft.utility<craft.utility);
    CHECK(evaluatingCraft.utility>craft.utility);

    Character blocked=resident(30,0.0);
    blocked.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.9);
    World blockedWorld(seed+3);
    blockedWorld.characters.clear();
    blockedWorld.facilities.clear();
    blockedWorld.storageSites.clear();
    blockedWorld.resourceNodes.clear();
    blockedWorld.characters.push_back(blocked);
    CHECK(technologyAdoptionDisposition(
        blockedWorld,blockedWorld.characters[0],
        TechnologyId::SharpFlake)
        ==TechnologyAdoptionDisposition::Blocked);

    std::cout << "civilization rediscovery + adoption closeout passed\n";
    return 0;
}
