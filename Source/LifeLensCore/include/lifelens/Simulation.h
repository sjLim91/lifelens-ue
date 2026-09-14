#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
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
    bool completeExternalPhysicalAction(CharacterId id);
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

inline bool Simulation::completeExternalPhysicalAction(CharacterId id)
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

    // Resolve the same authoritative Core plan only when the physical executor
    // confirms arrival/use. Temporarily disable external waiting so buildPlan()
    // performs normal resource validation and produces the real action sequence.
    world_.externalPhysicalExecution=false;
    std::vector<Action> resolvedPlan=buildPlan(world_,*character,runtime.goal,runtime.pos);
    world_.externalPhysicalExecution=true;
    if(resolvedPlan.empty()) return false;

    runtime.plan=std::move(resolvedPlan);
    runtime.actionIndex=0;
    runtime.announced=false;

    // Fast-forward the authoritative action sequence after physical completion.
    // This preserves the existing need effects, resource consumption, Core grid
    // movement, reservations/releases and environmental residue rules while
    // preventing any of them from happening before the world ACK.
    int guard=0;
    while(!runtime.plan.empty() && guard<8192){
        advanceAction(*character,runtime);
        ++guard;
    }
    if(!runtime.plan.empty()){
        failPlan(runtime);
        return false;
    }
    return true;
}

}
