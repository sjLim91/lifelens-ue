#include "lifelens/Simulation.h"
#include "lifelens/EmotionRuntime.h"
#include "lifelens/ToolEffectiveness.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace lifelens {
namespace {

Character* findContextCharacter(World& world,CharacterId id)
{
    for(auto& character:world.characters) if(character.id==id) return &character;
    return nullptr;
}

const char* contextParentingActionName(ParentingAction action)
{
    switch(action){
        case ParentingAction::Feed: return "Feed";
        case ParentingAction::PutToSleep: return "PutToSleep";
        case ParentingAction::Bathe: return "Bathe";
        case ParentingAction::ToiletAssist: return "ToiletAssist";
        case ParentingAction::Hold: return "Hold";
        case ParentingAction::Play: return "Play";
        case ParentingAction::Educate: return "Educate";
        case ParentingAction::Discipline: return "Discipline";
        case ParentingAction::Comfort: return "Comfort";
        case ParentingAction::HealthCare: return "HealthCare";
    }
    return "Care";
}

bool sameGridPosition(GridPos a,GridPos b)
{
    return a.x==b.x && a.y==b.y;
}

bool validatePendingSanitationSite(const World& world,const PendingContextAction& pending)
{
    if(pending.sanitationSiteId==0) return false;
    const PrimitiveSanitationSite* site=findPrimitiveSanitationSite(
        world.primitiveSanitationSites,pending.sanitationSiteId);
    return site!=nullptr && site->active && sameGridPosition(site->pos,pending.targetPos);
}

bool validatePendingDesignatedAreaTarget(const World& world,const PendingContextAction& pending)
{
    return pending.hasSpatialTarget
        && activePrimitiveSanitationSite(world.primitiveSanitationSites)==nullptr
        && world.environmentalResidues.exposureAt(pending.targetPos)
            < PrimitiveSanitationCleanSiteExposureLimit;
}

CivilizationExecutionResult establishPendingDesignatedSanitationArea(
    World& world,
    Character& actor,
    GridPos targetPos)
{
    CivilizationExecutionResult result;
    if(!actor.civilization.knowledge.knowsAtLeast(
        TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible)) return result;
    if(activePrimitiveSanitationSite(world.primitiveSanitationSites)!=nullptr) return result;
    if(world.environmentalResidues.exposureAt(targetPos)>=PrimitiveSanitationCleanSiteExposureLimit)
        return result;

    PrimitiveSanitationSite site;
    site.id=nextPrimitiveSanitationSiteId(world.primitiveSanitationSites);
    site.kind=PrimitiveSanitationSiteKind::DesignatedArea;
    site.pos=targetPos;
    site.establishedBy=actor.id;
    site.establishedMinute=world.minute;
    site.active=true;
    site.useCount=0;
    world.primitiveSanitationSites.push_back(site);

    result.executed=true;
    result.success=true;
    result.sanitationSiteId=site.id;
    result.sanitationSitePos=site.pos;
    result.craft.success=true;
    result.craft.event.actor=actor.id;
    result.craft.event.type=CivilizationEventType::Crafted;
    result.craft.event.technique=TechniqueId::DesignatedSanitationArea;
    result.event=result.craft.event;
    actor.civilization.knowledge.recordSuccessfulUse(TechniqueId::DesignatedSanitationArea);
    actor.civilization.craftingSkill=clampCivilization01(
        actor.civilization.craftingSkill+0.004);
    return result;
}

} // namespace

PendingContextActionObservation Simulation::observePendingContextAction(CharacterId id) const
{
    const auto runtimeIt=runtime_.find(id);
    if(runtimeIt==runtime_.end()) return PendingContextActionObservation{};
    const Character* character=findObservedCharacter(world_,id);
    if(character==nullptr || !character->alive) return PendingContextActionObservation{};
    if(contextActionExpired(runtimeIt->second.pendingContext,world_.minute))
        return PendingContextActionObservation{};
    return lifelens::observePendingContextAction(id,runtimeIt->second.pendingContext);
}

bool Simulation::completeExternalContextAction(
    CharacterId id,
    std::uint64_t token,
    GridPos resolvedPosition)
{
    if(!world_.externalPhysicalExecution || token==0) return false;
    auto runtimeIt=runtime_.find(id);
    if(runtimeIt==runtime_.end()) return false;
    Character* actor=findContextCharacter(world_,id);
    if(actor==nullptr || !actor->alive) return false;
    return completeContextAction(*actor,runtimeIt->second,token,resolvedPosition);
}

bool Simulation::completeContextAction(
    Character& actor,
    Runtime& runtime,
    std::uint64_t token,
    GridPos resolvedPosition)
{
    PendingContextAction& pending=runtime.pendingContext;
    if(!actor.alive || !pending.active() || pending.token!=token) return false;
    if(contextActionExpired(pending,world_.minute)){
        pending.clear();
        return false;
    }
    if(pending.hasSpatialTarget && !contextActionNearTarget(resolvedPosition,pending.targetPos,1))
        return false;

    bool completed=false;

    switch(pending.kind){
        case ContextActionKind::Social: {
            Character* target=findContextCharacter(world_,pending.social.target);
            if(target==nullptr || !target->alive || target->id==actor.id){
                pending.clear();
                return false;
            }

            const auto targetRuntime=runtime_.find(target->id);
            if(targetRuntime==runtime_.end()){
                pending.clear();
                return false;
            }
            if(pending.social.intent==SocialIntent::Avoid){
                const int dx=std::abs(resolvedPosition.x-targetRuntime->second.pos.x);
                const int dy=std::abs(resolvedPosition.y-targetRuntime->second.pos.y);
                if(std::max(dx,dy)<1) return false;
            }else if(!contextActionNearTarget(resolvedPosition,targetRuntime->second.pos,1)){
                return false;
            }

            const DecisionExecutionResult result=executeSocialDecision(
                world_,relationships_,actor.id,pending.social,"simulation");
            if(!result.socialExecuted){
                pending.clear();
                return false;
            }
            if(result.generatedSocialEvent){
                recordSocialEvent(result.socialEvent);
            }

            int cooldown=20;
            if(pending.social.intent==SocialIntent::Avoid) cooldown=15;
            else if(pending.social.intent==SocialIntent::Repair) cooldown=30;
            else if(pending.social.intent==SocialIntent::Comfort) cooldown=25;
            runtime.socialCooldownUntilMinute=world_.minute+cooldown;
            runtime.pos=resolvedPosition;

            std::ostringstream log;
            log<<actor.name<<" -> "<<socialIntentName(pending.social.intent)<<" "<<target->name
               <<" (social utility "<<std::fixed<<std::setprecision(2)<<pending.social.utility<<")";
            emit(log.str());
            completed=true;
            break;
        }

        case ContextActionKind::Civilization: {
            CivilizationUtilityDecision decision=pending.civilization;
            GatherToolUseProfile gatherTool;
            if(decision.intent==CivilizationIntent::Gather){
                gatherTool=inspectGatherTool(actor.civilization.inventory,decision.material);
                if(gatherTool.available){
                    decision.quantity=gatheringQuantityWithTool(
                        actor.civilization.inventory,
                        decision.material,
                        decision.quantity);
                }
            }

            const ResourceNodeId resourceNode=decision.resourceNode;
            const StorageId storage=decision.storage;
            const bool hadSpatialTarget=pending.hasSpatialTarget;
            const GridPos targetPos=pending.targetPos;
            const SanitationSiteId targetSite=pending.sanitationSiteId;

            if(decision.intent==CivilizationIntent::Experiment
               && decision.experiment==ExperimentKind::DesignateSanitationArea
               && !validatePendingDesignatedAreaTarget(world_,pending)){
                pending.clear();
                return false;
            }
            if((decision.intent==CivilizationIntent::Experiment
                    && decision.experiment==ExperimentKind::DigSanitationPit)
               || (decision.intent==CivilizationIntent::Craft
                    && decision.technique==TechniqueId::DugSanitationPit)){
                if(!validatePendingSanitationSite(world_,pending)){
                    pending.clear();
                    return false;
                }
            }

            CivilizationExecutionResult result;
            if(decision.intent==CivilizationIntent::Craft
               && decision.technique==TechniqueId::DesignatedSanitationArea){
                if(!validatePendingDesignatedAreaTarget(world_,pending)){
                    pending.clear();
                    return false;
                }
                result=establishPendingDesignatedSanitationArea(
                    world_,actor,pending.targetPos);
            }else{
                result=executeCivilizationDecision(world_,actor,decision);
            }
            if(!result.executed){
                pending.clear();
                return false;
            }

            if(decision.intent==CivilizationIntent::Gather
               && result.event.type==CivilizationEventType::Gathered
               && result.event.quantity>0
               && gatherTool.available){
                GatherToolUseProfile consumedTool;
                if(consumeGatherToolUse(
                    actor.civilization.inventory,
                    decision.material,
                    &consumedTool)){
                    gatherTool=consumedTool;
                }
            }

            runtime.civilizationActive=true;
            runtime.civilizationEvent=result.event;
            runtime.civilizationActivityMinute=world_.minute;
            runtime.civilizationResourceNode=resourceNode;
            runtime.civilizationStorage=storage;
            runtime.civilizationHasSpatialTarget=hadSpatialTarget || result.sanitationSiteId!=0;
            runtime.civilizationTargetPos=result.sanitationSiteId!=0
                ? result.sanitationSitePos
                : targetPos;
            runtime.civilizationSanitationSiteId=result.sanitationSiteId!=0
                ? static_cast<std::uint64_t>(result.sanitationSiteId)
                : static_cast<std::uint64_t>(targetSite);
            runtime.pos=resolvedPosition;
            processCivilizationKnowledgeEvent(actor,result.event);
            applyCivilizationOutcomeEmotion(
                actor,result.success,
                result.event.type==CivilizationEventType::Discovered);

            std::ostringstream log;
            log<<actor.name<<" -> Civilization "<<civilizationIntentName(decision.intent);
            switch(result.event.type){
                case CivilizationEventType::Gathered:
                    log<<" "<<materialName(result.event.material)<<" x"<<result.event.quantity;
                    break;
                case CivilizationEventType::Stored:
                    log<<" "<<materialName(result.event.material)<<" x"<<result.event.quantity;
                    break;
                case CivilizationEventType::ExperimentFailed:
                    log<<" failed "<<techniqueName(result.event.technique);
                    break;
                case CivilizationEventType::Discovered:
                    log<<" discovered "<<techniqueName(result.event.technique);
                    break;
                case CivilizationEventType::Crafted:
                    log<<" crafted "<<techniqueName(result.event.technique);
                    break;
                default:
                    break;
            }
            if(gatherTool.used){
                log<<" tool="<<toolItemName(gatherTool.tool.kind)
                   <<" q="<<std::fixed<<std::setprecision(2)<<gatherTool.tool.quality
                   <<" durability="<<gatherTool.durabilityBefore
                   <<"->"<<gatherTool.durabilityAfter;
                if(gatherTool.broken) log<<" broken";
            }
            log<<" (civilization utility "<<std::fixed<<std::setprecision(2)<<decision.utility<<")";
            emit(log.str());
            completed=true;
            break;
        }

        case ContextActionKind::KnowledgeTeaching: {
            Character* learner=findContextCharacter(world_,pending.knowledgeTeachingTarget);
            if(learner==nullptr || !learner->alive || learner->id==actor.id){
                pending.clear();
                return false;
            }
            const auto learnerRuntime=runtime_.find(learner->id);
            if(learnerRuntime==runtime_.end()
               || !contextActionNearTarget(resolvedPosition,learnerRuntime->second.pos,1)){
                return false;
            }

            const TechniqueId technique=pending.knowledgeTeachingTechnique;
            if(technique==TechniqueId::None
               || !actor.civilization.knowledge.knowsAtLeast(
                    technique,KnowledgeLevel::Reproducible)
               || learner->civilization.knowledge.knowsAtLeast(
                    technique,KnowledgeLevel::Reproducible)
               || bestTechniqueFactForTeaching(
                    socialKnowledge_,actor.id,learner->id,technique)==nullptr){
                pending.clear();
                return false;
            }

            const TechniqueTransmissionOutcome outcome=teachTechnique(
                socialKnowledge_,actor,*learner,technique,relationships_,
                world_.seed,world_.minute,
                static_cast<std::uint64_t>(std::max(0,pending.issuedMinute/60)));

            if(outcome.result==TechniqueTeachingResult::Invalid
               || outcome.result==TechniqueTeachingResult::NoFact
               || outcome.result==TechniqueTeachingResult::DuplicateOrLoop
               || outcome.result==TechniqueTeachingResult::AlreadyKnown){
                pending.clear();
                return false;
            }

            runtime.pos=resolvedPosition;
            runtime.socialCooldownUntilMinute=world_.minute+20;

            std::ostringstream log;
            if(outcome.result==TechniqueTeachingResult::Advanced){
                log<<actor.name<<" taught "<<learner->name<<" "
                   <<techniqueName(technique)<<" -> knowledge "
                   <<static_cast<int>(outcome.before)<<"->"<<static_cast<int>(outcome.after);
            }else if(outcome.result==TechniqueTeachingResult::ComprehensionFailed){
                log<<actor.name<<" tried teaching "<<learner->name<<" "
                   <<techniqueName(technique)<<" but comprehension failed";
            }else{
                log<<actor.name<<" tried teaching "<<learner->name<<" "
                   <<techniqueName(technique)<<" but trust/clarity was too weak";
            }
            log<<" (teaching "<<std::fixed<<std::setprecision(2)
               <<pending.knowledgeTeachingScore<<")";
            emit(log.str());
            completed=true;
            break;
        }

        case ContextActionKind::Parenting: {
            Character* child=findContextCharacter(world_,pending.parentingTarget);
            if(child==nullptr || !child->alive || !isParentOf(actor,*child)){
                pending.clear();
                return false;
            }
            const auto childRuntime=runtime_.find(child->id);
            if(childRuntime==runtime_.end()
               || !contextActionNearTarget(resolvedPosition,childRuntime->second.pos,1)){
                return false;
            }

            ParentingContext context=pending.parentingContext;
            context.foodAvailable=actor.civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::PlantFood)>0;
            context.waterAvailable=actor.civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Water)>0;
            const bool consumeFood=pending.parentingAction==ParentingAction::Feed
                && context.foodAvailable && child->needs.hunger>0.05;
            const bool consumeWater=pending.parentingAction==ParentingAction::Feed
                && context.waterAvailable && child->needs.thirst>0.05;

            Relationship& parentToChild=relationships_.getOrCreate(actor.id,child->id);
            Relationship& childToParent=relationships_.getOrCreate(child->id,actor.id);
            const ParentingResult result=applyParentingAction(
                actor,*child,parentToChild,childToParent,pending.parentingAction,context);
            if(result!=ParentingResult::Performed){
                pending.clear();
                return false;
            }

            for(auto& runtimeEntry:runtime_){
                if(runtimeEntry.first==actor.id) continue;
                PendingContextAction& competing=runtimeEntry.second.pendingContext;
                if(competing.active()
                   && competing.kind==ContextActionKind::Parenting
                   && competing.parentingTarget==child->id){
                    competing.clear();
                }
            }

            if(consumeFood){
                actor.civilization.inventory.remove(
                    ItemKind::RawMaterial,MaterialKind::PlantFood,1);
            }
            if(consumeWater){
                actor.civilization.inventory.remove(
                    ItemKind::RawMaterial,MaterialKind::Water,1);
            }

            runtime.pos=resolvedPosition;
            if(pending.parentingAction==ParentingAction::ToiletAssist){
                childRuntime->second.pos=resolvedPosition;
                const auto& residue=world_.environmentalResidues.deposit(
                    EnvironmentalResidueKind::HumanWaste,resolvedPosition,child->id,
                    world_.minute,1.0,0.42,3);
                emit(child->name+" left sanitation residue id="+std::to_string(residue.id)+
                     " during assisted toilet care");
            }
            emit(actor.name+" cared for "+child->name+" -> "+
                 std::string(contextParentingActionName(pending.parentingAction)));
            completed=true;
            break;
        }

        case ContextActionKind::None:
        default:
            pending.clear();
            return false;
    }

    if(!completed) return false;

    pending.clear();
    runtime.goal=Goal::Idle;
    runtime.plan={{ActionType::Idle,0,5}};
    runtime.actionIndex=0;
    runtime.announced=false;
    runtime.consecutiveFailures=0;
    runtime.socialActive=false;
    runtime.socialIntent=SocialIntent::None;
    runtime.socialTarget=0;
    return true;
}

} // namespace lifelens
