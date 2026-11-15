#pragma once
#include "CognitiveCausalJournal.h"

namespace lifelens {
inline constexpr std::size_t MaxProductionSteps=6;
inline constexpr int ProductionPlanMinutes=120;
enum class ProductionPlanState { Active, Paused, Completed, Abandoned };
enum class ProductionPlanReason { None, Survival, SourceExhausted, SourceRemoved,
    AccessUnavailable, ContainerMissing, MaterialMissing, KnowledgeMissing,
    TargetMissing, Rejected, Illness, Unknown, Deadline, Budget, NewerGoal,
    SatisfiedElsewhere, ActorDied, NoFeasibleStep };
struct ResidentProductionStep {
    std::uint64_t token=0;
    int issuedMinute=-1,finishedMinute=-1;
    CivilizationIntent intent=CivilizationIntent::None;
    TechniqueId technique=TechniqueId::None;
    MaterialKind material=MaterialKind::Unknown;
    ResourceNodeId source=0;
    CognitiveStepResult result=CognitiveStepResult::Pending;
    ProductionPlanReason reason=ProductionPlanReason::None;
    int stockBefore=0,stockAfter=0;
    GridPos from{},to{};
    bool attempted=false;
};
// One concrete objective of the existing resident agency, never an executor.
// Native embodied choices and journal-validated model choices retain distinct
// provenance. Only actual issued ContextActions enter the bounded step list.
struct ResidentProductionPlan {
    std::uint64_t id=0;
    CharacterId actor=0;
    TechniqueId target=TechniqueId::None;
    int createdMinute=-1,expiresMinute=-1,finishedMinute=-1;
    int targetStock=0;
    std::uint64_t cognitiveFingerprint=0;
    int thoughtMinute=-1;
    double priority=0;
    ProductionPlanState state=ProductionPlanState::Active;
    ProductionPlanReason reason=ProductionPlanReason::None;
    std::vector<ResidentProductionStep> steps;
    bool open() const {return id &&(state==ProductionPlanState::Active ||state==ProductionPlanState::Paused);}
};
inline bool productionToolTechnique(TechniqueId t) {
    const auto r=techniqueRecipe(t);
    return t!=TechniqueId::None &&r.technique==t &&r.producesItem
        &&r.outputQuantity>0 &&(itemCapability(r.outputKind)!=ToolCapability::None
                              ||r.outputKind==ItemKind::Cordage);
}
inline int productionOutputStock(const Character& c,TechniqueId t) {
    const auto r=techniqueRecipe(t);
    return c.civilization.inventory.count(r.outputKind,r.outputMaterial,r.outputMaterial==MaterialKind::Unknown);
}
inline int productionStepStock(const Character& c,const ResidentProductionStep& s) {
    return s.intent==CivilizationIntent::Gather
        ?c.civilization.inventory.count(ItemKind::RawMaterial,s.material)
        :productionOutputStock(c,s.technique);
}
inline bool sameProductionCommand(const CivilizationUtilityDecision& a,const CivilizationUtilityDecision& b) {
    return a.intent==b.intent &&a.technique==b.technique &&a.material==b.material
        &&a.resourceNode==b.resourceNode &&a.experiment==b.experiment
        &&a.facility==b.facility &&a.facilityAction==b.facilityAction;
}
} // namespace lifelens
