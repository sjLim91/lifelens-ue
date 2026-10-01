#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SocialUtility.h"

using namespace lifelens;

namespace {

void learnBaselineTechniques(Character& character)
{
    character.civilization.knowledge.learn(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::FireMaking,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible,0.92);
}

const EnvironmentalResidueRecord* residueAt(
    const EnvironmentalResidueField& field,
    GridPos pos)
{
    for(const auto& residue:field.all()){
        if(residue.kind==EnvironmentalResidueKind::HumanWaste
           && residue.pos.x==pos.x && residue.pos.y==pos.y) return &residue;
    }
    return nullptr;
}

Simulation makeUrgentToiletSimulation(std::uint64_t seed)
{
    Simulation simulation(seed);
    simulation.setupNewGame();
    simulation.setExternalPhysicalExecution(true);
    simulation.world().minute=481;
    for(auto& resident:simulation.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }
    simulation.world().characters.front().needs.bladder=0.99;
    simulation.runMinutes(5);
    return simulation;
}

Simulation makeDugPitSchedulerProbe(
    std::uint64_t seed,
    double hunger,
    double thirst,
    double bladder,
    double hygiene,
    bool knowsDugPit=true)
{
    Simulation simulation(seed);
    simulation.setupNewGame();
    simulation.setExternalPhysicalExecution(true);
    simulation.world().characters.resize(1);
    simulation.world().minute=480;

    Character& actor=simulation.world().characters.front();
    actor.needs={hunger,thirst,0.10,bladder,hygiene};
    actor.civilization.craftingSkill=1.0;
    actor.personality.conscientiousness=1.0;
    actor.personality.patience=1.0;
    actor.personality.orderliness=1.0;
    learnBaselineTechniques(actor);
    if(knowsDugPit){
        actor.civilization.knowledge.learn(
            TechniqueId::DugSanitationPit,
            KnowledgeLevel::Reproducible,
            0.95);
    }

    // Keep hygiene from creating a missing-Water acquisition task. The probe is
    // specifically about the scheduler gate between an immediately available
    // UseToilet action and the already-known DugPit improvement.
    actor.civilization.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Clay,1,0.5,1.0});
    actor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,1,0.5,1.0});

    GridPos actorPos{};
    assert(simulation.runtimePosition(actor.id,actorPos));
    simulation.world().primitiveSanitationSites.clear();

    PrimitiveSanitationSite site;
    site.id=1;
    site.kind=PrimitiveSanitationSiteKind::DesignatedArea;
    site.pos=actorPos;
    site.establishedBy=actor.id;
    site.establishedMinute=0;
    site.active=true;
    site.useCount=2;
    simulation.world().primitiveSanitationSites.push_back(site);
    return simulation;
}

} // namespace

int main()
{
    // A designated sanitation area must exist before the dug-pit technique can
    // even be investigated. Mere designated-area knowledge is not enough.
    World world(9201);
    world.minute=720;
    Character builder;
    builder.id=11;
    builder.civilization.character=builder.id;
    builder.civilization.craftingSkill=0.80;
    builder.civilization.learningSkill=0.92;
    builder.personality.curiosity=0.95;
    builder.personality.openness=0.90;
    builder.personality.patience=0.90;
    builder.personality.conscientiousness=0.88;
    builder.personality.orderliness=0.84;
    learnBaselineTechniques(builder);

    assert(bestExperimentDecision(world,builder).intent==CivilizationIntent::None);

    const PrimitiveSanitationSiteCreationResult created=
        establishDesignatedSanitationArea(
            world.seed,builder,world.environmentalResidues,
            world.primitiveSanitationSites,world.minute);
    assert(created.established);
    assert(world.primitiveSanitationSites.size()==1);
    assert(world.primitiveSanitationSites.front().kind==PrimitiveSanitationSiteKind::DesignatedArea);

    // A brand-new clean designated area does not automatically imply that the
    // resident has recognized the value of digging a pit. Repeated actual use
    // is enough evidence to make the improvement experiment meaningful.
    assert(!evaluateDugSanitationPitOpportunity(
        builder,world.environmentalResidues,world.primitiveSanitationSites).candidateAvailable);
    assert(recordPrimitiveSanitationSiteUse(
        world.primitiveSanitationSites,created.siteId,created.pos));
    assert(recordPrimitiveSanitationSiteUse(
        world.primitiveSanitationSites,created.siteId,created.pos));
    assert(evaluateDugSanitationPitOpportunity(
        builder,world.environmentalResidues,world.primitiveSanitationSites).candidateAvailable);

    // Chronic sanitation pressure must not close the civilization gate that is
    // required to discover the sanitation improvement itself. This is only a
    // competition exception: the DugPit experiment still has to beat the
    // currently available Physical action on utility.
    Character pressureBuilder=builder;
    pressureBuilder.needs={0.10,0.10,0.20,0.75,0.75};
    RelationshipBook noRelationships;
    const CivilizationUtilityDecision pressureExperiment=
        chooseDispositionAwareCivilizationDecisionAtPosition(
            world,pressureBuilder,created.pos,nullptr);
    assert(pressureExperiment.intent==CivilizationIntent::Experiment);
    assert(pressureExperiment.technique==TechniqueId::DugSanitationPit);

    const UnifiedUtilityDecision pressureDecision=
        chooseUnifiedUtilityDecisionAtPosition(
            world,pressureBuilder,noRelationships,created.pos,0.18,0.14,nullptr);
    assert(pressureDecision.kind==UnifiedDecisionKind::Civilization);
    assert(pressureDecision.civilization.technique==TechniqueId::DugSanitationPit);

    // The sanitation evaluator must remain available independently from the
    // overall civilization winner.
    const CivilizationUtilityDecision independentSanitation=
        chooseDispositionAwareSanitationProgressionDecisionAtPosition(
            world,pressureBuilder,created.pos,nullptr);
    assert(independentSanitation.intent==CivilizationIntent::Experiment);
    assert(independentSanitation.technique==TechniqueId::DugSanitationPit);

    // Reproduce the exact candidate-shadowing contract without depending on a
    // fragile world-scoring coincidence: an unrelated civilization candidate
    // is stronger than the sanitation candidate, but urgent bladder/hygiene
    // pressure suppresses ordinary civilization. The independently preserved
    // DugPit candidate must still be allowed to compete with Physical/Social.
    CivilizationUtilityDecision shadowingCivilization;
    shadowingCivilization.intent=CivilizationIntent::Experiment;
    shadowingCivilization.technique=TechniqueId::PrimitiveStorage;
    shadowingCivilization.utility=0.95;

    CivilizationUtilityDecision shadowedSanitation;
    shadowedSanitation.intent=CivilizationIntent::Experiment;
    shadowedSanitation.technique=TechniqueId::DugSanitationPit;
    shadowedSanitation.utility=0.75;

    UnifiedUtilityDecision shadowedPressureDecision;
    shadowedPressureDecision.kind=UnifiedDecisionKind::Physical;
    shadowedPressureDecision.physicalGoal=Goal::UseToilet;
    shadowedPressureDecision.utility=0.60;
    considerCivilizationUnderNeedPressure(
        pressureBuilder,
        shadowingCivilization,
        shadowedSanitation,
        0.14,
        shadowedPressureDecision);
    assert(shadowedPressureDecision.kind==UnifiedDecisionKind::Civilization);
    assert(shadowedPressureDecision.civilization.technique
        ==TechniqueId::DugSanitationPit);

    // The exception closes as soon as food survival is also urgent. Carried
    // food keeps this focused on the gate rather than missing-provision search.
    pressureBuilder.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.8,1.0});
    pressureBuilder.needs.hunger=0.80;
    const UnifiedUtilityDecision hungryPressureDecision=
        chooseUnifiedUtilityDecisionAtPosition(
            world,pressureBuilder,noRelationships,created.pos,0.18,0.14,nullptr);
    assert(hungryPressureDecision.kind!=UnifiedDecisionKind::Civilization
        || hungryPressureDecision.civilization.technique
            !=TechniqueId::DugSanitationPit);

    // The scheduler exception must not perturb the discovery/experiment phase.
    // Pressure alone is insufficient; the resident must already know DugPit
    // reproducibly and have a DesignatedArea that can be improved.
    Simulation preDiscoverySchedulerProbe=makeDugPitSchedulerProbe(
        9201,0.10,0.10,0.75,0.75,false);
    Character& preDiscoveryActor=
        preDiscoverySchedulerProbe.world().characters.front();
    assert(sanitationProgressionPressureCouldCompete(preDiscoveryActor));
    assert(!dugSanitationPitCraftPressureCouldCompete(
        preDiscoverySchedulerProbe.world(),preDiscoveryActor));

    // Production scheduler regression: once the pit technique is reproducible,
    // urgent-but-noncritical sanitation pressure must reach the same Unified
    // Utility competition tested above. Previously beginPlan saw UseToilet at
    // the 0.70 urgent band and skipped civilization entirely, making the
    // sanitation-pressure exception unreachable from the real runtime.
    Simulation schedulerProbe=makeDugPitSchedulerProbe(
        9202,0.10,0.10,0.75,0.75);
    const CharacterId schedulerActorId=
        schedulerProbe.world().characters.front().id;
    schedulerProbe.step();
    const PendingContextActionObservation schedulerPending=
        schedulerProbe.observePendingContextAction(schedulerActorId);
    assert(schedulerPending.active);
    assert(schedulerPending.kind==ContextActionKind::Civilization);
    assert(schedulerPending.civilizationIntent==CivilizationIntent::Craft);
    assert(schedulerPending.technique==TechniqueId::DugSanitationPit);

    // The narrow scheduler opening must not bypass food survival pressure.
    Simulation hungrySchedulerProbe=makeDugPitSchedulerProbe(
        9203,0.80,0.10,0.75,0.75);
    const CharacterId hungrySchedulerActorId=
        hungrySchedulerProbe.world().characters.front().id;
    hungrySchedulerProbe.step();
    const PendingContextActionObservation hungrySchedulerPending=
        hungrySchedulerProbe.observePendingContextAction(
            hungrySchedulerActorId);
    assert(!hungrySchedulerPending.active
        || hungrySchedulerPending.technique!=TechniqueId::DugSanitationPit);

    // Nor does the new opening force construction when immediate bladder relief
    // is overwhelmingly stronger. Unified Utility keeps its existing 1.08
    // margin, so the Physical action still wins this state.
    Simulation severeBladderProbe=makeDugPitSchedulerProbe(
        9204,0.10,0.10,0.99,0.75);
    const CharacterId severeBladderActorId=
        severeBladderProbe.world().characters.front().id;
    severeBladderProbe.step();
    const PendingContextActionObservation severeBladderPending=
        severeBladderProbe.observePendingContextAction(
            severeBladderActorId);
    assert(!severeBladderPending.active
        || severeBladderPending.technique!=TechniqueId::DugSanitationPit);
    const ResidentPresentationObservation severeBladderPresentation=
        severeBladderProbe.observeResidentPresentation(severeBladderActorId);
    assert(severeBladderPresentation.active);
    assert(severeBladderPresentation.kind==PresentationActionKind::Physical);
    assert(severeBladderPresentation.physicalGoal==Goal::UseToilet);

    // Before discovery, Craft must not silently improve the facility.
    const CivilizationUtilityDecision beforeDiscovery=bestCraftDecision(world,builder);
    assert(beforeDiscovery.technique!=TechniqueId::DugSanitationPit);
    assert(world.primitiveSanitationSites.front().kind==PrimitiveSanitationSiteKind::DesignatedArea);
    assert(world.primitiveSanitationSites.front().improvementWork==0.0);

    // With all other experiment families already known, the autonomous
    // Experiment axis selects DugSanitationPit. Deterministic attempts may fail
    // first and leave a Hypothesized state, but eventually one succeeds.
    bool discovered=false;
    for(int attempt=0;attempt<64 && !discovered;++attempt){
        const CivilizationUtilityDecision experiment=bestExperimentDecision(world,builder);
        assert(experiment.intent==CivilizationIntent::Experiment);
        assert(experiment.experiment==ExperimentKind::DigSanitationPit);
        assert(experiment.technique==TechniqueId::DugSanitationPit);
        const CivilizationExecutionResult result=
            executeCivilizationDecision(world,builder,experiment);
        assert(result.executed);
        discovered=result.success;
        if(!discovered) world.minute+=15;
    }
    assert(discovered);
    assert(builder.civilization.knowledge.knowsAtLeast(
        TechniqueId::DugSanitationPit,KnowledgeLevel::Reproducible));
    assert(world.primitiveSanitationSites.size()==1);
    assert(world.primitiveSanitationSites.front().kind==PrimitiveSanitationSiteKind::DesignatedArea);

    // Put realistic surface waste at the same authoritative cell before the
    // excavation finishes. Containment must reduce exposure without deleting
    // the waste mass.
    const auto& surfaceWaste=world.environmentalResidues.deposit(
        EnvironmentalResidueKind::HumanWaste,created.pos,builder.id,
        world.minute,3.0,0.80,3);
    const EnvironmentalResidueId wasteId=surfaceWaste.id;
    const double amountBefore=surfaceWaste.amount;
    const double exposureBefore=world.environmentalResidues.exposureAt(created.pos);
    assert(exposureBefore>=0.79);

    bool completed=false;
    int workActions=0;
    while(!completed && workActions<10){
        const CivilizationUtilityDecision work=bestCraftDecision(world,builder);
        assert(work.intent==CivilizationIntent::Craft);
        assert(work.technique==TechniqueId::DugSanitationPit);
        const CivilizationExecutionResult result=
            executeCivilizationDecision(world,builder,work);
        assert(result.executed);
        assert(result.success);
        assert(result.sanitationSiteId==created.siteId);
        assert(result.sanitationSitePos.x==created.pos.x);
        assert(result.sanitationSitePos.y==created.pos.y);
        assert(result.sanitationWorkAfter>result.sanitationWorkBefore);
        completed=result.sanitationImprovementCompleted;
        ++workActions;
        if(!completed){
            assert(world.primitiveSanitationSites.front().kind==PrimitiveSanitationSiteKind::DesignatedArea);
        }
    }
    assert(completed);
    assert(workActions>=2);
    assert(world.primitiveSanitationSites.size()==1);

    const PrimitiveSanitationSite& pit=world.primitiveSanitationSites.front();
    assert(pit.id==created.siteId);
    assert(pit.pos.x==created.pos.x && pit.pos.y==created.pos.y);
    assert(pit.kind==PrimitiveSanitationSiteKind::DugPit);
    assert(pit.improvementWork==DugSanitationPitWorkRequired);
    assert(pit.improvedBy==builder.id);
    assert(pit.improvedMinute==world.minute);

    const EnvironmentalResidueRecord* contained=residueAt(world.environmentalResidues,created.pos);
    assert(contained!=nullptr);
    assert(contained->id==wasteId);
    assert(std::abs(contained->amount-amountBefore)<1e-9);
    assert(contained->radiusTiles==DugSanitationPitContainmentRadiusTiles);
    assert(contained->intensity<0.80);
    assert(world.environmentalResidues.exposureAt(created.pos)<exposureBefore);

    // Runtime physical execution keeps the same site identity/GridPos and
    // deposits future waste with the DugPit containment profile.
    Simulation simulation=makeUrgentToiletSimulation(9301);
    Character& actor=simulation.world().characters.front();
    const CharacterId actorId=actor.id;
    learnBaselineTechniques(actor);
    actor.civilization.knowledge.learn(
        TechniqueId::DugSanitationPit,KnowledgeLevel::Reproducible,0.94);

    const PrimitiveSanitationSiteCreationResult runtimeCreated=
        establishDesignatedSanitationArea(
            simulation.world().seed,actor,
            simulation.world().environmentalResidues,
            simulation.world().primitiveSanitationSites,
            simulation.world().minute);
    assert(runtimeCreated.established);

    bool runtimeCompleted=false;
    for(int i=0;i<10 && !runtimeCompleted;++i){
        const DugSanitationPitWorkResult work=workOnDugSanitationPit(
            actor,simulation.world().environmentalResidues,
            simulation.world().primitiveSanitationSites,
            simulation.world().minute);
        assert(work.worked);
        runtimeCompleted=work.completed;
    }
    assert(runtimeCompleted);
    assert(simulation.world().primitiveSanitationSites.size()==1);
    const PrimitiveSanitationSite& runtimePit=simulation.world().primitiveSanitationSites.front();
    assert(runtimePit.id==runtimeCreated.siteId);
    assert(runtimePit.kind==PrimitiveSanitationSiteKind::DugPit);

    SanitationUseTarget target;
    assert(simulation.sanitationUseTarget(actorId,target));
    assert(target.kind==SanitationUseTargetKind::DesignatedArea);
    assert(target.siteId==runtimeCreated.siteId);
    assert(target.pos.x==runtimeCreated.pos.x && target.pos.y==runtimeCreated.pos.y);

    const double bladderBefore=actor.needs.bladder;
    assert(simulation.completeExternalPhysicalAction(
        actorId,false,target.pos,target.siteId));
    assert(actor.needs.bladder<bladderBefore);

    const EnvironmentalResidueRecord* runtimeResidue=
        residueAt(simulation.world().environmentalResidues,target.pos);
    assert(runtimeResidue!=nullptr);
    assert(runtimeResidue->radiusTiles==primitiveSanitationResidueRadiusTiles(
        PrimitiveSanitationSiteKind::DugPit));
    assert(std::abs(runtimeResidue->intensity-
        primitiveSanitationResidueIntensity(PrimitiveSanitationSiteKind::DugPit))<1e-9);
    assert(simulation.world().primitiveSanitationSites.front().useCount==1);

    // The outer snapshot format may advance as new authoritative extensions
    // are added. Roundtrip must preserve the same facility identity and
    // completed improvement state.
    const SimulationStateSnapshot captured=simulation.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(captured,bytes,&error));
    assert(error.empty());
    assert(bytes.size()>12);
    assert(bytes[8]==static_cast<std::uint8_t>(SimulationSnapshotBinaryFormatVersion)
        && bytes[9]==0 && bytes[10]==0 && bytes[11]==0);

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());
    assert(decoded.world.primitiveSanitationSites.size()==1);
    const PrimitiveSanitationSite& decodedPit=decoded.world.primitiveSanitationSites.front();
    assert(decodedPit.id==runtimeCreated.siteId);
    assert(decodedPit.kind==PrimitiveSanitationSiteKind::DugPit);
    assert(decodedPit.pos.x==runtimeCreated.pos.x && decodedPit.pos.y==runtimeCreated.pos.y);
    assert(decodedPit.improvementWork==DugSanitationPitWorkRequired);
    assert(decodedPit.improvedBy==actorId);
    assert(decodedPit.improvedMinute==simulation.world().minute);
    assert(decodedPit.useCount==1);

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    assert(error.empty());
    assert(restored.world().primitiveSanitationSites.size()==1);
    const PrimitiveSanitationSite& restoredPit=restored.world().primitiveSanitationSites.front();
    assert(restoredPit.id==runtimeCreated.siteId);
    assert(restoredPit.kind==PrimitiveSanitationSiteKind::DugPit);
    assert(restoredPit.improvedBy==actorId);
    assert(restoredPit.useCount==1);

    return 0;
}
