#include "lifelens/Simulation.h"
#include "lifelens/ResidentAIExistingAffordance.h"
#include <limits>

namespace lifelens {
namespace {
CivilizationUtilityDecision nextProductionStep(const World& world,const Character& c,
    GridPos position,TechniqueId target,const SettlementPopulation* population,int depth=0) {
    if(depth>3 ||!productionToolTechnique(target))return {};
    const bool known=c.civilization.knowledge.knowsAtLeast(target,KnowledgeLevel::Reproducible);
    TechniqueRecipe recipe=techniqueRecipe(target);
    if(!known) {
        bool supported=false;
        for(const auto kind:CivilizationExperimentCatalog)if(experimentTechnique(kind)==target) {
            ExperimentContext context;context.worldSeed=world.seed;context.actor=c.id;
            context.attemptIndex=static_cast<std::uint64_t>(world.minute);context.kind=kind;
            context.material=experimentMaterial(kind);
            if(experimentPrerequisitesMet(context,c.civilization.knowledge)
               &&technologyKnowledgePrerequisitesSatisfied(c.civilization.knowledge,technologyIdForTechnique(target))) {
                recipe=experimentRecipe(context);supported=true;break;
            }
        }
        if(!supported)return {};
    }
    for(const auto& input:recipe.inputs) {
        const int missing=input.quantity-c.civilization.inventory.count(input.kind,input.material,input.anyMaterial);
        if(missing<=0)continue;
        if(input.kind==ItemKind::RawMaterial) {
            auto next=chooseResidentAIFreshIngredientGather(world,c,position,input.material,population);
            if(next.intent==CivilizationIntent::Gather) {
                next.quantity=std::min(next.quantity,missing);
                next.gatherGoalTarget=c.civilization.inventory.count(input.kind,input.material,input.anyMaterial)+missing;
            }
            return next;
        }
        for(const auto& technology:TechnologyRegistry) {
            const auto intermediate=techniqueRecipe(technology.legacyTechnique);
            if(intermediate.producesItem &&intermediate.outputKind==input.kind
               &&(input.anyMaterial ||intermediate.outputMaterial==input.material))
                return nextProductionStep(world,c,position,intermediate.technique,population,depth+1);
        }
        return {};
    }
    return known ?bestCraftDecisionAtPosition(world,c,position,population,target)
        :bestExperimentDecisionAtPosition(world,c,position,population,target);
}
bool executableProductionStep(const CivilizationUtilityDecision& d) {
    return std::isfinite(d.utility) &&d.utility>=0.14 &&d.utility<=1
        &&d.facilityAction==FacilityBuildAction::None
        &&(d.intent==CivilizationIntent::Gather ||d.intent==CivilizationIntent::Craft
           ||d.intent==CivilizationIntent::Experiment);
}
ProductionPlanReason productionFailure(const World& w,const Character& c,
    const ResidentProductionStep& s,CognitiveStepResult result,const std::string& reason,bool attempted) {
    if(result==CognitiveStepResult::Succeeded)return ProductionPlanReason::None;
    if(result==CognitiveStepResult::Interrupted)return ProductionPlanReason::Survival;
    if(result==CognitiveStepResult::RouteFailed)return ProductionPlanReason::AccessUnavailable;
    if(result==CognitiveStepResult::Expired)return ProductionPlanReason::Deadline;
    if(!attempted)return ProductionPlanReason::Unknown;
    if(s.intent==CivilizationIntent::Gather) {
        const auto* n=findCivilizationResourceNodeSpatial(w,s.source);
        if(!n)return ProductionPlanReason::SourceRemoved;
        if(n->quantity<=0)return ProductionPlanReason::SourceExhausted;
        if(s.material==MaterialKind::Water &&emptySimpleContainerCount(c.civilization.inventory)<=0)
            return ProductionPlanReason::ContainerMissing;
    }
    if(s.intent==CivilizationIntent::Craft &&productionToolTechnique(s.technique)) {
        if(!c.civilization.knowledge.knowsAtLeast(s.technique,KnowledgeLevel::Reproducible))return ProductionPlanReason::KnowledgeMissing;
        for(const auto& input:techniqueRecipe(s.technique).inputs)
            if(c.civilization.inventory.count(input.kind,input.material,input.anyMaterial)<input.quantity)
                return ProductionPlanReason::MaterialMissing;
    }
    if(reason=="actor-died")return ProductionPlanReason::ActorDied;
    if(reason=="civilization-core-rejected")return ProductionPlanReason::Rejected;
    // A probabilistic failed experiment is not proof of a missing tool,
    // sickness, inaccessible route or rejected social proposal.
    return ProductionPlanReason::Unknown;
}
}

void Simulation::maintainProductionPlan(Character& c,Runtime& r) {
    auto& p=r.productionPlan;if(!p.open())return;
    ProductionPlanReason abandoned=ProductionPlanReason::None;
    if(!c.alive)abandoned=ProductionPlanReason::ActorDied;
    else if(world_.minute>=p.expiresMinute)abandoned=ProductionPlanReason::Deadline;
    else if(p.steps.size()>=MaxProductionSteps &&p.steps.back().result!=CognitiveStepResult::Pending)
        abandoned=ProductionPlanReason::Budget;
    else if(productionOutputStock(c,p.target)>=p.targetStock)
        abandoned=ProductionPlanReason::SatisfiedElsewhere;
    for(auto it=acceptedCognitiveDecisions_.rbegin();it!=acceptedCognitiveDecisions_.rend();++it)if(it->actor==c.id) {
        if(it->minute>p.createdMinute &&it->contextFingerprint!=p.cognitiveFingerprint
           &&(p.steps.empty() ||p.steps.back().result!=CognitiveStepResult::Pending
              ||r.pendingContext.token!=p.steps.back().token))
            abandoned=ProductionPlanReason::NewerGoal;
        break;
    }
    if(abandoned!=ProductionPlanReason::None) {
        // Do not cancel an urgent survival action or another real command.
        if(!p.steps.empty() &&p.steps.back().result==CognitiveStepResult::Pending
           &&r.pendingContext.token==p.steps.back().token) {
            finishCognitiveStep(c.id,r.pendingContext.token,CognitiveStepResult::Cancelled,"production-goal-ended");
            r.pendingContext.clear();clearNavigation(r);
        }
        if(r.suspendedGather.active() &&std::any_of(p.steps.begin(),p.steps.end(),[&](const auto& step){
            return step.token==r.suspendedGather.goal.token;
        }))r.suspendedGather={};
        p.state=ProductionPlanState::Abandoned;p.reason=abandoned;p.finishedMinute=world_.minute;
        emit("AL2_PRODUCTION_END actor="+std::to_string(c.id)+" goal="+std::to_string(p.id)
             +" reason="+std::to_string(static_cast<int>(p.reason)));
    }
}

bool Simulation::chooseProductionContinuation(Character& c,Runtime& r,
    UnifiedUtilityDecision& decision,const AcceptedCognitiveDecision*& selectedOrigin) {
    maintainProductionPlan(c,r);
    auto& p=r.productionPlan;
    if(!p.open() ||r.pendingContext.active() ||p.steps.size()>=MaxProductionSteps
       ||world_.minute<r.penaltyUntilMinute)return false;
    if(selectedOrigin &&selectedOrigin->contextFingerprint!=p.cognitiveFingerprint)return false;
    if(decision.kind==UnifiedDecisionKind::Social
       &&(decision.social.intent==SocialIntent::Avoid ||decision.social.intent==SocialIntent::Comfort))return false;
    const auto population=settlementPopulation();
    const auto next=nextProductionStep(world_,c,r.pos,p.target,&population);
    if(!executableProductionStep(next)) {
        p.state=ProductionPlanState::Abandoned;p.reason=ProductionPlanReason::NoFeasibleStep;p.finishedMinute=world_.minute;
        return false;
    }
    // Retain a modest, bounded commitment within the SAME decision slot.
    // Body emergencies were evaluated before this function; strong competing
    // ordinary choices still win. No new resident or separate planner runs.
    // A native remembered objective has no model-authored strategic priority.
    // Never let its past curiosity utility displace the live resident choice
    // (including proven personal source learning). Only an accepted thought
    // may retain that thought's bounded strategic commitment.
    const double score=p.cognitiveFingerprint
        ?std::min(1.0,std::max(next.utility,p.priority)+0.04):next.utility;
    if(p.cognitiveFingerprint ?decision.utility>score:decision.utility>=score)return false;
    if(p.cognitiveFingerprint) {
        for(auto it=acceptedCognitiveDecisions_.rbegin();it!=acceptedCognitiveDecisions_.rend();++it)
            if(it->actor==c.id &&it->contextFingerprint==p.cognitiveFingerprint &&it->minute==p.thoughtMinute) {selectedOrigin=&*it;break;}
    }
    decision.kind=UnifiedDecisionKind::Civilization;decision.civilization=next;
    decision.civilization.utility=score;decision.utility=score;
    return true;
}

void Simulation::recordProductionStep(Character& c,Runtime& r,
    PendingContextAction& pending,const AcceptedCognitiveDecision* origin) {
    if(pending.kind!=ContextActionKind::Civilization ||!pending.active())return;
    const auto& d=pending.civilization;
    if(!executableProductionStep(d))return;
    auto& p=r.productionPlan;
    if(p.open() &&!p.steps.empty() &&p.steps.back().result==CognitiveStepResult::Pending
       &&p.steps.back().token!=pending.token)
        finishProductionStep(c.id,p.steps.back().token,CognitiveStepResult::Cancelled,"replanned-before-execution",false);
    if(!p.open()) {
        if(p.id &&world_.minute<p.expiresMinute)return;
        TechniqueId target=TechniqueId::None;
        if((d.intent==CivilizationIntent::Experiment ||d.intent==CivilizationIntent::Craft)
           &&productionToolTechnique(d.technique) &&residentAIToolOutputNeeded(c,d.technique))target=d.technique;
        if(target==TechniqueId::None &&origin &&origin->intent==CognitiveIntentKind::CraftUsefulTools) {
            const auto population=settlementPopulation();
            for(const auto& technology:TechnologyRegistry) {
                const auto t=technology.legacyTechnique;
                if(!productionToolTechnique(t) ||!residentAIToolOutputNeeded(c,t))continue;
                if(sameProductionCommand(d,nextProductionStep(world_,c,r.pos,t,&population))) {target=t;break;}
            }
        }
        if(target==TechniqueId::None)return;
        p={};p.id=pending.token;p.actor=c.id;p.target=target;p.createdMinute=world_.minute;
        p.expiresMinute=world_.minute>std::numeric_limits<int>::max()-ProductionPlanMinutes
            ?std::numeric_limits<int>::max():world_.minute+ProductionPlanMinutes;
        p.targetStock=productionOutputStock(c,target)+1;p.priority=d.utility;
        if(origin &&pending.cognitiveGoal.active()) {
            p.cognitiveFingerprint=origin->contextFingerprint;p.thoughtMinute=origin->minute;
            p.priority=std::max(p.priority,origin->priority);
        }
    }else {
        if(p.steps.size()>=MaxProductionSteps ||p.steps.back().result==CognitiveStepResult::Pending)return;
        const auto population=settlementPopulation();
        const auto next=nextProductionStep(world_,c,r.pos,p.target,&population);
        // Route recovery may select a different proven source, but only for
        // the exact raw ingredient still missing from this concrete goal.
        if(!sameProductionCommand(d,next)
           &&!(d.intent==CivilizationIntent::Gather &&next.intent==CivilizationIntent::Gather
                &&d.material==next.material))return;
    }
    if(p.cognitiveFingerprint &&d.intent==CivilizationIntent::Gather &&pending.gatherGoal.active()) {
        const auto population=settlementPopulation();
        const auto needed=nextProductionStep(world_,c,r.pos,p.target,&population);
        const int held=c.civilization.inventory.count(ItemKind::RawMaterial,d.material);
        if(needed.intent==CivilizationIntent::Gather &&needed.material==d.material
           &&needed.gatherGoalTarget>held) {
            pending.civilization.quantity=std::min(pending.civilization.quantity,needed.gatherGoalTarget-held);
            pending.gatherGoal.targetQuantity=needed.gatherGoalTarget;
        }
    }
    ResidentProductionStep step;step.token=pending.token;step.issuedMinute=world_.minute;
    step.intent=d.intent;step.technique=d.technique;step.material=d.material;step.source=d.resourceNode;
    step.from=step.to=r.pos;step.stockBefore=step.stockAfter=productionStepStock(c,step);
    p.state=ProductionPlanState::Active;p.reason=ProductionPlanReason::None;p.steps.push_back(step);
    emit("AL2_PRODUCTION_STEP actor="+std::to_string(c.id)+" goal="+std::to_string(p.id)
         +" target="+techniqueName(p.target)+" token="+std::to_string(step.token)
         +" action="+civilizationIntentName(step.intent));
}

void Simulation::finishProductionStep(CharacterId actor,std::uint64_t token,
    CognitiveStepResult result,const std::string& reason,bool attempted) {
    const auto it=runtime_.find(actor);if(it==runtime_.end())return;
    auto& r=it->second;auto& p=r.productionPlan;
    if(!p.open() ||p.steps.empty())return;
    auto& s=p.steps.back();if(s.token!=token ||s.result!=CognitiveStepResult::Pending)return;
    const Character* c=nullptr;for(const auto& resident:world_.characters)if(resident.id==actor)c=&resident;
    if(!c)return;
    s.result=result;s.finishedMinute=world_.minute;s.attempted=attempted;s.to=r.pos;
    s.stockAfter=productionStepStock(*c,s);s.reason=productionFailure(world_,*c,s,result,reason,attempted);
    if(result==CognitiveStepResult::Interrupted){p.state=ProductionPlanState::Paused;p.reason=ProductionPlanReason::Survival;}
    if(result==CognitiveStepResult::Succeeded
       &&(s.intent==CivilizationIntent::Craft ||s.intent==CivilizationIntent::Experiment)
       &&s.technique==p.target &&productionOutputStock(*c,p.target)>=p.targetStock) {
        p.state=ProductionPlanState::Completed;p.reason=ProductionPlanReason::None;p.finishedMinute=world_.minute;
    }
    emit("AL2_PRODUCTION_RESULT actor="+std::to_string(actor)+" goal="+std::to_string(p.id)
         +" token="+std::to_string(token)+" result="+std::to_string(static_cast<int>(result))
         +" stock="+std::to_string(s.stockBefore)+"->"+std::to_string(s.stockAfter));
}
} // namespace lifelens
