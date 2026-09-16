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
#include "EnvironmentalExposure.h"
#include "EnvironmentalResidue.h"
#include "ObserverReadModelV2.h"
#include "Parenting.h"
#include "Planner.h"
#include "PrimitiveSanitation.h"
#include "SimulationSnapshot.h"
#include "SocialCommunicationReadModel.h"
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
        const GridPos settlementReference=world_.hasInitialStartRegionSelection
            ? world_.initialStartRegionCenterGrid()
            : GridPos{};
        outPosition=chooseLowExposureOutdoorReliefPosition(
            world_.seed,*character,world_.environmentalResidues,world_.minute,
            settlementReference);
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
        const GridPos settlementReference=world_.hasInitialStartRegionSelection
            ? world_.initialStartRegionCenterGrid()
            : GridPos{};
        outTarget=resolveSanitationUseTarget(
            world_.seed,*character,world_.environmentalResidues,
            world_.primitiveSanitationSites,world_.minute,settlementReference);
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
private:
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
    std::vector<EventCallback> callbacks_;
    std::vector<std::string> logs_;
    std::vector<SocialCommunicationObservation> recentSocialEvents_;
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
    void clearRecentSocialEvents(){
        recentSocialEvents_.clear();
        nextSocialEventSequence_=1;
    }
    std::string stamp() const;
    SmartObject* objectById(ObjectId id);
    void beginPlan(Character& c,Runtime& r);
    void advanceAction(Character& c,Runtime& r);
    void failPlan(Runtime& r);
    void clearRuntimeActivity(Runtime& r);
    bool completeContextAction(Character& actor,Runtime& runtime,std::uint64_t token,GridPos resolvedPosition);
    bool tryCivilizationDecision(Character& c,Runtime& r);
    bool trySocialDecision(Character& c,Runtime& r);
    void processCivilizationKnowledgeEvent(Character& actor,const CivilizationEvent& event);
    void advanceCivilizationKnowledgeTeaching();
    void advanceDependentCare();
    void advanceAutonomousFamilyProgression();
    void updatePregnanciesAndBirths();
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

    // Food and water are never synthesized by presentation. Even when a real
    // table/campfire/well presentation affordance is used, Core must possess the
    // consumable provision before it can acknowledge the outcome.
    if(runtime.goal==Goal::Eat && !character->civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::PlantFood,1)) return false;
    if(runtime.goal==Goal::Drink && !character->civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) return false;

    const PrimitiveSanitationSiteKind sanitationKind=sanitationSite!=nullptr
        ? sanitationSite->kind
        : PrimitiveSanitationSiteKind::DesignatedArea;
    const int duration=primitiveSanitation
        ? primitiveSanitationUseDurationTicks(sanitationKind)
        : (emergencyFallback
            ? emergencyUseDurationTicks(runtime.goal)
            : facilityUseDurationTicks(runtime.goal));
    const NeedsDelta effect=primitiveSanitation
        ? primitiveSanitationUseEffectPerTick(sanitationKind)
        : (emergencyFallback
            ? emergencyUseEffectPerTick(runtime.goal)
            : facilityUseEffectPerTick(runtime.goal));
    for(int tick=0;tick<std::max(1,duration);++tick){
        character->needs.apply(effect);
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