#include "lifelens/Simulation.h"

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
                if(contextActionNearTarget(resolvedPosition,targetRuntime->second.pos,0)) return false;
            }else if(!contextActionNearTarget(resolvedPosition,targetRuntime->second.pos,1)){
                return false;
            }

            const DecisionExecutionResult result=executeSocialDecision(
                world_,relationships_,actor.id,pending.social,"simulation");
            if(!result.socialExecuted){
                pending.clear();
                return false;
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
            const CivilizationUtilityDecision decision=pending.civilization;
            const ResourceNodeId resourceNode=decision.resourceNode;
            const StorageId storage=decision.storage;
            const bool hadSpatialTarget=pending.hasSpatialTarget;
            const GridPos targetPos=pending.targetPos;
            const SanitationSiteId targetSite=pending.sanitationSiteId;

            const CivilizationExecutionResult result=executeCivilizationDecision(world_,actor,decision);
            if(!result.executed){
                pending.clear();
                return false;
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
            log<<" (civilization utility "<<std::fixed<<std::setprecision(2)<<decision.utility<<")";
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
