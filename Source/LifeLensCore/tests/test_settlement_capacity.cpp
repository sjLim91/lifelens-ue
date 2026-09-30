#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static ConstructedFacility completedFixture(FacilityId id, FacilityKind kind, GridPos pos, CharacterId owner, int minute)
{
    auto facility=makeFacilityConstructionSite(id,kind,pos,owner,minute);
    for(auto& requirement:facility.requirements) requirement.delivered=requirement.required;
    facility.constructionWork=facility.requiredWork;
    assert(activateConstructedFacility(facility,0,minute));
    return facility;
}

int main()
{
    Simulation simulation(770031);
    simulation.setupNewGame();
    auto& world=simulation.world();
    assert(world.facilities.empty());
    const GridPos anchor=world.initialStartRegionCenterGrid();
    SettlementPopulation population;
    for(auto& resident:world.characters){
        population.emplace(resident.id,anchor);
        resident.needs={0.1,0.1,0.58,0.1,0.1};
    }
    auto& actor=world.characters.front();
    const CharacterId owner=actor.id;
    const auto demand=[&](FacilityKind kind,GridPos pos){
        return observeSettlementFacilityDemand(world,owner,kind,pos,&population);
    };
    assert(demand(FacilityKind::SleepingPlace,anchor).residents==4);
    assert(demand(FacilityKind::SleepingPlace,anchor).operationalCapacity==0);

    // Four real residents build four beds in sequence, with paid material/work.
    // A completed first bed must neither hide later projects nor end all demand.
    for(int built=0;built<4;++built){
        const auto site=chooseSettlementFacilitySite(world,owner,FacilityKind::SleepingPlace,anchor,&population);
        if(!site.available){
            std::cerr << "No site after " << built << " beds\n";
            return 1;
        }
        for(const auto& existing:world.facilities)
            assert(!facilityFootprintsConflict(FacilityKind::SleepingPlace,site.pos,existing.kind,existing.pos));
        auto* project=establishSettlementFacilityProject(world,owner,FacilityKind::SleepingPlace,site.pos,&population);
        assert(project && !project->active && project->constructionWork==0.0);
        const FacilityId id=project->id;
        assert(demand(FacilityKind::SleepingPlace,anchor).operationalCapacity==built);
        assert(!chooseSettlementFacilitySite(world,world.characters[1].id,FacilityKind::SleepingPlace,anchor,&population).available);
        assert(!establishSettlementFacilityProject(world,world.characters[1].id,FacilityKind::SleepingPlace,
            {site.pos.x+20,site.pos.y},&population));
        assert(!workOnSettlementFacility(world,actor,id,100.0).worked);
        const auto requirements=project->requirements;
        for(const auto& requirement:requirements){
            actor.civilization.inventory.add({ItemKind::RawMaterial,requirement.material,requirement.required,0.5,1.0});
            const int before=actor.civilization.inventory.count(ItemKind::RawMaterial,requirement.material);
            assert(deliverFacilityMaterial(*project,actor.civilization.inventory,requirement.material,requirement.required)==requirement.required);
            assert(actor.civilization.inventory.count(ItemKind::RawMaterial,requirement.material)==before-requirement.required);
        }
        const auto next=bestSettlementFoundationDecision(world,actor,anchor,&population);
        assert(next.facility==id && next.facilityAction==FacilityBuildAction::Work);
        assert(workOnSettlementFacility(world,actor,id,100.0).completed);
        assert(facilityOperationalAndActive(*project));
    }
    assert(!demand(FacilityKind::SleepingPlace,anchor).unmet());
    assert(!chooseSettlementFacilitySite(world,owner,FacilityKind::SleepingPlace,anchor,&population).available);
    assert(settlementFacilityNeedPressure(world,actor,anchor,FacilityKind::SleepingPlace,&population)==0.0);

    // A worn-out bed reuses its surviving frame before anyone clears land for
    // a fifth replacement. Restoration costs are derived from the original
    // construction package and are strictly cheaper (Fiber 2 + Wood 1 here,
    // versus Fiber 4 + Wood 2 for a new SleepingPlace).
    const std::size_t facilityCountBeforeRestoration=world.facilities.size();
    const FacilityId ruinedBedId=world.facilities.front().id;
    assert(ruinConstructedFacility(world.facilities.front()));
    assert(demand(FacilityKind::SleepingPlace,anchor).unmet());
    assert(facilityRestorationMaterialRequirement(
        FacilityKind::SleepingPlace,MaterialKind::Fiber)==2);
    assert(facilityRestorationMaterialRequirement(
        FacilityKind::SleepingPlace,MaterialKind::Wood)==1);
    assert(settlementRepairMaterialDemand(world,MaterialKind::Fiber)>=2);
    assert(settlementRepairMaterialDemand(world,MaterialKind::Wood)>=1);

    for(const MaterialKind material:{MaterialKind::Fiber,MaterialKind::Wood}){
        const int held=actor.civilization.inventory.count(
            ItemKind::RawMaterial,material);
        if(held>0){
            assert(actor.civilization.inventory.remove(
                ItemKind::RawMaterial,material,held));
        }
    }

    const auto withoutRepairMaterials=
        bestSettlementFoundationDecision(
            world,actor,anchor,&population);
    assert(!(withoutRepairMaterials.facilityKind==FacilityKind::SleepingPlace
        && withoutRepairMaterials.facilityAction==FacilityBuildAction::Plan));

    actor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Fiber,2,0.5,1.0});
    actor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});
    const int fiberBeforeRestore=actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Fiber);
    const int woodBeforeRestore=actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood);

    const auto restoreBed=bestSettlementFoundationDecision(
        world,actor,anchor,&population);
    assert(restoreBed.facilityAction==FacilityBuildAction::Repair);
    assert(restoreBed.facilityKind==FacilityKind::SleepingPlace);
    assert(restoreBed.facility==ruinedBedId);
    const auto restored=executeCivilizationDecisionAtPosition(
        world,actor,restoreBed,world.facilities.front().pos,&population);
    assert(restored.executed && restored.success);
    assert(world.facilities.size()==facilityCountBeforeRestoration);
    assert(world.facilities.front().state==FacilityState::Operational);
    assert(world.facilities.front().active);
    assert(world.facilities.front().durability>=0.52);
    assert(world.facilities.front().durability<=0.70);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Fiber)==fiberBeforeRestore-2);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==woodBeforeRestore-1);
    assert(!demand(FacilityKind::SleepingPlace,anchor).unmet());
    assert(!chooseSettlementFacilitySite(
        world,owner,FacilityKind::SleepingPlace,anchor,&population).available);

    // Lived facility-use history is authoritative and must survive the same
    // snapshot path used by the settlement itself.
    assert(!world.facilities.empty());
    for(int use=0;use<3;++use){
        assert(recordFacilityUse(
            world.facilities.front(),owner,world.minute+use));
    }
    const int savedUsageCount=world.facilities.front().usageCount;
    const int savedLastUsedMinute=world.facilities.front().lastUsedMinute;

    // Multiple facilities remain authoritative through the real save codec.
    std::vector<std::uint8_t> bytes;
    std::string error;
    if(!encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error)){
        std::cerr << "Snapshot encoding failed: " << error << '\n';
        return 1;
    }
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(decoded.world.facilities.size()==world.facilities.size());
    for(std::size_t index=0;index<world.facilities.size();++index){
        assert(decoded.world.facilities[index].id==world.facilities[index].id);
        assert(decoded.world.facilities[index].state==world.facilities[index].state);
        assert(manhattan(decoded.world.facilities[index].pos,world.facilities[index].pos)==0);
    }
    assert(decoded.world.facilities.front().usageCount==savedUsageCount);
    assert(decoded.world.facilities.front().lastUsedMinute==savedLastUsedMinute);

    // Birth/population growth changes demand; death and leaving the area do not
    // leave phantom residents. Distant infrastructure does not serve this camp.
    Character newcomer=actor;
    newcomer.id=999;
    newcomer.civilization.character=newcomer.id;
    newcomer.lifeStage=LifeStage::Baby;
    world.characters.push_back(newcomer);
    population.emplace(newcomer.id,anchor);
    assert(demand(FacilityKind::SleepingPlace,anchor).canPlan());
    assert(demand(FacilityKind::Shelter,anchor).residents==5);
    assert(demand(FacilityKind::WorkSurface,anchor).residents==4);
    world.characters.back().alive=false;
    assert(!demand(FacilityKind::SleepingPlace,anchor).canPlan());
    const GridPos distant{anchor.x+SettlementServiceRadiusGrid*4,anchor.y};
    for(auto& location:population) location.second=distant;
    assert(demand(FacilityKind::SleepingPlace,anchor).residents==0);
    assert(demand(FacilityKind::SleepingPlace,distant).operationalCapacity==0);
    assert(demand(FacilityKind::SleepingPlace,distant).canPlan());
    // Missing authoritative positions fail closed rather than relocate people.
    population.erase(world.characters[1].id);
    assert(demand(FacilityKind::SleepingPlace,distant).residents==3);

    // Local shelters and workshops have shared, finite planning capacity.
    world.facilities.clear();
    for(auto& location:population) location.second=anchor;
    population.emplace(world.characters[1].id,anchor);
    world.facilities.push_back(completedFixture(100,FacilityKind::Shelter,anchor,owner,world.minute));
    assert(!demand(FacilityKind::Shelter,anchor).unmet());
    world.characters.back().alive=true;
    population[999]=anchor;
    assert(demand(FacilityKind::Shelter,anchor).canPlan());
    // A roof does not manufacture dedicated bedding.
    assert(demand(FacilityKind::SleepingPlace,anchor).canPlan());
    world.facilities.push_back(completedFixture(101,FacilityKind::WorkSurface,{anchor.x+20,anchor.y},owner,world.minute));
    assert(demand(FacilityKind::WorkSurface,anchor).canPlan());
    world.facilities.push_back(completedFixture(102,FacilityKind::WorkSurface,{anchor.x-20,anchor.y},owner,world.minute));
    assert(!demand(FacilityKind::WorkSurface,anchor).canPlan());

    // Maintenance searches all local facilities; a healthy first entry cannot
    // conceal a damaged second one. Ruins don't provide service capacity.
    for(auto& resident:world.characters) resident.needs={0.1,0.1,0.1,0.1,0.1};
    world.facilities.back().durability=0.4;
    auto& maintainer=world.characters.front();
    maintainer.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    const auto repair=bestSettlementFoundationDecision(world,maintainer,anchor,&population);
    assert(repair.facilityAction==FacilityBuildAction::Repair && repair.facility==102);
    assert(executeCivilizationDecisionAtPosition(world,maintainer,repair,world.facilities.back().pos,&population).success);
    assert(ruinConstructedFacility(world.facilities.back()));
    assert(demand(FacilityKind::WorkSurface,anchor).canPlan());

    // Exercise the actual Simulation -> unified utility -> pending context
    // path. With one existing bed and four residents, a second plan must exist.
    Simulation live(770031);
    live.setupNewGame();
    live.setExternalPhysicalExecution(true);
    auto& liveWorld=live.world();
    GridPos liveAnchor{};
    assert(live.runtimePosition(liveWorld.characters.front().id,liveAnchor));
    liveWorld.facilities.push_back(completedFixture(1,FacilityKind::SleepingPlace,
        {liveAnchor.x+14,liveAnchor.y},liveWorld.characters.front().id,liveWorld.minute));
    for(auto& resident:liveWorld.characters){
        resident.needs={0.1,0.1,0.58,0.1,0.1};
        resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Water,3,0.5,1.0});
        resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.5,1.0});
    }
    bool additionalPlan=false;
    for(int minute=0;minute<31 && !additionalPlan;++minute){
        live.step();
        for(const auto& resident:liveWorld.characters){
            const auto pending=live.observePendingContextAction(resident.id);
            if(pending.active && pending.facilityKind==FacilityKind::SleepingPlace
               && pending.facilityAction==FacilityBuildAction::Plan){
                additionalPlan=true;
                break;
            }
        }
    }
    assert(additionalPlan);

    // DU-01: sleep facilities are capacity-aware at actual use time.
    // Two one-person beds must not attract two simultaneous sleepers to the
    // same nearest bed while the other bed remains empty.
    SimulationRuleset reservationRules=DefaultSimulationRuleset;
    reservationRules.needs.hungerPerMinute=0.0;
    reservationRules.needs.thirstPerMinute=0.0;
    reservationRules.needs.bladderPerMinute=0.0;
    reservationRules.needs.hygienePerMinute=0.0;
    Simulation reservedSleep(
        770033,0,CurrentWorldGenerationVersion,reservationRules);
    reservedSleep.setupNewGame();
    reservedSleep.world().minute=22*60;

    auto& reservedWorld=reservedSleep.world();
    const CharacterId firstSleeperId=reservedWorld.characters[0].id;
    const CharacterId secondSleeperId=reservedWorld.characters[1].id;
    for(std::size_t i=2;i<reservedWorld.characters.size();++i){
        reservedWorld.characters[i].alive=false;
        reservedWorld.characters[i].deathMinute=reservedWorld.minute;
    }
    for(std::size_t i=0;i<2;++i){
        reservedWorld.characters[i].needs={0.01,0.01,0.95,0.01,0.01};
        reservedWorld.characters[i].sleepTendency=1.0;
    }

    GridPos reservationAnchor{};
    assert(reservedSleep.runtimePosition(
        firstSleeperId,reservationAnchor));
    const GridPos firstBedPos{
        reservationAnchor.x+6,reservationAnchor.y};
    const GridPos secondBedPos{
        reservationAnchor.x-6,reservationAnchor.y};
    reservedWorld.facilities.push_back(completedFixture(
        99201,FacilityKind::SleepingPlace,firstBedPos,
        firstSleeperId,reservedWorld.minute));
    reservedWorld.facilities.push_back(completedFixture(
        99202,FacilityKind::SleepingPlace,secondBedPos,
        secondSleeperId,reservedWorld.minute));

    bool sawConcurrentSleepTargets=false;
    GridPos firstTarget{};
    GridPos secondTarget{};
    for(int minute=0;minute<240 && !sawConcurrentSleepTargets;++minute){
        reservedSleep.step();
        const auto firstPresentation=
            reservedSleep.observeResidentPresentation(firstSleeperId);
        const auto secondPresentation=
            reservedSleep.observeResidentPresentation(secondSleeperId);
        if(firstPresentation.active
           && secondPresentation.active
           && firstPresentation.kind==PresentationActionKind::Physical
           && secondPresentation.kind==PresentationActionKind::Physical
           && firstPresentation.physicalGoal==Goal::Sleep
           && secondPresentation.physicalGoal==Goal::Sleep
           && firstPresentation.hasTargetGrid
           && secondPresentation.hasTargetGrid){
            firstTarget=firstPresentation.targetGrid;
            secondTarget=secondPresentation.targetGrid;
            sawConcurrentSleepTargets=true;
        }
    }
    assert(sawConcurrentSleepTargets);
    assert(!sameGridPos(firstTarget,secondTarget));
    const bool firstKnownBed=
        sameGridPos(firstTarget,firstBedPos)
        || sameGridPos(firstTarget,secondBedPos);
    const bool secondKnownBed=
        sameGridPos(secondTarget,firstBedPos)
        || sameGridPos(secondTarget,secondBedPos);
    assert(firstKnownBed && secondKnownBed);

    // Sleep is continuous time, not an atomic "rest completed" effect. A tired
    // resident walks to real bedding first, gains no sleep recovery in transit,
    // then recovers minute-by-minute until the rested threshold is reached.
    SimulationRuleset sleepRules=DefaultSimulationRuleset;
    sleepRules.needs.hungerPerMinute=0.0;
    sleepRules.needs.thirstPerMinute=0.0;
    sleepRules.needs.bladderPerMinute=0.0;
    sleepRules.needs.hygienePerMinute=0.0;
    Simulation timedSleep(
        770032,0,CurrentWorldGenerationVersion,sleepRules);
    timedSleep.setupNewGame();
    timedSleep.world().minute=22*60;
    Character& timedSleeper=timedSleep.world().characters.front();
    const CharacterId timedSleeperId=timedSleeper.id;
    for(std::size_t i=1;i<timedSleep.world().characters.size();++i){
        timedSleep.world().characters[i].alive=false;
        timedSleep.world().characters[i].deathMinute=timedSleep.world().minute;
    }
    timedSleeper.needs={0.01,0.01,0.95,0.01,0.01};
    timedSleeper.sleepTendency=1.0;

    GridPos timedStart{};
    assert(timedSleep.runtimePosition(timedSleeperId,timedStart));
    const GridPos timedBedPos{timedStart.x+8,timedStart.y};
    ConstructedFacility timedBed=completedFixture(
        99101,FacilityKind::SleepingPlace,timedBedPos,
        timedSleeperId,timedSleep.world().minute);
    timedSleep.world().facilities.push_back(timedBed);

    const double outdoorRecovery=sleepRecoveryPerMinuteAt(
        timedSleep.world(),timedStart,nullptr);
    const double bedRecovery=sleepRecoveryPerMinuteAt(
        timedSleep.world(),timedBedPos,&timedSleep.world().facilities.back());
    assert(bedRecovery>outdoorRecovery);

    // Compare planned duration below the hard session cap. Extreme fatigue can
    // legitimately give both plans the same 10-hour cap, while bedding still
    // reaches the rested threshold earlier through higher per-minute recovery.
    Character durationProbe=timedSleeper;
    durationProbe.needs.sleep=0.62;
    durationProbe.sleepTendency=1.0;
    const int outdoorMinutes=sleepDurationMinutesForNeed(
        durationProbe,outdoorRecovery,sleepRules.needs);
    const int bedMinutes=sleepDurationMinutesForNeed(
        durationProbe,bedRecovery,sleepRules.needs);
    assert(bedMinutes<outdoorMinutes);
    assert(bedMinutes>=MinimumSleepSessionMinutes);

    bool startedSleepTravel=false;
    double fatigueDuringTravel=timedSleeper.needs.sleep;
    for(int minute=0;minute<180 && !startedSleepTravel;++minute){
        timedSleep.step();
        const ResidentPresentationObservation presentation=
            timedSleep.observeResidentPresentation(timedSleeperId);
        if(!presentation.active
           || presentation.kind!=PresentationActionKind::Physical
           || presentation.physicalGoal!=Goal::Sleep){
            continue;
        }
        assert(presentation.phase==PresentationActionPhase::Moving);
        assert(presentation.hasTargetGrid);
        assert(presentation.targetGrid.x==timedBedPos.x);
        assert(presentation.targetGrid.y==timedBedPos.y);
        fatigueDuringTravel=timedSleeper.needs.sleep;
        startedSleepTravel=true;
    }
    assert(startedSleepTravel);

    // At least one further travel minute must not provide rest.
    timedSleep.step();
    const ResidentPresentationObservation continuingTravel=
        timedSleep.observeResidentPresentation(timedSleeperId);
    if(continuingTravel.active
       && continuingTravel.kind==PresentationActionKind::Physical
       && continuingTravel.physicalGoal==Goal::Sleep
       && continuingTravel.phase==PresentationActionPhase::Moving){
        assert(timedSleeper.needs.sleep>=fatigueDuringTravel);
    }

    bool reachedBed=false;
    for(int minute=0;minute<120 && !reachedBed;++minute){
        timedSleep.step();
        const auto presentation=
            timedSleep.observeResidentPresentation(timedSleeperId);
        reachedBed=
            presentation.active
            && presentation.kind==PresentationActionKind::Physical
            && presentation.physicalGoal==Goal::Sleep
            && presentation.phase==PresentationActionPhase::Interacting;
    }
    assert(reachedBed);
    GridPos reachedPosition{};
    assert(timedSleep.runtimePosition(timedSleeperId,reachedPosition));
    assert(reachedPosition.x==timedBedPos.x);
    assert(reachedPosition.y==timedBedPos.y);

    const double fatigueAtSleepStart=timedSleeper.needs.sleep;
    timedSleep.runMinutes(60);
    assert(timedSleeper.needs.sleep<fatigueAtSleepStart);
    assert(timedSleeper.needs.sleep>RestedSleepNeedTarget);

    bool wokeRested=false;
    for(int minute=0;minute<MaximumSleepSessionMinutes && !wokeRested;++minute){
        timedSleep.step();
        const ResidentObservation observed=
            timedSleep.observeResident(timedSleeperId);
        wokeRested=!(
            observed.activityKind==ObservedActivityKind::Physical
            && observed.physicalGoal==Goal::Sleep);
    }
    assert(wokeRested);
    assert(timedSleeper.needs.sleep<0.20);
    assert(timedSleep.world().facilities.back().usageCount==1);

    std::cout << "Settlement capacity, local demand, paid construction, maintenance, persistence and runtime wiring passed\n";
}
