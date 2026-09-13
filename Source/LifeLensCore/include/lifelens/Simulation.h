#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "DecisionExecution.h"
#include "Planner.h"
namespace lifelens {
class Simulation {
public:
    using EventCallback=std::function<void(const std::string&)>;
    explicit Simulation(std::uint64_t seed=1);
    void setupDemo();
    void setupSocialDemo();
    void step();
    void runMinutes(int minutes);
    void onEvent(EventCallback cb);
    World& world(){return world_;}
    const World& world() const{return world_;}
    RelationshipBook& relationships(){return relationships_;}
    const RelationshipBook& relationships() const{return relationships_;}
    const std::vector<std::string>& logs() const{return logs_;}
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
    };
    World world_;
    RelationshipBook relationships_;
    std::unordered_map<CharacterId,Runtime> runtime_;
    std::vector<EventCallback> callbacks_;
    std::vector<std::string> logs_;
    void emit(const std::string& message);
    std::string stamp() const;
    SmartObject* objectById(ObjectId id);
    void beginPlan(Character& c,Runtime& r);
    void advanceAction(Character& c,Runtime& r);
    void failPlan(Runtime& r);
    bool trySocialDecision(Character& c,Runtime& r);
};
}
