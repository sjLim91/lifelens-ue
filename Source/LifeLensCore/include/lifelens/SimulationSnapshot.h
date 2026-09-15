#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Birth.h"
#include "DecisionExecution.h"
#include "ObserverReadModelV2.h"
#include "Planner.h"
#include "SimulationRuleset.h"
#include "WitnessRumor.h"

namespace lifelens {

constexpr std::uint32_t SimulationSnapshotVersion=2;

struct SimulationRuntimeSnapshot {
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

struct SimulationStateSnapshot {
    std::uint32_t version=SimulationSnapshotVersion;
    SimulationRuleset ruleset=DefaultSimulationRuleset;
    World world{1};
    RelationshipBook relationships;
    GenealogyBook genealogy;
    RomanceBook romances;
    HouseholdBook households;
    PregnancyBook pregnancies;
    BirthBook births;
    SocialKnowledgeBook socialKnowledge;
    std::unordered_map<CharacterId,SimulationRuntimeSnapshot> runtime;
    std::vector<std::string> logs;
};

} // namespace lifelens
