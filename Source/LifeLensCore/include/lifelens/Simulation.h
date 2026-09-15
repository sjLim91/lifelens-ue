#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "Birth.h"
#include "CivilizationKnowledgeTransmission.h"
#include "CivilizationObserverReadModel.h"
#include "DecisionExecution.h"
#include "EnvironmentalResidue.h"
#include "ObserverReadModelV2.h"
#include "Planner.h"
#include "SimulationSnapshot.h"
namespace lifelens {
class Simulation {
public:
    using EventCallback=std::function<void(const std::string&)>;
    explicit Simulation(std::uint64_t seed=1);
    void setupDemo();
    void setupSocialDemo();
    void setupNewGame();
    void step();
    void runMinutes(int minutes);
    void setExternalPhysicalExecution(bool enabled){ world_.externalPhysicalExecution=enabled; }
    bool externalPhysicalExecutionEnabled() const{return world_.externalPhysicalExecution;}
    bool completeExternalPhysicalAction(
        CharacterId id,
        bool emergencyFallback,
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
    ResidentObservation observeResident(CharacterId id) const;
    std::vector<ResidentObservation> observeAllResidents() const;
    FamilyObservation observeFamily(CharacterId id) const;
    WorldOverviewObservation observeWorldOverview() const;
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
    };
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
    void emit(const std::string& message);
    std::string stamp() const;
    SmartObject* objectById(ObjectId id);
    void beginPlan(Character& c,Runtime& r);
    void advanceAction(Character& c,Runtime& r);
    void failPlan(Runtime& r);
    bool tryCivilizationDecision(Character& c,Runtime& r);
    bool trySocialDecision(Character& c,Runtime& r);
    void processCivilizationKnowledgeEvent(Character& actor,const CivilizationEvent& event);
    void advanceCivilizationKnowledgeTeaching();
    void advanceAutonomousFamilyProgression();
    void updatePregnanciesAndBirths();
    void evaluateDailyFamilyTransitions();
    CharacterId nextCharacterId() const;
    HouseholdId nextHouseholdId() const;
    std::string makeChildName(Sex sex,CharacterId childId) const;
};

inline bool Simulation::completeExternalPhysicalAction(
    CharacterId id,
    bool emergencyFallback,
    GridPos resolvedPosition)
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

    // Food and water are never synthesized by presentation. Even when a real
    // table/campfire/well presentation affordance is used, Core must possess the
    // consumable provision before it can acknowledge the outcome.
    if(runtime.goal==Goal::Eat && !character->civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::PlantFood,1)) return false;
    if(runtime.goal==Goal::Drink && !character->civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) return false;

    const int duration=emergencyFallback
        ? emergencyUseDurationTicks(runtime.goal)
        : facilityUseDurationTicks(runtime.goal);
    const NeedsDelta effect=emergencyFallback
        ? emergencyUseEffectPerTick(runtime.goal)
        : facilityUseEffectPerTick(runtime.goal);
    for(int tick=0;tick<std::max(1,duration);++tick){
        character->needs.apply(effect);
    }

    // World is the physical resolver, so Core records environmental consequence
    // at the actual acknowledged world-grid position rather than independently
    // choosing a second position that could disagree with what the player saw.
    runtime.pos=resolvedPosition;
    if(emergencyFallback && runtime.goal==Goal::UseToilet){
        const auto& residue=world_.environmentalResidues.deposit(
            EnvironmentalResidueKind::HumanWaste,resolvedPosition,character->id,
            world_.minute,1.0,0.42,3);
        character->needs.hygiene=Needs::clamp01(character->needs.hygiene+0.025);
        emit(character->name+" left sanitation residue id="+std::to_string(residue.id));
    }

    emit(character->name+" completed "+std::string(goalName(runtime.goal))+
         (emergencyFallback ? " via emergency fallback" : " via world affordance"));

    runtime.plan.clear();
    runtime.actionIndex=0;
    runtime.announced=false;
    runtime.consecutiveFailures=0;
    return true;
}

}
