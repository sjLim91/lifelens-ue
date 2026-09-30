#include <cassert>
#include <optional>

#include "lifelens/ContextAction.h"
#include "lifelens/SocialCognition.h"
#include "lifelens/SocialUtility.h"

using namespace lifelens;

static SmartObject makeFridge()
{
    SmartObject fridge;
    fridge.id = 100;
    fridge.kind = ObjectKind::Fridge;
    fridge.pos = {1, 1};
    fridge.effectPerTick.hunger = -0.08;
    fridge.useDurationTicks = 8;
    return fridge;
}

int main()
{
    // Strong positive relationship should create an Approach decision when physical needs are calm.
    World friendlyWorld(42);
    friendlyWorld.minute = 10 * 60;

    Character self;
    self.id = 1;
    self.name = "Self";
    self.personality.sociability = 0.90;
    self.personality.curiosity = 0.75;
    self.personality.introversion = 0.10;

    Character friendCharacter;
    friendCharacter.id = 2;
    friendCharacter.name = "Friend";

    friendlyWorld.characters = {self, friendCharacter};

    RelationshipBook friendlyRelationships;
    Relationship& friendRelation = friendlyRelationships.getOrCreate(1, 2);
    friendRelation.affection = 0.82;
    friendRelation.trust = 0.86;
    friendRelation.respect = 0.72;
    friendRelation.comfort = 0.84;
    friendRelation.familiarity = 0.90;

    SocialUtilityDecision friendlySocial = chooseSocialUtilityDecision(
        friendlyWorld,
        friendlyWorld.characters[0],
        friendlyRelationships);

    assert(friendlySocial.intent == SocialIntent::Approach);
    assert(friendlySocial.target == 2);
    assert(friendlySocial.utility > 0.18);

    UnifiedUtilityDecision friendlyUnified = chooseUnifiedUtilityDecision(
        friendlyWorld,
        friendlyWorld.characters[0],
        friendlyRelationships);
    assert(friendlyUnified.kind == UnifiedDecisionKind::Social);
    assert(friendlyUnified.social.intent == SocialIntent::Approach);

    // Urgent physical needs still override ordinary social desire.
    friendlyWorld.objects.push_back(makeFridge());
    friendlyWorld.characters[0].civilization.inventory.add({
        ItemKind::RawMaterial, MaterialKind::PlantFood, 1, 0.5, 1.0});
    friendlyWorld.characters[0].needs.hunger = 0.98;
    UnifiedUtilityDecision hungryDecision = chooseUnifiedUtilityDecision(
        friendlyWorld,
        friendlyWorld.characters[0],
        friendlyRelationships);
    assert(hungryDecision.kind == UnifiedDecisionKind::Physical);
    assert(hungryDecision.physicalGoal == Goal::Eat);
    friendlyWorld.characters[0].needs.hunger = 0.0;

    // Critical hunger is settlement-local: remote storage must not create
    // an impossible retrieve trip, and known natural food chooses the nearest
    // real node rather than global insertion order.
    World survivalWorld(991122);
    survivalWorld.resourceNodes.clear();
    survivalWorld.storageSites.clear();
    survivalWorld.generatedNaturalChunks.clear();

    Character survivalSelf;
    survivalSelf.id = 11;
    survivalSelf.civilization.character = survivalSelf.id;
    survivalSelf.needs = {1.0, 0.10, 0.10, 0.10, 0.10};
    const GridPos survivalPos{0, 0};

    StorageSite remoteFood;
    remoteFood.id = 701;
    remoteFood.pos = {SettlementServiceRadiusGrid + 20, 0};
    remoteFood.inventory.add({
        ItemKind::RawMaterial, MaterialKind::PlantFood, 2, 0.5, 1.0});
    survivalWorld.storageSites.push_back(remoteFood);

    ResourceNode fartherFood;
    fartherFood.id = 801;
    fartherFood.material = MaterialKind::PlantFood;
    fartherFood.quantity = 4;
    fartherFood.maxQuantity = 4;
    fartherFood.pos = {WorldChunkSpanGridCells * 2, 0};
    survivalWorld.resourceNodes.push_back(fartherFood);

    ResourceNode nearerFood;
    nearerFood.id = 802;
    nearerFood.material = MaterialKind::PlantFood;
    nearerFood.quantity = 4;
    nearerFood.maxQuantity = 4;
    nearerFood.pos = {2, 0};
    survivalWorld.resourceNodes.push_back(nearerFood);

    CivilizationUtilityDecision localGather =
        urgentSurvivalProvisionDecisionAtPosition(
            survivalWorld, survivalSelf, survivalPos);
    assert(localGather.intent == CivilizationIntent::Gather);
    assert(localGather.resourceNode == nearerFood.id);
    assert(localGather.storage == 0);

    StorageSite localFood;
    localFood.id = 702;
    localFood.pos = {1, 0};
    localFood.inventory.add({
        ItemKind::RawMaterial, MaterialKind::PlantFood, 2, 0.5, 1.0});
    survivalWorld.storageSites.push_back(localFood);

    CivilizationUtilityDecision localRetrieve =
        urgentSurvivalProvisionDecisionAtPosition(
            survivalWorld, survivalSelf, survivalPos);
    assert(localRetrieve.intent == CivilizationIntent::Retrieve);
    assert(localRetrieve.storage == localFood.id);

    // Long-run depletion: the ordinary six-chunk frontier may already be
    // fully explored. Critical hunger must keep a survival frontier beyond
    // that envelope, and target resolution must agree with the decision.
    World exhaustedFrontierWorld(991123);
    exhaustedFrontierWorld.resourceNodes.clear();
    exhaustedFrontierWorld.storageSites.clear();
    exhaustedFrontierWorld.generatedNaturalChunks.clear();

    Character exhaustedSelf;
    exhaustedSelf.id=12;
    exhaustedSelf.civilization.character=exhaustedSelf.id;
    exhaustedSelf.needs={1.0,0.10,0.10,1.0,0.10};
    const GridPos exhaustedPos{0,0};
    const ChunkCoord exhaustedCenter=chunkCoordForGrid(exhaustedPos);

    for(int dx=-ResourceExplorationMaxRadiusChunks;
        dx<=ResourceExplorationMaxRadiusChunks;++dx){
        for(int dy=-ResourceExplorationMaxRadiusChunks;
            dy<=ResourceExplorationMaxRadiusChunks;++dy){
            GeneratedNaturalChunk generated;
            generated.coord={exhaustedCenter.x+dx,exhaustedCenter.y+dy};
            exhaustedFrontierWorld.generatedNaturalChunks.push_back(generated);
        }
    }
    std::sort(
        exhaustedFrontierWorld.generatedNaturalChunks.begin(),
        exhaustedFrontierWorld.generatedNaturalChunks.end(),
        [](const GeneratedNaturalChunk& a,const GeneratedNaturalChunk& b){
            return a.coord<b.coord;
        });

    const ResourceExplorationOpportunity ordinaryExhausted=
        chooseResourceExplorationOpportunity(
            exhaustedFrontierWorld,
            exhaustedSelf.id,
            MaterialKind::PlantFood,
            exhaustedPos);
    assert(!ordinaryExhausted.available);

    const CivilizationUtilityDecision criticalFrontier=
        urgentSurvivalProvisionDecisionAtPosition(
            exhaustedFrontierWorld,exhaustedSelf,exhaustedPos);
    assert(criticalFrontier.intent==CivilizationIntent::Explore);
    assert(criticalFrontier.material==MaterialKind::PlantFood);

    GridPos criticalTarget{};
    SanitationSiteId criticalSanitation=0;
    assert(resolveCivilizationContextTarget(
        exhaustedFrontierWorld,
        exhaustedSelf,
        criticalFrontier,
        exhaustedPos,
        criticalTarget,
        criticalSanitation));
    const ChunkCoord criticalChunk=chunkCoordForGrid(criticalTarget);
    assert(std::max(
        std::abs(criticalChunk.x-exhaustedCenter.x),
        std::abs(criticalChunk.y-exhaustedCenter.y))
        >ResourceExplorationMaxRadiusChunks);

    // The same escape path applies to thirst when the world has no known
    // freshwater node: explore for new water instead of falling through to a
    // lower-order physical action.
    exhaustedSelf.needs={0.10,1.0,0.10,1.0,0.10};
    const CivilizationUtilityDecision criticalWaterFrontier=
        urgentSurvivalProvisionDecisionAtPosition(
            exhaustedFrontierWorld,exhaustedSelf,exhaustedPos);
    assert(criticalWaterFrontier.intent==CivilizationIntent::Explore);
    assert(criticalWaterFrontier.material==MaterialKind::Water);

    // Repeated betrayal should shift the same resident toward avoiding that person.
    friendlyWorld.characters[0].personality.agreeableness = 0.10;
    friendlyWorld.characters[0].personality.empathy = 0.10;
    friendlyWorld.characters[0].personality.patience = 0.10;
    friendlyWorld.characters[0].personality.riskTolerance = 0.10;
    friendlyWorld.characters[0].personality.introversion = 0.75;

    for (int i = 0; i < 4; ++i) {
        SocialEvent betrayal;
        betrayal.actor = 2;
        betrayal.recipient = 1;
        betrayal.type = SocialEventType::Betrayal;
        betrayal.intensity = 1.0;
        betrayal.minute = 700 + i * 10;
        betrayal.where = "home";
        betrayal.source = MemorySource::DirectWitness;
        betrayal.witnessed = true;
        processSocialEvent(friendlyWorld.characters[0], friendlyRelationships, betrayal);
    }
    friendlyWorld.minute = 750;

    SocialUtilityDecision afterBetrayal = chooseSocialUtilityDecision(
        friendlyWorld,
        friendlyWorld.characters[0],
        friendlyRelationships);
    assert(afterBetrayal.target == 2);
    assert(afterBetrayal.intent == SocialIntent::Avoid);
    assert(scoreAvoidIntent(
        friendlyWorld,
        friendlyWorld.characters[0],
        2,
        friendlyRelationships) >
        scoreApproachIntent(
            friendlyWorld,
            friendlyWorld.characters[0],
            2,
            friendlyRelationships));

    UnifiedUtilityDecision avoidUnified = chooseUnifiedUtilityDecision(
        friendlyWorld,
        friendlyWorld.characters[0],
        friendlyRelationships);
    assert(avoidUnified.kind == UnifiedDecisionKind::Social);
    assert(avoidUnified.social.intent == SocialIntent::Avoid);

    // A bonded, empathetic character with conflict should prefer repair over avoidance.
    World repairWorld(11);
    Character repairer;
    repairer.id = 10;
    repairer.personality.sociability = 0.55;
    repairer.personality.introversion = 0.35;
    repairer.personality.agreeableness = 0.96;
    repairer.personality.empathy = 0.94;
    repairer.personality.patience = 0.95;
    repairer.personality.riskTolerance = 0.70;

    Character partner;
    partner.id = 11;
    repairWorld.characters = {repairer, partner};

    RelationshipBook repairRelationships;
    Relationship& repairRelation = repairRelationships.getOrCreate(10, 11);
    repairRelation.affection = 0.78;
    repairRelation.trust = 0.66;
    repairRelation.comfort = 0.70;
    repairRelation.familiarity = 0.85;
    repairRelation.conflict = 0.62;
    repairRelation.fear = 0.03;
    repairRelation.grudge = 0.04;

    SocialUtilityDecision repairDecision = chooseSocialUtilityDecision(
        repairWorld,
        repairWorld.characters[0],
        repairRelationships);
    assert(repairDecision.intent == SocialIntent::Repair);
    assert(repairDecision.target == 11);

    // Empathy + visible distress should make Comfort the best social response.
    World comfortWorld(19);
    Character comforter;
    comforter.id = 20;
    comforter.personality.empathy = 0.98;
    comforter.personality.agreeableness = 0.92;
    comforter.personality.sociability = 0.65;

    Character distressed;
    distressed.id = 21;
    distressed.emotion.sadness = 0.92;
    distressed.emotion.grief = 0.74;
    distressed.emotion.refreshSummary();

    comfortWorld.characters = {comforter, distressed};

    RelationshipBook comfortRelationships;
    Relationship& comfortRelation = comfortRelationships.getOrCreate(20, 21);
    comfortRelation.affection = 0.72;
    comfortRelation.trust = 0.70;
    comfortRelation.comfort = 0.68;
    comfortRelation.familiarity = 0.72;

    SocialUtilityDecision comfortDecision = chooseSocialUtilityDecision(
        comfortWorld,
        comfortWorld.characters[0],
        comfortRelationships);
    assert(comfortDecision.intent == SocialIntent::Comfort);
    assert(comfortDecision.target == 21);

    // A neutral stranger should not automatically force social interaction.
    World strangerWorld(23);
    Character quiet;
    quiet.id = 30;
    quiet.personality.sociability = 0.20;
    quiet.personality.curiosity = 0.20;
    quiet.personality.introversion = 0.85;
    Character stranger;
    stranger.id = 31;
    strangerWorld.characters = {quiet, stranger};
    RelationshipBook strangerRelationships;

    UnifiedUtilityDecision strangerDecision = chooseUnifiedUtilityDecision(
        strangerWorld,
        strangerWorld.characters[0],
        strangerRelationships);
    assert(strangerDecision.kind == UnifiedDecisionKind::Physical);
    assert(strangerDecision.physicalGoal == Goal::Idle);

    return 0;
}
