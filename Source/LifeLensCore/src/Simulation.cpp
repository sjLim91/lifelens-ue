#include "lifelens/Simulation.h"
#include "lifelens/CoreNavigation.h"
#include "lifelens/FamilyProgression.h"
#include "lifelens/InitialPopulation.h"
#include "lifelens/EmotionRuntime.h"
#include "lifelens/PrimitiveFireProgression.h"
#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <utility>
namespace lifelens {
namespace {

Character* findFamilyCharacter(World& world,CharacterId id)
{
    for(auto& character:world.characters) if(character.id==id) return &character;
    return nullptr;
}

double pastRelationshipPenalty(const RomanceBook& romances,CharacterId id)
{
    int ended=0;
    for(const auto& pair:romances.all()){
        if(pair.contains(id) && !pair.active()) ++ended;
    }
    return clampFamilyProgression(static_cast<double>(ended)*0.12);
}

void recordPairLifeEvent(Character& first,Character& second,LifeEventType type,int minute)
{
    recordLifeEvent(first.lifeHistory,type,minute,{second.id});
    recordLifeEvent(second.lifeHistory,type,minute,{first.id});
}

bool shareHousehold(const HouseholdBook& households,CharacterId first,CharacterId second)
{
    const Household* firstHome=households.householdOf(first);
    const Household* secondHome=households.householdOf(second);
    return firstHome!=nullptr && secondHome!=nullptr && firstHome->id==secondHome->id;
}

int latestCohabitationMinute(const Character& character)
{
    const LifeHistoryEntry* event=latestLifeEvent(character.lifeHistory,LifeEventType::CohabitationStarted);
    return event==nullptr ? -1 : event->minute;
}

const char* parentingActionName(ParentingAction action)
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

bool isRoutineDevelopmentalCare(ParentingAction action)
{
    return action==ParentingAction::Play ||
           action==ParentingAction::Educate ||
           action==ParentingAction::Discipline;
}

void advanceFoodSpoilageOneDay(World& world)
{
    // C1-D v1: PlantFood freshness is carried by the existing serialized
    // ItemStack::quality field. Organized storage slows spoilage but does not
    // stop it; no food is created or destroyed except through explicit spoilage.
    constexpr double CarriedFreshnessLossPerDay=0.085;
    constexpr double StoredFreshnessLossPerDay=0.045;

    for(auto& character:world.characters){
        if(!character.alive) continue;
        character.civilization.inventory.agePlantFoodOneDay(
            CarriedFreshnessLossPerDay);
    }
    for(auto& storage:world.storageSites){
        storage.inventory.agePlantFoodOneDay(
            StoredFreshnessLossPerDay);
    }
}

} // namespace

Simulation::Simulation(
    WorldSeed worldSeed,
    PopulationSeed populationSeed,
    WorldGenerationVersion generationVersion,
    SimulationRuleset ruleset)
    :ruleset_(ruleset),world_(worldSeed,populationSeed,generationVersion){}

void Simulation::setupDemo(){
    world_.characters.clear(); world_.objects.clear(); world_.environmentalResidues.clear(); relationships_=RelationshipBook{}; genealogy_=GenealogyBook{}; romances_=RomanceBook{}; households_=HouseholdBook{}; pregnancies_=PregnancyBook{}; births_=BirthBook{}; socialKnowledge_.clear(); runtime_.clear(); logs_.clear(); world_.minute=7*60;
    Character c; c.id=1; c.name="DevResident"; c.personality=Personality::generate(world_.rng);
    std::uniform_real_distribution<double> start(0.10,0.42);
    c.needs={start(world_.rng),start(world_.rng),start(world_.rng),start(world_.rng),start(world_.rng)};
    std::normal_distribution<double> trait(1.0,0.08);
    c.metabolism=std::max(0.8,std::min(1.2,trait(world_.rng))); c.sleepTendency=std::max(0.8,std::min(1.2,trait(world_.rng)));
    world_.characters.push_back(c);
    world_.objects.push_back({1,ObjectKind::Bed,{1,1},std::nullopt,{0,0,-0.055,0,0},16});
    world_.objects.push_back({2,ObjectKind::Toilet,{4,1},std::nullopt,{0,0,0,-0.12,0},7});
    world_.objects.push_back({3,ObjectKind::Sink,{4,3},std::nullopt,{0,-0.085,0,0,-0.055},8});
    world_.objects.push_back({4,ObjectKind::Fridge,{1,4},std::nullopt,{-0.075,0,0,0,0},9});
    world_.objects.push_back({5,ObjectKind::Chair,{2,2},std::nullopt,{0,0,0,0,0},5});
    runtime_[c.id]=Runtime{};
    emit("simulation start seed="+std::to_string(world_.seed));
}

void Simulation::setupSocialDemo(){
    world_.characters.clear(); world_.objects.clear(); world_.environmentalResidues.clear(); relationships_=RelationshipBook{}; genealogy_=GenealogyBook{}; romances_=RomanceBook{}; households_=HouseholdBook{}; pregnancies_=PregnancyBook{}; births_=BirthBook{}; socialKnowledge_.clear(); runtime_.clear(); logs_.clear(); world_.minute=7*60;

    Character a;
    a.id=1; a.name="SocialA";
    a.needs={0.10,0.10,0.10,0.10,0.10};
    a.personality.sociability=0.94;
    a.personality.curiosity=0.72;
    a.personality.introversion=0.08;
    a.personality.agreeableness=0.82;
    a.personality.empathy=0.80;
    a.personality.patience=0.76;

    Character b;
    b.id=2; b.name="SocialB";
    b.needs={0.11,0.10,0.10,0.10,0.10};
    b.personality.sociability=0.82;
    b.personality.curiosity=0.60;
    b.personality.introversion=0.18;
    b.personality.agreeableness=0.78;
    b.personality.empathy=0.84;
    b.personality.patience=0.74;

    world_.characters={a,b};
    world_.objects.push_back({1,ObjectKind::Bed,{1,1},std::nullopt,{0,0,-0.055,0,0},16});
    world_.objects.push_back({2,ObjectKind::Toilet,{4,1},std::nullopt,{0,0,0,-0.12,0},7});
    world_.objects.push_back({3,ObjectKind::Sink,{4,3},std::nullopt,{0,-0.085,0,0,-0.055},8});
    world_.objects.push_back({4,ObjectKind::Fridge,{1,4},std::nullopt,{-0.075,0,0,0,0},9});

    Relationship& aToB=relationships_.getOrCreate(1,2);
    aToB.affection=0.84; aToB.trust=0.82; aToB.comfort=0.80; aToB.familiarity=0.88;
    Relationship& bToA=relationships_.getOrCreate(2,1);
    bToA.affection=0.72; bToA.trust=0.74; bToA.comfort=0.70; bToA.familiarity=0.82;

    runtime_[a.id]=Runtime{};
    runtime_[b.id]=Runtime{};
    emit("social simulation start seed="+std::to_string(world_.seed));
}

void Simulation::setupNewGame(){
    world_.characters.clear();
    world_.objects.clear();
    relationships_=RelationshipBook{};
    genealogy_=GenealogyBook{};
    romances_=RomanceBook{};
    households_=HouseholdBook{};
    pregnancies_=PregnancyBook{};
    births_=BirthBook{};
    socialKnowledge_.clear();
    runtime_.clear();
    logs_.clear();
    world_.minute=8*60;
    world_.resetCivilizationEnvironment();
    // Production NEW GAME begins with nature only. The compatibility World
    // constructor still seeds a utility-test storage/resource baseline, but the
    // production path replaces it with the selected natural chunk and no storage.
    world_.resourceNodes.clear();
    world_.storageSites.clear();
    world_.clearGeneratedNaturalWorld();
    const InitialStartRegionSelection startRegion=world_.establishInitialStartRegion();
    world_.materializeNaturalChunk(startRegion.region.coord);

    // v2+ starts on dry land but materializes its nearby authoritative fresh
    // water source too, so WaterPresentation sees real local hydrology without
    // placing founders inside a centered river/lake/wetland shape.
    if(world_.generationVersion >= 2){
        ChunkCoord freshwaterCoord{};
        if(findNearestFreshSurfaceWaterChunk(
                world_.genesisIdentity(),
                startRegion.region.coord,
                freshwaterCoord,
                FreshSurfaceNeighbourRadiusChunks)){
            world_.materializeNaturalChunk(freshwaterCoord);
        }
    }

    // World randomness and initial-population randomness are separate.
    // Founder generation must not advance the world RNG or affect future chunk
    // baselines merely because names/traits were regenerated.
    world_.rng.seed(world_.seed);
    std::mt19937_64 populationRng(world_.populationSeed);
    world_.characters=generateInitialFounders(populationRng,world_.minute);

    const GridPos startCenter=world_.initialStartRegionCenterGrid();
    const std::array<GridPos,4> founderOffsets={GridPos{-1,-1},GridPos{1,-1},GridPos{-1,1},GridPos{1,1}};
    std::uniform_real_distribution<double> familiarity(0.0,0.04);
    std::size_t founderIndex=0;
    for(const Character& from:world_.characters){
        Runtime initialRuntime;
        const GridPos offset=founderOffsets[std::min(founderIndex,founderOffsets.size()-1)];
        initialRuntime.pos={startCenter.x+offset.x,startCenter.y+offset.y};
        runtime_[from.id]=initialRuntime;
        ++founderIndex;
        for(const Character& to:world_.characters){
            if(from.id==to.id) continue;
            Relationship& relation=relationships_.getOrCreate(from.id,to.id);
            relation.familiarity=familiarity(populationRng);
        }
    }

    emit("new game start seed="+std::to_string(world_.seed)+" populationSeed="+std::to_string(world_.populationSeed)+" founders=4 startChunk=("+std::to_string(startRegion.region.coord.x)+","+std::to_string(startRegion.region.coord.y)+")");
}

ResidentObservation Simulation::observeResident(CharacterId id) const{
    const Character* character=findObservedCharacter(world_,id);
    if(!character) return ResidentObservation{};

    bool hasPhysicalAction=false;
    Goal physicalGoal=Goal::Idle;
    bool socialActive=false;
    SocialIntent socialIntent=SocialIntent::None;
    CharacterId socialTarget=0;

    const auto it=runtime_.find(id);
    if(it!=runtime_.end()){
        const Runtime& r=it->second;
        const bool pendingSocial=character->alive
            && r.pendingContext.active()
            && r.pendingContext.kind==ContextActionKind::Social;
        hasPhysicalAction=character->alive
            && !r.pendingContext.active()
            && !r.plan.empty()
            && !r.socialActive;
        physicalGoal=hasPhysicalAction ? r.goal : Goal::Idle;
        socialActive=pendingSocial || (character->alive && r.socialActive);
        socialIntent=pendingSocial ? r.pendingContext.social.intent
            : (socialActive ? r.socialIntent : SocialIntent::None);
        socialTarget=pendingSocial ? r.pendingContext.social.target
            : (socialActive ? r.socialTarget : 0);
    }

    return buildResidentObservation(
        world_,relationships_,*character,
        hasPhysicalAction,physicalGoal,
        socialActive,socialIntent,socialTarget);
}

ResidentCivilizationActivityObservation Simulation::observeResidentCivilizationActivity(CharacterId id) const{
    ResidentCivilizationActivityObservation dto;
    dto.residentId=id;

    const Character* character=findObservedCharacter(world_,id);
    const auto it=runtime_.find(id);
    if(character==nullptr || !character->alive || it==runtime_.end()) return dto;

    const Runtime& r=it->second;
    if(!r.civilizationActive) return dto;

    dto.active=true;
    dto.kind=civilizationActivityKindFromEvent(r.civilizationEvent.type);
    dto.eventType=r.civilizationEvent.type;
    dto.material=r.civilizationEvent.material;
    dto.item=r.civilizationEvent.item;
    dto.technique=r.civilizationEvent.technique;
    dto.quantity=r.civilizationEvent.quantity;
    dto.minute=r.civilizationActivityMinute;
    dto.resourceNode=r.civilizationResourceNode;
    dto.storage=r.civilizationStorage;
    dto.success=r.civilizationEvent.type!=CivilizationEventType::ExperimentFailed;
    dto.hasSpatialTarget=r.civilizationHasSpatialTarget;
    dto.targetGridX=r.civilizationTargetPos.x;
    dto.targetGridY=r.civilizationTargetPos.y;
    dto.sanitationSiteId=r.civilizationSanitationSiteId;
    return dto;
}

std::vector<ResidentObservation> Simulation::observeAllResidents() const{
    std::vector<ResidentObservation> result;
    result.reserve(world_.characters.size());
    for(const auto& character:world_.characters){
        result.push_back(observeResident(character.id));
    }
    return result;
}

FamilyObservation Simulation::observeFamily(CharacterId id) const{
    const Character* character=findObservedCharacter(world_,id);
    if(!character) return FamilyObservation{};
    return buildFamilyObservation(
        world_,genealogy_,romances_,households_,pregnancies_,*character);
}

WorldOverviewObservation Simulation::observeWorldOverview() const{
    return buildWorldOverviewObservation(world_,households_,romances_,pregnancies_);
}

std::string Simulation::stamp() const{
    const int absolute=world_.minute; const int day=absolute/(24*60)+1; const int md=absolute%(24*60);
    std::ostringstream s; s<<"[Day "<<day<<" "<<std::setfill('0')<<std::setw(2)<<(md/60)<<":"<<std::setw(2)<<(md%60)<<"] "; return s.str();
}
void Simulation::emit(const std::string& message){ const std::string line=stamp()+message; logs_.push_back(line); for(auto& cb:callbacks_) cb(line); }
void Simulation::onEvent(EventCallback cb){ callbacks_.push_back(std::move(cb)); }
SmartObject* Simulation::objectById(ObjectId id){ for(auto& o:world_.objects) if(o.id==id) return &o; return nullptr; }
void Simulation::clearNavigation(Runtime& r){
    r.navigationRoute.clear();
    r.navigationRouteIndex=0;
    r.navigationTarget={};
    r.navigationArrivalRadius=0;
    r.navigationHasTarget=false;
    r.navigationArrived=false;
    r.navigationRouteFailed=false;
    r.navigationNextStepMinute=0;
}

bool Simulation::advanceNavigation(
    Runtime& r,
    GridPos target,
    int arrivalRadius)
{
    const int radius=std::max(0,arrivalRadius);
    const bool targetChanged=
        !r.navigationHasTarget
        || !sameGridPos(r.navigationTarget,target)
        || r.navigationArrivalRadius!=radius;

    if(targetChanged){
        clearNavigation(r);
        r.navigationTarget=target;
        r.navigationArrivalRadius=radius;
        r.navigationHasTarget=true;
    }

    if(gridWithinRadius(r.pos,target,radius)){
        r.navigationArrived=true;
        r.navigationRouteFailed=false;
        return true;
    }

    if(r.navigationRouteFailed) return false;

    if(r.navigationRoute.empty()
       || r.navigationRouteIndex>=r.navigationRoute.size()){
        r.navigationRoute.clear();
        r.navigationRouteIndex=0;
        if(!buildCoreGroundRoute(
                world_,
                r.pos,
                target,
                radius,
                r.navigationRoute)){
            r.navigationRouteFailed=true;
            return false;
        }
    }

    if(world_.minute<r.navigationNextStepMinute){
        return false;
    }

    if(r.navigationRouteIndex<r.navigationRoute.size()){
        r.pos=r.navigationRoute[r.navigationRouteIndex++];
        r.navigationNextStepMinute=
            world_.minute + coreGroundStepIntervalMinutes(world_,r.pos);
    }

    if(gridWithinRadius(r.pos,target,radius)){
        r.navigationArrived=true;
        return true;
    }

    return false;
}

bool Simulation::advancePendingContext(
    Character& actor,
    Runtime& runtime)
{
    PendingContextAction& pending=runtime.pendingContext;
    if(!pending.active()) return false;

    GridPos target=runtime.pos;
    int arrivalRadius=0;
    bool requiresMovement=false;

    switch(pending.kind){
        case ContextActionKind::Civilization:
            if(pending.hasSpatialTarget){
                target=pending.targetPos;
                arrivalRadius=1;
                requiresMovement=true;
            }
            break;

        case ContextActionKind::Social: {
            const auto targetRuntime=runtime_.find(pending.social.target);
            if(targetRuntime==runtime_.end()) return false;
            if(pending.social.intent==SocialIntent::Avoid){
                const GridPos other=targetRuntime->second.pos;
                if(gridWithinRadius(runtime.pos,other,0)){
                    const GridPos candidates[4]={
                        {runtime.pos.x+1,runtime.pos.y},
                        {runtime.pos.x,runtime.pos.y+1},
                        {runtime.pos.x-1,runtime.pos.y},
                        {runtime.pos.x,runtime.pos.y-1}
                    };
                    bool found=false;
                    for(const GridPos candidate:candidates){
                        if(coreGroundTraversable(world_,candidate)){
                            target=candidate;
                            found=true;
                            break;
                        }
                    }
                    if(!found) return false;
                    requiresMovement=true;
                }
            }else{
                if(!pending.hasSpatialTarget){
                    pending.hasSpatialTarget=true;
                    pending.targetPos=targetRuntime->second.pos;
                }
                target=pending.targetPos;
                arrivalRadius=1;
                requiresMovement=true;
            }
            break;
        }

        case ContextActionKind::KnowledgeTeaching: {
            const auto learnerRuntime=
                runtime_.find(pending.knowledgeTeachingTarget);
            if(learnerRuntime==runtime_.end()) return false;
            if(!pending.hasSpatialTarget){
                pending.hasSpatialTarget=true;
                pending.targetPos=learnerRuntime->second.pos;
            }
            target=pending.targetPos;
            arrivalRadius=1;
            requiresMovement=true;
            break;
        }

        case ContextActionKind::Parenting: {
            const auto childRuntime=runtime_.find(pending.parentingTarget);
            if(childRuntime==runtime_.end()) return false;
            if(!pending.hasSpatialTarget){
                pending.hasSpatialTarget=true;
                pending.targetPos=childRuntime->second.pos;
            }
            target=pending.targetPos;
            arrivalRadius=1;
            requiresMovement=true;
            break;
        }

        case ContextActionKind::None:
        default:
            return false;
    }

    if(requiresMovement){
        if(!advanceNavigation(runtime,target,arrivalRadius)){
            return false;
        }
    }

    const auto retargetMovingResident=
        [&](CharacterId targetId)->bool
        {
            const auto targetRuntime=runtime_.find(targetId);
            if(targetRuntime==runtime_.end()) return false;
            if(contextActionNearTarget(runtime.pos,targetRuntime->second.pos,1))
                return false;

            pending.hasSpatialTarget=true;
            pending.targetPos=targetRuntime->second.pos;
            clearNavigation(runtime);
            return true;
        };

    if(pending.kind==ContextActionKind::Social
       && pending.social.intent!=SocialIntent::Avoid
       && retargetMovingResident(pending.social.target)){
        return false;
    }
    if(pending.kind==ContextActionKind::KnowledgeTeaching
       && retargetMovingResident(pending.knowledgeTeachingTarget)){
        return false;
    }
    if(pending.kind==ContextActionKind::Parenting
       && retargetMovingResident(pending.parentingTarget)){
        return false;
    }

    const std::uint64_t token=pending.token;
    const bool completed=
        completeContextAction(actor,runtime,token,runtime.pos);
    if(completed) clearNavigation(runtime);
    return completed;
}

void Simulation::failPlan(Character& character,Runtime& r){
    clearNavigation(r);
    r.plan.clear(); r.actionIndex=0; r.announced=false; r.pendingContext.clear();
    r.socialActive=false; r.socialIntent=SocialIntent::None; r.socialTarget=0;
    r.civilizationActive=false;
    ++r.consecutiveFailures;
    applyActionFailureEmotion(character,r.consecutiveFailures);
    if(r.consecutiveFailures>=3){
        r.penaltyUntilMinute=world_.minute+30;
        r.consecutiveFailures=0;
    }
}
void Simulation::clearRuntimeActivity(Runtime& r){
    clearNavigation(r);
    r.goal=Goal::Idle;
    r.plan.clear();
    r.actionIndex=0;
    r.announced=false;
    r.repeatCount=0;
    r.consecutiveFailures=0;
    r.pendingContext.clear();
    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;
    r.civilizationActive=false;
    r.civilizationEvent=CivilizationEvent{};
    r.civilizationActivityMinute=-1;
    r.civilizationResourceNode=0;
    r.civilizationStorage=0;
    r.civilizationHasSpatialTarget=false;
    r.civilizationTargetPos={};
    r.civilizationSanitationSiteId=0;
}

bool Simulation::tryCivilizationDecision(Character& c,Runtime& r){
    if(!c.alive || !lifeStageProfile(c.lifeStage).canWork || r.pendingContext.active()) return false;
    if(world_.minute%15!=0) return false;

    const UnifiedUtilityDecision decision=chooseUnifiedUtilityDecisionAtPosition(
        world_,c,relationships_,r.pos);
    if(decision.kind!=UnifiedDecisionKind::Civilization || decision.civilization.intent==CivilizationIntent::None) return false;

    PendingContextAction pending;
    pending.token=nextContextActionToken();
    pending.kind=ContextActionKind::Civilization;
    pending.issuedMinute=world_.minute;
    pending.civilization=decision.civilization;
    GridPos target{};
    SanitationSiteId sanitationSiteId=0;
    const bool resolved=resolveCivilizationContextTarget(
        world_,c,decision.civilization,target,sanitationSiteId);
    if(civilizationContextRequiresSpatialTarget(decision.civilization) && !resolved) return false;
    if(resolved){
        pending.hasSpatialTarget=true;
        pending.targetPos=target;
        pending.sanitationSiteId=sanitationSiteId;
    }

    r.pendingContext=pending;
    r.civilizationActive=false;
    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;
    r.goal=Goal::Idle;
    r.plan.clear();
    r.actionIndex=0;
    r.announced=false;

    clearNavigation(r);
    return true;
}

bool Simulation::trySocialDecision(Character& c,Runtime& r){
    if(!c.alive || lifeStageProfile(c.lifeStage).autonomy<0.35 || r.pendingContext.active()) return false;
    if(world_.minute<r.socialCooldownUntilMinute) return false;

    const UnifiedUtilityDecision decision=world_.minute%15==0
        ? chooseUnifiedUtilityDecisionAtPosition(
            world_,c,relationships_,r.pos)
        : chooseUnifiedUtilityDecisionAtPosition(
            world_,c,relationships_,r.pos,0.18,2.0);
    if(decision.kind!=UnifiedDecisionKind::Social || decision.social.intent==SocialIntent::None) return false;

    const Character* target=findCharacter(world_,decision.social.target);
    if(target==nullptr || !target->alive) return false;

    PendingContextAction pending;
    pending.token=nextContextActionToken();
    pending.kind=ContextActionKind::Social;
    pending.issuedMinute=world_.minute;
    pending.social=decision.social;
    const auto targetRuntime=runtime_.find(decision.social.target);
    if(targetRuntime!=runtime_.end() && decision.social.intent!=SocialIntent::Avoid){
        pending.hasSpatialTarget=true;
        pending.targetPos=targetRuntime->second.pos;
    }
    r.pendingContext=pending;
    r.civilizationActive=false;
    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;
    r.goal=Goal::Idle;
    r.plan.clear();
    r.actionIndex=0;
    r.announced=false;

    clearNavigation(r);
    return true;
}

void Simulation::beginPlan(Character& c,Runtime& r){
    if(!c.alive || requiresDirectCare(c.lifeStage)){
        clearRuntimeActivity(r);
        return;
    }
    if(r.pendingContext.active()) return;
    if(world_.minute>=r.penaltyUntilMinute && tryCivilizationDecision(c,r)) return;
    if(world_.minute>=r.penaltyUntilMinute && trySocialDecision(c,r)) return;

    r.civilizationActive=false;
    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;

    Goal chosen=(world_.minute<r.penaltyUntilMinute)?Goal::Idle:chooseGoal(world_,c,ruleset_.utilityAI);
    if(chosen==r.lastGoal){ ++r.repeatCount; } else { r.lastGoal=chosen; r.repeatCount=1; }
    if(r.repeatCount>=5){ chosen=Goal::Idle; r.repeatCount=0; }
    clearNavigation(r);
    r.goal=chosen; r.plan=buildPlan(world_,c,chosen,r.pos); r.actionIndex=0; r.announced=false;
    if(r.plan.empty()){ failPlan(c,r); return; }
    std::ostringstream s; s<<c.name<<" -> "<<goalName(chosen)<<" (need "<<std::fixed<<std::setprecision(2)<<needForGoal(c,chosen)<<")"; emit(s.str());
}

void Simulation::advanceAction(Character& c,Runtime& r){
    if(!c.alive){ clearRuntimeActivity(r); return; }
    if(r.actionIndex>=r.plan.size()) return;
    Action& a=r.plan[r.actionIndex]; SmartObject* obj=a.objectId?objectById(a.objectId):nullptr;
    if(!r.announced){
        if(a.type==ActionType::MoveTo) emit(c.name+" moving to "+goalName(r.goal));
        else if(a.type==ActionType::Use) emit(c.name+" using "+std::string(goalName(r.goal)));
        else if(a.type==ActionType::EmergencyUse) emit(c.name+" using emergency "+std::string(goalName(r.goal))+" fallback");
        r.announced=true;
    }
    switch(a.type){
        case ActionType::FindObject: ++r.actionIndex; r.announced=false; break;
        case ActionType::Reserve:
            if(!obj || (obj->reservedBy && *obj->reservedBy!=c.id)){ failPlan(c,r); return; }
            obj->reservedBy=c.id; ++r.actionIndex; r.announced=false; break;
        case ActionType::MoveTo:
            if(!obj){ failPlan(c,r); return; }
            if(advanceNavigation(r,obj->pos,0)){
                clearNavigation(r);
                a.remainingTicks=0;
                ++r.actionIndex;
                r.announced=false;
            }else if(r.navigationRouteFailed){
                failPlan(c,r);
                return;
            }else{
                a.remainingTicks=std::max(
                    1,
                    static_cast<int>(
                        r.navigationRoute.size()-r.navigationRouteIndex));
            }
            break;
        case ActionType::Use:
            if(!obj){ failPlan(c,r); return; }
            {
            const Needs before=c.needs;
            c.needs.apply(obj->effectPerTick);
            applyNeedResolutionEmotion(c,before,r.goal);
            }
            if(--a.remainingTicks<=0){ ++r.actionIndex; r.announced=false; } break;
        case ActionType::EmergencyUse:
            if(r.goal==Goal::UseToilet){
                const GridPos reliefTarget=r.navigationHasTarget
                    ? r.navigationTarget
                    : deterministicOutdoorReliefPosition(
                        world_.seed,c.id,r.pos);
                if(!advanceNavigation(r,reliefTarget,0)){
                    if(r.navigationRouteFailed){
                        failPlan(c,r);
                    }
                    return;
                }
            }
            {
            ConstructedFacility* settlementSleepFacility=
                r.goal==Goal::Sleep
                    ? bestOperationalSleepFacility(world_,r.pos,1)
                    : nullptr;
            const Needs before=c.needs;
            if(settlementSleepFacility!=nullptr){
                c.needs.apply({
                    0,0,-settlementSleepRecoveryPerTick(*settlementSleepFacility),0,0});
            }else{
                c.needs.apply(emergencyUseEffectPerTick(r.goal));
            }
            applyNeedResolutionEmotion(c,before,r.goal);
            if(a.remainingTicks==1 && settlementSleepFacility!=nullptr){
                applyFacilityWear(
                    *settlementSleepFacility,
                    facilityWearPerUse(settlementSleepFacility->kind));
            }
            }
            if(--a.remainingTicks<=0){
                if(r.goal==Goal::UseToilet){
                    const auto& residue=world_.environmentalResidues.deposit(
                        EnvironmentalResidueKind::HumanWaste,r.pos,c.id,world_.minute,1.0,0.42,3);
                    c.needs.hygiene=Needs::clamp01(c.needs.hygiene+0.025);
                    std::ostringstream consequence;
                    consequence<<c.name<<" left sanitation residue id="<<residue.id
                               <<" at ("<<r.pos.x<<","<<r.pos.y<<") amount="
                               <<std::fixed<<std::setprecision(2)<<residue.amount;
                    emit(consequence.str());
                }
                emit(c.name+" completed "+std::string(goalName(r.goal))+" via emergency fallback");
                clearNavigation(r);
                ++r.actionIndex; r.announced=false; r.consecutiveFailures=0;
            }
            break;
        case ActionType::Release:
            if(obj && obj->reservedBy && *obj->reservedBy==c.id) obj->reservedBy.reset();
            emit(c.name+" completed "+std::string(goalName(r.goal)));
            ++r.actionIndex; r.announced=false; r.consecutiveFailures=0; break;
        case ActionType::Idle:
            if(--a.remainingTicks<=0){ ++r.actionIndex; r.announced=false; } break;
    }
    if(r.actionIndex>=r.plan.size()){
        r.plan.clear(); r.actionIndex=0;
        if(r.civilizationActive){
            r.civilizationActive=false;
        }
        if(r.socialActive){
            r.socialActive=false;
            r.socialIntent=SocialIntent::None;
            r.socialTarget=0;
        }
    }
}

CharacterId Simulation::nextCharacterId() const
{
    CharacterId result=1;
    for(const auto& character:world_.characters) result=std::max(result,character.id+1);
    return result;
}

HouseholdId Simulation::nextHouseholdId() const
{
    HouseholdId result=1;
    for(const auto& household:households_.all()) result=std::max(result,household.id+1);
    return result;
}

std::string Simulation::makeChildName(Sex sex,CharacterId childId) const
{
    static const std::array<const char*,12> maleNames={
        "Yejun","Eunwoo","Juwon","Hajun","Sunwoo","Yunho",
        "Jinwoo","Minho","Woojin","Seungmin","Jisung","Jaeyun"
    };
    static const std::array<const char*,12> femaleNames={
        "Seoa","Arin","Dayeon","Jiyu","Eunseo","Sena",
        "Yeji","Nari","Haeun","Bomin","Somin","Chaeyeon"
    };
    const auto& names=sex==Sex::Female ? femaleNames : maleNames;
    const double roll=deterministicFamilyRoll(world_.seed,childId,static_cast<CharacterId>(sex==Sex::Female ? 2 : 1),0);
    std::size_t index=static_cast<std::size_t>(roll*static_cast<double>(names.size()));
    if(index>=names.size()) index=names.size()-1;
    std::string candidate=names[index];
    bool used=false;
    for(const auto& character:world_.characters) if(character.name==candidate){ used=true; break; }
    if(used) candidate+=std::to_string(childId);
    return candidate;
}

void Simulation::advanceDependentCare()
{
    if(world_.minute%30!=0) return;
    const bool dailyDevelopmentWindow=
        world_.minute%FamilyProgressionDayMinutes==FamilyProgressionDecisionMinuteOfDay;

    for(auto& child:world_.characters){
        if(!child.alive || !isDependentStage(child.lifeStage)) continue;

        const double maxPhysicalNeed=std::max({
            child.needs.hunger,child.needs.thirst,child.needs.sleep,
            child.needs.bladder,child.needs.hygiene});
        const double distress=std::max({
            child.development.stress,child.emotion.sadness,
            child.emotion.anxiety,child.emotion.fear});
        const bool urgentPhysical=maxPhysicalNeed>=0.35 || child.development.health<0.65;
        const bool urgentDistress=distress>=0.40;
        if(!urgentPhysical && !urgentDistress && !dailyDevelopmentWindow) continue;

        Character* chosenCaregiver=nullptr;
        ParentingDecision chosenDecision;
        ParentingContext chosenContext;
        chosenDecision.utility=-1.0;

        const auto considerCaregiver=
            [&](Character* caregiver)
            {
                if(caregiver==nullptr
                   || caregiver->id==child.id
                   || !caregiver->alive
                   || !lifeStageProfile(caregiver->lifeStage).canParent){
                    return;
                }

                auto caregiverRuntime=runtime_.find(caregiver->id);
                if(caregiverRuntime==runtime_.end()
                   || caregiverRuntime->second.pendingContext.active()
                   || !caregiverRuntime->second.plan.empty()){
                    return;
                }

                Relationship& caregiverToChild=
                    relationships_.getOrCreate(caregiver->id,child.id);
                ParentingContext context;
                const double caregiverNeed=std::max({
                    caregiver->needs.hunger,caregiver->needs.thirst,
                    caregiver->needs.sleep,caregiver->needs.bladder,
                    caregiver->needs.hygiene});
                context.timeAvailable=
                    clampDevelopment(1.0-0.65*caregiverNeed);
                context.caregiverStress=
                    clampDevelopment(caregiver->development.stress);
                context.warmth=clampDevelopment(
                    0.35+0.35*caregiver->personality.empathy+
                    0.30*caregiver->personality.patience);
                context.consistency=clampDevelopment(
                    0.35+0.40*caregiver->personality.conscientiousness+
                    0.25*caregiver->personality.patience);
                context.harshness=clampDevelopment(
                    0.08+0.28*caregiver->personality.impulsiveness-
                    0.18*caregiver->personality.patience);
                context.foodAvailable=
                    caregiver->civilization.inventory.count(
                        ItemKind::RawMaterial,MaterialKind::PlantFood)>0;
                context.waterAvailable=
                    caregiver->civilization.inventory.count(
                        ItemKind::RawMaterial,MaterialKind::Water)>0;

                ParentingDecision decision=chooseParentingAction(
                    *caregiver,child,caregiverToChild,context);
                if(!decision.valid) return;
                if(!dailyDevelopmentWindow
                   && isRoutineDevelopmentalCare(decision.action)){
                    return;
                }
                if(!urgentDistress && !dailyDevelopmentWindow
                   && (decision.action==ParentingAction::Hold
                       || decision.action==ParentingAction::Comfort)){
                    return;
                }

                if(decision.utility>chosenDecision.utility){
                    chosenCaregiver=caregiver;
                    chosenDecision=decision;
                    chosenContext=context;
                }
            };

        // Biological parents remain the first-choice caregivers.
        bool hasLivingBiologicalParent=false;
        for(CharacterId parentId:child.parentIds){
            Character* parent=findFamilyCharacter(world_,parentId);
            if(parent!=nullptr && parent->alive){
                hasLivingBiologicalParent=true;
            }
            considerCaregiver(parent);
        }

        // If no biological parent is currently able to care, another living
        // adult in the same household may act as a temporary caregiver. This
        // does not rewrite genealogy or parentIds.
        if(chosenCaregiver==nullptr){
            const Household* childHome=households_.householdOf(child.id);
            if(childHome!=nullptr){
                for(const HouseholdMember& member:childHome->members){
                    if(member.characterId==child.id) continue;
                    if(std::find(
                            child.parentIds.begin(),
                            child.parentIds.end(),
                            member.characterId)!=child.parentIds.end()){
                        continue;
                    }
                    considerCaregiver(
                        findFamilyCharacter(world_,member.characterId));
                }
            }
        }

        // Close living relatives can visit and provide care even when they are
        // in another household. This avoids forcing genealogy or inheritance
        // changes merely to keep a dependent alive.
        if(chosenCaregiver==nullptr){
            for(auto& candidate:world_.characters){
                const KinshipType kinship=
                    genealogy_.relationBetween(candidate.id,child.id);
                if(kinship!=KinshipType::Grandparent
                   && kinship!=KinshipType::Sibling
                   && kinship!=KinshipType::HalfSibling){
                    continue;
                }
                considerCaregiver(&candidate);
            }
        }

        // A truly orphaned dependent with no available household/kin caregiver
        // may receive community care from another eligible adult. This stage is
        // deliberately disabled while any biological parent is still alive.
        if(chosenCaregiver==nullptr && !hasLivingBiologicalParent){
            for(auto& candidate:world_.characters){
                considerCaregiver(&candidate);
            }
        }

        if(chosenCaregiver==nullptr || !chosenDecision.valid) continue;
        auto caregiverRuntime=runtime_.find(chosenCaregiver->id);
        if(caregiverRuntime==runtime_.end()
           || caregiverRuntime->second.pendingContext.active()){
            continue;
        }

        PendingContextAction pending;
        pending.token=nextContextActionToken();
        pending.kind=ContextActionKind::Parenting;
        pending.issuedMinute=world_.minute;
        pending.parentingTarget=child.id;
        pending.parentingAction=chosenDecision.action;
        pending.parentingContext=chosenContext;
        const auto childRuntime=runtime_.find(child.id);
        if(childRuntime!=runtime_.end()){
            pending.hasSpatialTarget=true;
            pending.targetPos=childRuntime->second.pos;
        }
        caregiverRuntime->second.pendingContext=pending;
        caregiverRuntime->second.civilizationActive=false;
        caregiverRuntime->second.socialActive=false;
        caregiverRuntime->second.socialIntent=SocialIntent::None;
        caregiverRuntime->second.socialTarget=0;

        clearNavigation(caregiverRuntime->second);
    }
}

void Simulation::updatePregnanciesAndBirths()
{
    std::vector<CharacterId> dueParents;
    for(auto& character:world_.characters){
        PregnancyState* pregnancy=pregnancies_.activeFor(character.id);
        if(pregnancy==nullptr) continue;
        if(!character.alive){
            pregnancies_.terminate(character.id,world_.minute);
            continue;
        }
        advancePregnancy(*pregnancy,character,world_.minute);
        if(pregnancy->active() && world_.minute>=pregnancy->dueMinute){
            dueParents.push_back(character.id);
        }
    }

    for(CharacterId gestationalId:dueParents){
        PregnancyState* pregnancy=pregnancies_.activeFor(gestationalId);
        if(pregnancy==nullptr || world_.minute<pregnancy->dueMinute) continue;
        const CharacterId partnerId=pregnancy->geneticPartner;
        Character* gestationalParent=findFamilyCharacter(world_,gestationalId);
        Character* partner=findFamilyCharacter(world_,partnerId);
        if(gestationalParent==nullptr || !gestationalParent->alive || partner==nullptr) continue;

        const std::string gestationalName=gestationalParent->name;
        const std::string partnerName=partner->name;
        const CharacterId childId=nextCharacterId();
        const Sex childSex=deterministicFamilyRoll(world_.seed,gestationalId,partnerId,childId)<0.5
            ? Sex::Male : Sex::Female;
        const std::string childName=makeChildName(childSex,childId);
        BirthOutcome outcome=performBirth(
            *gestationalParent,*partner,childId,childName,
            pregnancies_,households_,births_,world_.rng,world_.minute,0.08,&genealogy_);
        if(outcome.result!=BirthResult::Success) continue;

        outcome.child.sex=childSex;
        outcome.child.baseMetabolism=1.0;
        outcome.child.baseSleepTendency=1.0;
        applyLifeStageProfile(outcome.child,LifeStage::Baby);
        outcome.child.lifeCondition=lifeConditionForAge(
            0,outcome.child.genetics.healthPotential,outcome.child.childrenIds.size());

        recordLifeEvent(gestationalParent->lifeHistory,LifeEventType::ChildBorn,world_.minute,{childId,partnerId});
        recordLifeEvent(partner->lifeHistory,LifeEventType::ChildBorn,world_.minute,{childId,gestationalId});

        for(const auto& existing:world_.characters){
            Relationship& childToExisting=relationships_.getOrCreate(childId,existing.id);
            Relationship& existingToChild=relationships_.getOrCreate(existing.id,childId);
            const bool isParent=existing.id==gestationalId || existing.id==partnerId;
            if(isParent){
                childToExisting.affection=0.72;
                childToExisting.trust=0.62;
                childToExisting.comfort=0.68;
                childToExisting.familiarity=0.82;
                existingToChild.affection=0.86;
                existingToChild.trust=0.72;
                existingToChild.comfort=0.78;
                existingToChild.familiarity=0.88;
                existingToChild.commitment=0.82;
            }else{
                childToExisting.familiarity=0.08;
                existingToChild.familiarity=0.08;
            }
        }

        GridPos childPosition=world_.hasInitialStartRegionSelection
            ? world_.initialStartRegionCenterGrid()
            : GridPos{};
        const auto gestationalRuntime=runtime_.find(gestationalId);
        const auto partnerRuntime=runtime_.find(partnerId);
        if(gestationalRuntime!=runtime_.end()) childPosition=gestationalRuntime->second.pos;
        else if(partnerRuntime!=runtime_.end()) childPosition=partnerRuntime->second.pos;

        emit("birth: "+childName+" child of "+gestationalName+" and "+partnerName);
        world_.characters.push_back(std::move(outcome.child));
        Runtime childRuntime;
        childRuntime.pos=childPosition;
        runtime_[childId]=childRuntime;
    }
}

void Simulation::evaluateDailyMortality()
{
    std::vector<CharacterId> dueDeaths;
    for(const auto& character:world_.characters){
        if(character.alive && shouldDieToday(character,world_.seed,world_.minute)){
            dueDeaths.push_back(character.id);
        }
    }

    for(CharacterId id:dueDeaths){
        Character* deceased=findFamilyCharacter(world_,id);
        if(deceased==nullptr || !deceased->alive) continue;

        std::vector<Character*> residents;
        residents.reserve(world_.characters.size());
        for(auto& character:world_.characters) residents.push_back(&character);

        std::vector<CharacterId> formerHouseholdMembers;
        if(const Household* household=households_.householdOf(id)){
            for(const auto& member:household->members){
                if(member.characterId!=id) formerHouseholdMembers.push_back(member.characterId);
            }
        }
        const bool pregnancyEnded=pregnancies_.activeFor(id)!=nullptr;
        const DeathCause cause=inferNaturalDeathCause(*deceased,world_.minute);
        const std::string deceasedName=deceased->name;
        const DeathOutcome outcome=applyDeath(
            *deceased,world_.minute,cause,residents,relationships_,romances_);
        if(!outcome.died) continue;

        if(pregnancyEnded) pregnancies_.terminate(id,world_.minute);
        households_.removeMember(id);
        households_.pruneEmpty();
        for(CharacterId survivorId:formerHouseholdMembers){
            Character* survivor=findFamilyCharacter(world_,survivorId);
            if(survivor!=nullptr && survivor->alive){
                recordLifeEvent(survivor->lifeHistory,LifeEventType::HouseholdChanged,world_.minute,{id});
            }
        }
        if(outcome.survivingPartner!=0){
            Character* survivor=findFamilyCharacter(world_,outcome.survivingPartner);
            if(survivor!=nullptr && survivor->alive){
                recordLifeEvent(survivor->lifeHistory,LifeEventType::PartnerWidowed,world_.minute,{id});
            }
        }

        auto runtimeIt=runtime_.find(id);
        if(runtimeIt!=runtime_.end()) clearRuntimeActivity(runtimeIt->second);
        for(auto& object:world_.objects){
            if(object.reservedBy && *object.reservedBy==id) object.reservedBy.reset();
        }

        emit(deceasedName+" died");
        if(pregnancyEnded){
            emit("pregnancy ended because gestational parent "+deceasedName+" died");
        }
    }
}

void Simulation::evaluateDailyFamilyTransitions()
{
    for(auto& character:world_.characters){
        if(character.alive) advanceAging(character,world_.minute);
    }
    evaluateDailyMortality();

    for(std::size_t i=0;i<world_.characters.size();++i){
        for(std::size_t j=i+1;j<world_.characters.size();++j){
            Character& first=world_.characters[i];
            Character& second=world_.characters[j];
            if(isRomanceProhibitedKinship(genealogy_.relationBetween(first.id,second.id))) continue;
            Relationship& firstToSecond=relationships_.getOrCreate(first.id,second.id);
            Relationship& secondToFirst=relationships_.getOrCreate(second.id,first.id);
            evolveRomanticChemistry(first,second,firstToSecond,secondToFirst);
        }
    }

    struct DatingCandidate {
        CharacterId first=0;
        CharacterId second=0;
        double firstScore=0.0;
        double secondScore=0.0;
        double mutualScore=0.0;
    };
    std::vector<DatingCandidate> candidates;

    for(std::size_t i=0;i<world_.characters.size();++i){
        for(std::size_t j=i+1;j<world_.characters.size();++j){
            Character& first=world_.characters[i];
            Character& second=world_.characters[j];
            if(!first.alive || !second.alive) continue;
            if(isRomanceProhibitedKinship(genealogy_.relationBetween(first.id,second.id))) continue;
            if(!romances_.isAvailable(first.id) || !romances_.isAvailable(second.id)) continue;

            Relationship& firstToSecond=relationships_.getOrCreate(first.id,second.id);
            Relationship& secondToFirst=relationships_.getOrCreate(second.id,first.id);
            const RomanceContext firstContext=autonomousRomanceContext(
                first,second,firstToSecond,pastRelationshipPenalty(romances_,first.id));
            const RomanceContext secondContext=autonomousRomanceContext(
                second,first,secondToFirst,pastRelationshipPenalty(romances_,second.id));
            const RomanceEvaluation firstEval=evaluateRomanceInterest(first,firstToSecond,firstContext,0.60);
            const RomanceEvaluation secondEval=evaluateRomanceInterest(second,secondToFirst,secondContext,0.60);
            if(firstEval.ready && secondEval.ready){
                candidates.push_back({first.id,second.id,firstEval.score,secondEval.score,std::min(firstEval.score,secondEval.score)});
            }
        }
    }

    std::sort(candidates.begin(),candidates.end(),[](const DatingCandidate& a,const DatingCandidate& b){
        if(a.mutualScore!=b.mutualScore) return a.mutualScore>b.mutualScore;
        if(a.first!=b.first) return a.first<b.first;
        return a.second<b.second;
    });

    for(const DatingCandidate& candidate:candidates){
        if(!romances_.isAvailable(candidate.first) || !romances_.isAvailable(candidate.second)) continue;
        if(isRomanceProhibitedKinship(genealogy_.relationBetween(candidate.first,candidate.second))) continue;
        Character* first=findFamilyCharacter(world_,candidate.first);
        Character* second=findFamilyCharacter(world_,candidate.second);
        if(first==nullptr || second==nullptr || !first->alive || !second->alive) continue;
        Relationship& firstToSecond=relationships_.getOrCreate(first->id,second->id);
        Relationship& secondToFirst=relationships_.getOrCreate(second->id,first->id);
        const RomanceContext firstContext=autonomousRomanceContext(
            *first,*second,firstToSecond,pastRelationshipPenalty(romances_,first->id));
        const RomanceContext secondContext=autonomousRomanceContext(
            *second,*first,secondToFirst,pastRelationshipPenalty(romances_,second->id));

        Character* proposer=candidate.firstScore>=candidate.secondScore ? first : second;
        Character* recipient=proposer==first ? second : first;
        Relationship& proposerToRecipient=proposer==first ? firstToSecond : secondToFirst;
        Relationship& recipientToProposer=proposer==first ? secondToFirst : firstToSecond;
        const RomanceContext& proposerContext=proposer==first ? firstContext : secondContext;
        const RomanceContext& recipientContext=proposer==first ? secondContext : firstContext;
        const DatingProposalOutcome outcome=applyDatingProposal(
            *proposer,*recipient,proposerToRecipient,recipientToProposer,
            proposerContext,recipientContext,romances_,world_.minute,0.60,0.60);
        if(outcome.result==DatingProposalResult::Accepted){
            recordPairLifeEvent(*first,*second,LifeEventType::DatingStarted,world_.minute);
            emit(first->name+" and "+second->name+" started dating");
        }
    }

    const std::vector<RomancePair> stagePairs=romances_.all();
    for(const RomancePair& snapshot:stagePairs){
        if(!snapshot.active()) continue;
        Character* first=findFamilyCharacter(world_,snapshot.first);
        Character* second=findFamilyCharacter(world_,snapshot.second);
        if(first==nullptr || second==nullptr || !first->alive || !second->alive) continue;
        if(isRomanceProhibitedKinship(genealogy_.relationBetween(first->id,second->id))) continue;
        Relationship& firstToSecond=relationships_.getOrCreate(first->id,second->id);
        Relationship& secondToFirst=relationships_.getOrCreate(second->id,first->id);

        if(snapshot.stage==RomanceStage::Dating){
            const int datingDuration=world_.minute-snapshot.startedMinute;
            if(datingDuration>=FamilyDatingToCohabitationMinutes && !shareHousehold(households_,first->id,second->id)){
                const CohabitationContext firstContext=autonomousCohabitationContext(*first,*second,firstToSecond);
                const CohabitationContext secondContext=autonomousCohabitationContext(*second,*first,secondToFirst);
                const CohabitationProposalOutcome outcome=applyCohabitationProposal(
                    *first,*second,firstToSecond,secondToFirst,
                    firstContext,secondContext,households_,nextHouseholdId(),true);
                if(outcome.result==CohabitationProposalResult::Accepted){
                    recordPairLifeEvent(*first,*second,LifeEventType::CohabitationStarted,world_.minute);
                    recordPairLifeEvent(*first,*second,LifeEventType::HouseholdChanged,world_.minute);
                    emit(first->name+" and "+second->name+" started cohabiting");
                }
            }

            const int firstCohab=latestCohabitationMinute(*first);
            const int secondCohab=latestCohabitationMinute(*second);
            const int cohabStart=std::max(firstCohab,secondCohab);
            const bool cohabMature=shareHousehold(households_,first->id,second->id) &&
                cohabStart>=0 && world_.minute-cohabStart>=FamilyCohabitationToEngagementMinutes;
            if(datingDuration>=FamilyDatingToEngagementMinutes && cohabMature){
                const double duration=clampFamilyProgression(
                    static_cast<double>(datingDuration)/static_cast<double>(180*FamilyProgressionDayMinutes));
                const MarriageContext firstContext=autonomousMarriageContext(*first,*second,firstToSecond,duration);
                const MarriageContext secondContext=autonomousMarriageContext(*second,*first,secondToFirst,duration);
                const EngagementProposalOutcome outcome=applyEngagementProposal(
                    *first,*second,firstToSecond,secondToFirst,
                    firstContext,secondContext,romances_,world_.minute);
                if(outcome.result==EngagementProposalResult::Accepted){
                    recordPairLifeEvent(*first,*second,LifeEventType::Engaged,world_.minute);
                    emit(first->name+" and "+second->name+" became engaged");
                }
            }
        }else if(snapshot.stage==RomanceStage::Engaged){
            if(snapshot.engagedMinute>=0 && world_.minute-snapshot.engagedMinute>=FamilyEngagementToMarriageMinutes){
                const double duration=clampFamilyProgression(
                    static_cast<double>(world_.minute-snapshot.startedMinute)/static_cast<double>(240*FamilyProgressionDayMinutes));
                const MarriageContext firstContext=autonomousMarriageContext(*first,*second,firstToSecond,duration);
                const MarriageContext secondContext=autonomousMarriageContext(*second,*first,secondToFirst,duration);
                const HouseholdId householdId=shareHousehold(households_,first->id,second->id) ? 0 : nextHouseholdId();
                const MarriageDecisionOutcome outcome=applyMarriageDecision(
                    *first,*second,firstToSecond,secondToFirst,
                    firstContext,secondContext,romances_,households_,world_.minute,householdId);
                if(outcome.result==MarriageDecisionResult::Married){
                    genealogy_.linkSpouses(first->id,second->id);
                    recordPairLifeEvent(*first,*second,LifeEventType::Married,world_.minute);
                    emit(first->name+" and "+second->name+" got married");
                }
            }
        }else if(snapshot.stage==RomanceStage::Married){
            const int marriedDuration=snapshot.marriedMinute>=0
                ? world_.minute-snapshot.marriedMinute
                : 0;
            const double breakdownPressure=
                familyMutualRelationshipBreakdownPressure(
                    firstToSecond,secondToFirst);
            if(marriedDuration>=FamilyMarriageBreakdownGraceMinutes &&
               breakdownPressure>=FamilySeparationPressureThreshold &&
               romances_.separate(first->id,second->id,world_.minute)){
                recordPairLifeEvent(
                    *first,*second,LifeEventType::Separated,world_.minute);
                emit(first->name+" and "+second->name+" separated");
            }
        }else if(snapshot.stage==RomanceStage::Separated){
            const LifeHistoryEntry* firstSeparated=
                latestLifeEvent(first->lifeHistory,LifeEventType::Separated);
            const LifeHistoryEntry* secondSeparated=
                latestLifeEvent(second->lifeHistory,LifeEventType::Separated);
            int separatedMinute=-1;
            if(firstSeparated!=nullptr) separatedMinute=firstSeparated->minute;
            if(secondSeparated!=nullptr){
                separatedMinute=separatedMinute<0
                    ? secondSeparated->minute
                    : std::max(separatedMinute,secondSeparated->minute);
            }

            const double breakdownPressure=
                familyMutualRelationshipBreakdownPressure(
                    firstToSecond,secondToFirst);
            if(separatedMinute>=0 &&
               world_.minute-separatedMinute>=FamilySeparationToDivorceMinutes &&
               breakdownPressure>=FamilyDivorcePressureThreshold &&
               romances_.divorce(first->id,second->id,world_.minute)){
                genealogy_.unlinkSpouses(first->id,second->id);
                recordPairLifeEvent(
                    *first,*second,LifeEventType::Divorced,world_.minute);

                if(shareHousehold(households_,first->id,second->id)){
                    Household* shared=households_.householdOf(first->id);
                    if(shared!=nullptr){
                        const HouseholdId sharedId=shared->id;
                        const double originalMoney=shared->sharedMoney;
                        const double movingMoney=0.5*originalMoney;
                        const std::uint64_t dayEpoch=
                            static_cast<std::uint64_t>(
                                world_.minute/FamilyProgressionDayMinutes);
                        const bool firstLeaves=deterministicFamilyRoll(
                            world_.seed,
                            first->id,
                            second->id,
                            dayEpoch+0xD1A0CEull)<0.5;
                        const CharacterId leavingId=
                            firstLeaves ? first->id : second->id;
                        const HouseholdId newHouseholdId=nextHouseholdId();

                        if(households_.removeMember(leavingId)){
                            Household* remaining=households_.find(sharedId);
                            if(remaining!=nullptr){
                                remaining->sharedMoney=
                                    std::max(0.0,originalMoney-movingMoney);
                            }

                            if(households_.create(
                                    newHouseholdId,
                                    {leavingId},
                                    0,
                                    movingMoney)){
                                households_.pruneEmpty();
                                recordPairLifeEvent(
                                    *first,*second,
                                    LifeEventType::HouseholdChanged,
                                    world_.minute);
                            }else{
                                households_.addMember(sharedId,leavingId);
                                remaining=households_.find(sharedId);
                                if(remaining!=nullptr){
                                    remaining->sharedMoney=originalMoney;
                                }
                            }
                        }
                    }
                }

                emit(first->name+" and "+second->name+" divorced");
            }
        }
    }

    const std::vector<RomancePair> pregnancyPairs=romances_.all();
    for(const RomancePair& pair:pregnancyPairs){
        if(pair.stage!=RomanceStage::Married || pair.marriedMinute<0) continue;
        if(isRomanceProhibitedKinship(genealogy_.relationBetween(pair.first,pair.second))) continue;
        const int daysSinceMarriage=(world_.minute-pair.marriedMinute)/FamilyProgressionDayMinutes;
        if(daysSinceMarriage<30 || ((daysSinceMarriage-30)%FamilyPregnancyAttemptIntervalDays)!=0) continue;

        Character* first=findFamilyCharacter(world_,pair.first);
        Character* second=findFamilyCharacter(world_,pair.second);
        if(first==nullptr || second==nullptr || !first->alive || !second->alive) continue;
        if(first->sex==second->sex) continue;

        Character* gestationalParent=first->sex==Sex::Female ? first : second;
        Character* partner=gestationalParent==first ? second : first;
        if(pregnancies_.activeFor(gestationalParent->id)!=nullptr) continue;

        Relationship& gestationalToPartner=relationships_.getOrCreate(gestationalParent->id,partner->id);
        Relationship& partnerToGestational=relationships_.getOrCreate(partner->id,gestationalParent->id);
        const ReproductiveProfile gestationalProfile=autonomousReproductiveProfile(*gestationalParent,world_.minute);
        const ReproductiveProfile partnerProfile=autonomousReproductiveProfile(*partner,world_.minute);
        const PregnancyContext context=autonomousPregnancyContext(
            *gestationalParent,*partner,gestationalToPartner,partnerToGestational);
        const std::uint64_t epoch=static_cast<std::uint64_t>(daysSinceMarriage/FamilyPregnancyAttemptIntervalDays);
        const double roll=deterministicFamilyRoll(
            world_.seed,gestationalParent->id,partner->id,epoch);
        const PregnancyAttemptOutcome outcome=applyPregnancyAttempt(
            *gestationalParent,*partner,gestationalProfile,partnerProfile,
            gestationalToPartner,partnerToGestational,context,
            pregnancies_,world_.minute,roll);
        if(outcome.result==PregnancyAttemptResult::Conceived){
            recordLifeEvent(gestationalParent->lifeHistory,LifeEventType::PregnancyStarted,world_.minute,{partner->id});
            recordLifeEvent(partner->lifeHistory,LifeEventType::PregnancyStarted,world_.minute,{gestationalParent->id});
            emit(gestationalParent->name+" and "+partner->name+" are expecting a child");
        }
    }
}

void Simulation::advanceAutonomousFamilyProgression()
{
    updatePregnanciesAndBirths();
    if(world_.minute%FamilyProgressionDayMinutes==FamilyProgressionDecisionMinuteOfDay){
        evaluateDailyFamilyTransitions();
    }
}

void Simulation::step(){
    advanceDependentCare();
    for(auto& c:world_.characters){
        Runtime& r=runtime_[c.id];
        if(!c.alive){
            clearRuntimeActivity(r);
            continue;
        }

        c.needs.decay(ruleset_.needs,c.metabolism,c.sleepTendency);
        advanceEmotionOneMinute(c);
        if(requiresDirectCare(c.lifeStage)){
            clearRuntimeActivity(r);
            continue;
        }
        if(r.pendingContext.active()){
            if(contextActionExpired(r.pendingContext,world_.minute)){
                emit(c.name+" context action timed out");
                r.pendingContext.clear();
                clearNavigation(r);
                r.penaltyUntilMinute=std::max(r.penaltyUntilMinute,world_.minute+5);
            }else if(!world_.externalPhysicalExecution){
                advancePendingContext(c,r);
                continue;
            }else{
                continue;
            }
        }
        if(r.plan.empty() && world_.minute%5==0) beginPlan(c,r);
        if(!r.plan.empty()) advanceAction(c,r);
    }
    ++world_.minute;

    // Environmental pressure follows each resident's authoritative Core runtime
    // position. Do not apply one start-region climate to residents who have
    // moved into a different chunk.
    for(auto& character:world_.characters){
        if(!character.alive) continue;
        const auto runtimeIt=runtime_.find(character.id);
        const GridPos position=runtimeIt!=runtime_.end()
            ? runtimeIt->second.pos
            : world_.initialStartRegionCenterGrid();
        applyResidentEnvironmentalNeedPressure(world_,character,position);
    }

    advanceSettlementFacilityWearOneMinute(world_);
    advancePrimitiveFireOneMinute(world_);
    world_.environmentalResidues.advanceToMinute(world_.minute);
    if(world_.minute%(24*60)==0){
        regenerateCivilizationEnvironment(world_);
        advanceFoodSpoilageOneDay(world_);
    }
    advanceCivilizationKnowledgeTeaching();
    advanceAutonomousFamilyProgression();
}
void Simulation::runMinutes(int minutes){ for(int i=0;i<minutes;++i) step(); }
}
