#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "Birth.h"
#include "CivilizationKnowledgeTransmission.h"
#include "DecisionExecution.h"
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
}
