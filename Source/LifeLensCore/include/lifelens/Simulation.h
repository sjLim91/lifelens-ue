#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "Birth.h"
#include "CivilizationActivityReadModel.h"
#include "CivilizationKnowledgeTransmission.h"
#include "CivilizationObserverReadModel.h"
#include "ContextAction.h"
#include "Death.h"
#include "DecisionExecution.h"
#include "EmotionRuntime.h"
#include "EnvironmentalExposure.h"
#include "EnvironmentalResidue.h"
#include "ObserverReadModelV2.h"
#include "Parenting.h"
#include "Planner.h"
#include "PrimitiveSanitation.h"
#include "PresentationDirective.h"
#include "SimulationSnapshot.h"
#include "SettlementProgression.h"
#include "SocialCommunicationReadModel.h"
#include "SocietyEconomy.h"
namespace lifelens {
class Simulation {
public:
    using EventCallback=std::function<void(const std::string&)>;
    explicit Simulation(
        WorldSeed worldSeed=1,
        PopulationSeed populationSeed=0,
        WorldGenerationVersion generationVersion=CurrentWorldGenerationVersion,
        SimulationRuleset ruleset=DefaultSimulationRuleset);
    void setupDemo();
    void setupSocialDemo();
    void setupNewGame();
    void step();
    void runMinutes(int minutes);
    const SimulationRuleset& ruleset() const{return ruleset_;}
    void setExternalPhysicalExecution(bool enabled){ world_.externalPhysicalExecution=enabled; }
    bool externalPhysicalExecutionEnabled() const{return world_.externalPhysicalExecution;}
    bool runtimePosition(CharacterId id,GridPos& outPosition) const {
        const auto it=runtime_.find(id);
        if(it==runtime_.end()) return false;
        outPosition=it->second.pos;
        return true;
    }
    bool recommendedOutdoorReliefPosition(CharacterId id,GridPos& outPosition) const {
        const auto runtimeIt=runtime_.find(id);
        if(runtimeIt==runtime_.end()) return false;
        const Character* character=nullptr;
        for(const auto& candidate:world_.characters){
            if(candidate.id==id){ character=&candidate; break; }
        }
        if(character==nullptr || !character->alive) return false;
        outPosition=chooseLowExposureOutdoorReliefPosition(
            world_.seed,*character,world_.environmentalResidues,world_.minute,
            runtimeIt->second.pos);
        return true;
    }
    bool sanitationUseTarget(CharacterId id,SanitationUseTarget& outTarget) const {
        const auto runtimeIt=runtime_.find(id);
        if(runtimeIt==runtime_.end()) return false;
        const Character* character=nullptr;
        for(const auto& candidate:world_.characters){
            if(candidate.id==id){ character=&candidate; break; }
        }
        if(character==nullptr || !character->alive) return false;
        outTarget=resolveSanitationUseTarget(
            world_.seed,*character,world_.environmentalResidues,
            world_.primitiveSanitationSites,world_.minute,runtimeIt->second.pos);
        return true;
    }
    bool settlementSleepTarget(
        CharacterId id,
        GridPos& outPosition,
        FacilityId& outFacilityId) const {
        outPosition={};
        outFacilityId=0;
        const auto runtimeIt=runtime_.find(id);
        if(runtimeIt==runtime_.end()) return false;
        const Character* character=nullptr;
        for(const auto& candidate:world_.characters){
            if(candidate.id==id){ character=&candidate; break; }
        }
        if(character==nullptr || !character->alive) return false;
        const ConstructedFacility* facility=
            nearestAvailableOperationalSleepFacility(
                id,runtimeIt->second.pos);
        if(facility==nullptr) return false;
        outPosition=facility->pos;
        outFacilityId=facility->id;
        return true;
    }
    bool completeExternalPhysicalAction(
        CharacterId id,
        bool emergencyFallback,
        GridPos resolvedPosition,
        SanitationSiteId sanitationSiteId=0);
    PendingContextActionObservation observePendingContextAction(CharacterId id) const;
    bool completeExternalContextAction(
        CharacterId id,
        std::uint64_t token,
        GridPos resolvedPosition);
    void onEvent(EventCallback cb);
    SimulationStateSnapshot captureSnapshot() const;
    bool restoreSnapshot(const SimulationStateSnapshot& snapshot,std::string* error=nullptr);
    World& world(){return world_;}
    const World& world() const{return world_;}
    RelationshipBook& relationships(){return relationships_;}
    const RelationshipBook& relationships() const{return relationships_;}
    GenealogyBook& genealogy(){return genealogy_;}
    const GenealogyBook& genealogy() const{return genealogy_;}
    RomanceBook& romances(){return romances_;}
    const RomanceBook& romances() const{return romances_;}
    HouseholdBook& households(){return households_;}
    const HouseholdBook& households() const{return households_;}
    PregnancyBook& pregnancies(){return pregnancies_;}
    const PregnancyBook& pregnancies() const{return pregnancies_;}
    BirthBook& births(){return births_;}
    const BirthBook& births() const{return births_;}
    SocialKnowledgeBook& socialKnowledge(){return socialKnowledge_;}
    const SocialKnowledgeBook& socialKnowledge() const{return socialKnowledge_;}
    const std::vector<std::string>& logs() const{return logs_;}
    std::vector<SocialCommunicationObservation> observeRecentSocialEvents(
        std::size_t maxEvents=32) const {
        return recentSocialCommunicationTail(recentSocialEvents_,maxEvents);
    }
    ResidentObservation observeResident(CharacterId id) const;
    ResidentPresentationObservation observeResidentPresentation(CharacterId id) const;
    std::vector<ResidentObservation> observeAllResidents() const;
    FamilyObservation observeFamily(CharacterId id) const;
    WorldOverviewObservation observeWorldOverview() const;
    ResidentCivilizationActivityObservation observeResidentCivilizationActivity(CharacterId id) const;
    EnvironmentObservation observeEnvironment(std::size_t maxResidues=64) const {
        return buildEnvironmentObservation(world_.minute,world_.environmentalResidues,maxResidues);
    }
    double environmentExposureAt(GridPos pos) const {
        return world_.environmentalResidues.exposureAt(pos);
    }
    ResidentCivilizationObservation observeResidentCivilization(CharacterId id) const {
        const Character* character=findObservedCharacter(world_,id);
        return character==nullptr
            ? ResidentCivilizationObservation{}
            : buildResidentCivilizationObservation(world_,socialKnowledge_,*character);
    }
    CivilizationWorldObservation observeCivilizationWorld(std::size_t maxRecentDiscoveries=32) const {
        return buildCivilizationWorldObservation(world_,socialKnowledge_,maxRecentDiscoveries);
    }
    SocietyWorldObservation observeSocietyWorld() const {
        return buildSocietyWorldObservation(
            world_,socialKnowledge_,&households_);
    }
    CivilizationWorldObservation observeCivilizationWorldWindow(
        ChunkCoord center,
        int radiusChunks,
        std::size_t maxRecentDiscoveries=32) const
    {
        CivilizationResourceObservationWindow window;
        window.center=center;
        window.radiusChunks=std::max(0,radiusChunks);
        return buildCivilizationWorldObservation(
            world_,
            socialKnowledge_,
            maxRecentDiscoveries,
            &window);
    }
private:
    SettlementPopulation settlementPopulation() const;
    bool sleepFacilityHasCapacityFor(
        CharacterId requester,
        const ConstructedFacility& facility) const;
    const ConstructedFacility* nearestAvailableOperationalSleepFacility(
        CharacterId requester,
        GridPos from) const;
    struct Runtime {
        Goal goal=Goal::Idle;
        std::vector<Action> plan;
        std::size_t actionIndex=0;
        GridPos pos{};
        bool announced=false;
        Goal lastGoal=Goal::Idle;
        int repeatCount=0;
        int consecutiveFailures=0;
        int penaltyUntilMinute=0;
        int socialCooldownUntilMinute=0;
        bool socialActive=false;
        SocialIntent socialIntent=SocialIntent::None;
        CharacterId socialTarget=0;
        PendingContextAction pendingContext{};

        // Headless/Core-owned locomotion continuation state. Save/restore
        // persists this because route choice/index affects deterministic future
        // simulation truth; presentation transforms remain separate.
        std::vector<GridPos> navigationRoute;
        std::size_t navigationRouteIndex=0;
        GridPos navigationTarget{};
        int navigationArrivalRadius=0;
        bool navigationHasTarget=false;
        bool navigationArrived=false;
        bool navigationRouteFailed=false;

        // Presentation provenance for the civilization action that actually
        // executed. Intentionally omitted from SimulationRuntimeSnapshot so
        // save/restore never replays stale work animations.
        bool civilizationActive=false;
        CivilizationEvent civilizationEvent{};
        int civilizationActivityMinute=-1;
        ResourceNodeId civilizationResourceNode=0;
        StorageId civilizationStorage=0;
        bool civilizationHasSpatialTarget=false;
        GridPos civilizationTargetPos{};
        std::uint64_t civilizationSanitationSiteId=0;
    };
    const SimulationRuleset ruleset_;
    World world_;
    RelationshipBook relationships_;
    GenealogyBook genealogy_;
    RomanceBook romances_;
    HouseholdBook households_;
    PregnancyBook pregnancies_;
    BirthBook births_;
    SocialKnowledgeBook socialKnowledge_;
    std::unordered_map<CharacterId,Runtime> runtime_;
    // Health fatalities are computed and consumed inside the same daily Core
    // transition. They are runtime transition state, never a second save truth.
    std::unordered_map<CharacterId,HealthFatalCause> pendingHealthFatality_;
    std::vector<EventCallback> callbacks_;
    std::vector<std::string> logs_;
    std::vector<SocialCommunicationObservation> recentSocialEvents_;
    std::uint64_t nextContextActionToken_=InitialContextActionToken;
    std::uint64_t nextSocialEventSequence_=1;
    static constexpr std::size_t MaxRecentSocialEvents=64;
    void emit(const std::string& message);
    void recordSocialEvent(const SocialEvent& event){
        if(event.actor==0 || event.recipient==0) return;
        recentSocialEvents_.push_back(
            makeSocialCommunicationObservation(event,nextSocialEventSequence_++));
        if(recentSocialEvents_.size()>MaxRecentSocialEvents){
            recentSocialEvents_.erase(
                recentSocialEvents_.begin(),
                recentSocialEvents_.begin()+static_cast<std::ptrdiff_t>(
                    recentSocialEvents_.size()-MaxRecentSocialEvents));
        }
    }
    std::uint64_t issueContextActionToken(){
        return consumeContextActionToken(nextContextActionToken_);
    }
    void clearRecentSocialEvents(){
        recentSocialEvents_.clear();
        nextSocialEventSequence_=1;
    }
    std::string stamp() const;
    SmartObject* objectById(ObjectId id);
    void beginPlan(Character& c,Runtime& r);
    void advanceAction(Character& c,Runtime& r);
    void failPlan(Character& character,Runtime& r);
    void cancelRuntimeActivityForCriticalReplan(
        Character& character,Runtime& r);
    bool preemptForCriticalSurvival(Character& character,Runtime& r);
    void clearRuntimeActivity(Runtime& r);
    void clearNavigation(Runtime& r);
    bool advanceNavigation(CharacterId moverId,Runtime& r,GridPos target,int arrivalRadius);
    bool advancePendingContext(Character& actor,Runtime& runtime);
    bool completeContextAction(Character& actor,Runtime& runtime,std::uint64_t token,GridPos resolvedPosition);
    bool tryCivilizationDecision(Character& c,Runtime& r);
    bool trySocialDecision(Character& c,Runtime& r);
    void processCivilizationKnowledgeEvent(Character& actor,CivilizationEvent& event);
    void advanceCivilizationKnowledgeTeaching();
    void advanceSocietyExchange();
    void advanceSocietyInstitutions();
    void advanceSocietyRecordkeeping();
    void advanceDependentCare();
    void advanceAutonomousFamilyProgression();
    void updatePregnanciesAndBirths();
    void advanceDailyPopulationHealth();
    void evaluateDailyMortality();
    void evaluateDailyFamilyTransitions();
    CharacterId nextCharacterId() const;
    HouseholdId nextHouseholdId() const;
    std::string makeChildName(Sex sex,CharacterId childId) const;
};

inline bool Simulation::completeExternalPhysicalAction(
    CharacterId id,
    bool emergencyFallback,
    GridPos resolvedPosition,
    SanitationSiteId sanitationSiteId)
{
    if(!world_.externalPhysicalExecution) return false;

    auto runtimeIt=runtime_.find(id);
    if(runtimeIt==runtime_.end()) return false;

    Character* character=nullptr;
    for(auto& candidate:world_.characters){
        if(candidate.id==id){ character=&candidate; break; }
    }
    if(character==nullptr || !character->alive) return false;

    Runtime& runtime=runtimeIt->second;
    if(runtime.socialActive || runtime.goal==Goal::Idle || runtime.plan.empty()) return false;

    const bool primitiveSanitation=sanitationSiteId!=0;
    const PrimitiveSanitationSite* sanitationSite=nullptr;
    if(primitiveSanitation){
        if(runtime.goal!=Goal::UseToilet || emergencyFallback) return false;
        sanitationSite=findPrimitiveSanitationSite(
            world_.primitiveSanitationSites,sanitationSiteId);
        if(sanitationSite==nullptr || !sanitationSite->active
           || !validPrimitiveSanitationSiteKind(sanitationSite->kind)
           || sanitationSite->pos.x!=resolvedPosition.x
           || sanitationSite->pos.y!=resolvedPosition.y) return false;
    }

    // Food/water/hygiene provisions are never synthesized by presentation.
    // Core must own the consumable before it can acknowledge the outcome.
    if(runtime.goal==Goal::Eat && !character->civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::PlantFood,1)) return false;
    if(runtime.goal==Goal::Drink || runtime.goal==Goal::Wash){
        if(portableWaterCount(character->civilization.inventory)>0){
            if(!consumePortableWater(
                    character->civilization.inventory,1)){
                return false;
            }
        }else{
            ResourceNode* directWater=nullptr;
            for(ResourceNode& node:world_.resourceNodes){
                if(node.id==0
                   || node.material!=MaterialKind::Water
                   || node.quantity<=0){
                    continue;
                }
                GridPos access{};
                if(!resolveCivilizationResourceAccessGridPosition(
                        world_,node.id,access)
                   || access.x!=resolvedPosition.x
                   || access.y!=resolvedPosition.y){
                    continue;
                }
                if(directWater==nullptr || node.id<directWater->id){
                    directWater=&node;
                }
            }
            if(directWater==nullptr) return false;
            --directWater->quantity;
            if(runtime.goal==Goal::Drink){
                recordContaminatedWaterExposure(
                    character->health,
                    world_.environmentalResidues.exposureAt(resolvedPosition),
                    world_.minute);
            }
        }
    }

    const Needs beforeNeeds=character->needs;
    const PrimitiveSanitationSiteKind sanitationKind=sanitationSite!=nullptr
        ? sanitationSite->kind
        : PrimitiveSanitationSiteKind::DesignatedArea;
    ConstructedFacility* settlementSleepFacility=nullptr;
    if(runtime.goal==Goal::Sleep && emergencyFallback){
        settlementSleepFacility=
            bestOperationalSleepFacility(world_,resolvedPosition,1);
        if(settlementSleepFacility!=nullptr
           && !sleepFacilityHasCapacityFor(
               id,*settlementSleepFacility)){
            return false;
        }
    }

    const double sleepRecoveryPerMinute=runtime.goal==Goal::Sleep
        ? sleepRecoveryPerMinuteAt(
            world_,resolvedPosition,settlementSleepFacility)
        : 0.0;
    const int duration=primitiveSanitation
        ? primitiveSanitationUseDurationTicks(sanitationKind)
        : (runtime.goal==Goal::Sleep
            ? sleepDurationMinutesForNeed(
                *character,sleepRecoveryPerMinute,ruleset_.needs)
            : (emergencyFallback
                ? emergencyUseDurationTicks(runtime.goal)
                : facilityUseDurationTicks(runtime.goal)));
    const NeedsDelta effect=primitiveSanitation
        ? primitiveSanitationUseEffectPerTick(sanitationKind)
        : (runtime.goal==Goal::Sleep
            ? NeedsDelta{0,0,-sleepRecoveryPerMinute,0,0}
            : (emergencyFallback
                ? emergencyUseEffectPerTick(runtime.goal)
                : facilityUseEffectPerTick(runtime.goal)));
    for(int tick=0;tick<std::max(1,duration);++tick){
        character->needs.apply(effect);
    }
    if(settlementSleepFacility!=nullptr){
        recordFacilityUse(
            *settlementSleepFacility,
            character->id,
            world_.minute);
        applyFacilityWear(
            *settlementSleepFacility,
            facilityWearPerUse(settlementSleepFacility->kind));
    }

    // World is the physical resolver, so Core records environmental consequence
    // at the actual acknowledged world-grid position rather than independently
    // choosing a second position that could disagree with what the player saw.
    runtime.pos=resolvedPosition;
    if(runtime.goal==Goal::UseToilet && (emergencyFallback || primitiveSanitation)){
        // #79 compatibility contract: recordDesignatedSanitationSiteUse remains
        // the designated-area wrapper; the generalized path below accepts both
        // DesignatedArea and DugPit without changing site identity/GridPos.
        if(primitiveSanitation && !recordPrimitiveSanitationSiteUse(
            world_.primitiveSanitationSites,sanitationSiteId,resolvedPosition)) return false;
        const double residueIntensity=primitiveSanitation
            ? primitiveSanitationResidueIntensity(sanitationKind)
            : 0.42;
        const int residueRadius=primitiveSanitation
            ? primitiveSanitationResidueRadiusTiles(sanitationKind)
            : 3;
        const auto& residue=world_.environmentalResidues.deposit(
            EnvironmentalResidueKind::HumanWaste,resolvedPosition,character->id,
            world_.minute,1.0,residueIntensity,residueRadius);
        const double hygieneBurden=primitiveSanitation
            ? primitiveSanitationHygieneBurden(sanitationKind)
            : 0.025;
        character->needs.hygiene=Needs::clamp01(character->needs.hygiene+hygieneBurden);
        emit(character->name+" left sanitation residue id="+std::to_string(residue.id));
    }

    applyNeedResolutionEmotion(*character,beforeNeeds,runtime.goal);

    const EnvironmentalExposureResult exposure=perceiveEnvironmentalContamination(
        *character,world_.environmentalResidues,resolvedPosition,world_.minute);
    if(exposure.memoryRecorded){
        emit(character->name+" noticed unsanitary surroundings at ("+
             std::to_string(resolvedPosition.x)+","+std::to_string(resolvedPosition.y)+")");
    }
    if(exposure.sanitationProblemNewlyRecognized){
        emit(character->name+" recognized recurring human-waste contamination as a sanitation problem");
    }

    if(primitiveSanitation){
        emit(character->name+" completed "+std::string(goalName(runtime.goal))+
             " at primitive sanitation site="+std::to_string(sanitationSiteId));
    }else if(settlementSleepFacility!=nullptr){
        emit(character->name+" completed Sleep at settlement facility="+
             std::to_string(settlementSleepFacility->id));
    }else{
        emit(character->name+" completed "+std::string(goalName(runtime.goal))+
             (emergencyFallback ? " via emergency fallback" : " via world affordance"));
    }

    runtime.plan.clear();
    runtime.actionIndex=0;
    runtime.announced=false;
    runtime.consecutiveFailures=0;
    return true;
}

}
