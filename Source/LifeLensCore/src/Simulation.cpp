#include "lifelens/Simulation.h"
#include "lifelens/FamilyProgression.h"
#include "lifelens/InitialPopulation.h"
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
        hasPhysicalAction=!r.plan.empty() && !r.socialActive;
        physicalGoal=r.goal;
        socialActive=r.socialActive;
        socialIntent=r.socialIntent;
        socialTarget=r.socialTarget;
    }

    return buildResidentObservation(
        world_,relationships_,*character,
        hasPhysicalAction,physicalGoal,
        socialActive,socialIntent,socialTarget);
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
void Simulation::failPlan(Runtime& r){ r.plan.clear(); r.actionIndex=0; r.announced=false; r.socialActive=false; r.socialIntent=SocialIntent::None; r.socialTarget=0; ++r.consecutiveFailures; if(r.consecutiveFailures>=3){r.penaltyUntilMinute=world_.minute+30;r.consecutiveFailures=0;} }

bool Simulation::tryCivilizationDecision(Character& c,Runtime& r){
    if(world_.minute%15!=0) return false;

    const UnifiedUtilityDecision decision=chooseUnifiedUtilityDecision(world_,c,relationships_);
    if(decision.kind!=UnifiedDecisionKind::Civilization || decision.civilization.intent==CivilizationIntent::None) return false;

    const CivilizationExecutionResult result=executeCivilizationDecision(world_,c,decision.civilization);
    if(!result.executed) return false;
    c.lastCivilizationEvent=result.event;
    c.lastCivilizationActivityMinute=world_.minute;
    processCivilizationKnowledgeEvent(c,result.event);

    std::ostringstream s;
    s<<c.name<<" -> Civilization "<<civilizationIntentName(decision.civilization.intent);
    switch(result.event.type){
        case CivilizationEventType::Gathered:
            s<<" "<<materialName(result.event.material)<<" x"<<result.event.quantity;
            break;
        case CivilizationEventType::Stored:
            s<<" "<<materialName(result.event.material)<<" x"<<result.event.quantity;
            break;
        case CivilizationEventType::ExperimentFailed:
            s<<" failed "<<techniqueName(result.event.technique);
            break;
        case CivilizationEventType::Discovered:
            s<<" discovered "<<techniqueName(result.event.technique);
            break;
        case CivilizationEventType::Crafted:
            s<<" crafted "<<techniqueName(result.event.technique);
            break;
        default:
            break;
    }
    s<<" (civilization utility "<<std::fixed<<std::setprecision(2)<<decision.civilization.utility<<")";
    emit(s.str());

    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;
    r.goal=Goal::Idle;
    r.plan={{ActionType::Idle,0,5}};
    r.actionIndex=0;
    r.announced=false;
    r.consecutiveFailures=0;
    return true;
}

bool Simulation::trySocialDecision(Character& c,Runtime& r){
    if(world_.minute<r.socialCooldownUntilMinute) return false;

    const UnifiedUtilityDecision decision=world_.minute%15==0
        ? chooseUnifiedUtilityDecision(world_,c,relationships_)
        : chooseUnifiedUtilityDecision(world_,c,relationships_,0.18,2.0);
    if(decision.kind!=UnifiedDecisionKind::Social || decision.social.intent==SocialIntent::None) return false;

    const Character* targetBefore=findCharacter(world_,decision.social.target);
    const std::string targetName=targetBefore?targetBefore->name:std::to_string(decision.social.target);
    const DecisionExecutionResult result=executeSocialDecision(world_,relationships_,c.id,decision.social,"simulation");
    if(!result.socialExecuted) return false;

    int cooldown=20;
    if(decision.social.intent==SocialIntent::Avoid) cooldown=15;
    else if(decision.social.intent==SocialIntent::Repair) cooldown=30;
    else if(decision.social.intent==SocialIntent::Comfort) cooldown=25;
    r.socialCooldownUntilMinute=world_.minute+cooldown;
    r.socialActive=true;
    r.socialIntent=decision.social.intent;
    r.socialTarget=decision.social.target;

    std::ostringstream s;
    s<<c.name<<" -> "<<socialIntentName(decision.social.intent)<<" "<<targetName
     <<" (social utility "<<std::fixed<<std::setprecision(2)<<decision.social.utility<<")";
    emit(s.str());

    r.goal=Goal::Idle;
    r.plan={{ActionType::Idle,0,5}};
    r.actionIndex=0;
    r.announced=false;
    r.consecutiveFailures=0;
    return true;
}

void Simulation::beginPlan(Character& c,Runtime& r){
    if(world_.minute>=r.penaltyUntilMinute && tryCivilizationDecision(c,r)) return;
    if(world_.minute>=r.penaltyUntilMinute && trySocialDecision(c,r)) return;

    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;

    Goal chosen=(world_.minute<r.penaltyUntilMinute)?Goal::Idle:chooseGoal(world_,c,ruleset_.utilityAI);
    if(chosen==r.lastGoal){ ++r.repeatCount; } else { r.lastGoal=chosen; r.repeatCount=1; }
    if(r.repeatCount>=5){ chosen=Goal::Idle; r.repeatCount=0; }
    r.goal=chosen; r.plan=buildPlan(world_,c,chosen,r.pos); r.actionIndex=0; r.announced=false;
    if(r.plan.empty()){ failPlan(r); return; }
    std::ostringstream s; s<<c.name<<" -> "<<goalName(chosen)<<" (need "<<std::fixed<<std::setprecision(2)<<needForGoal(c,chosen)<<")"; emit(s.str());
}

void Simulation::advanceAction(Character& c,Runtime& r){
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
            if(!obj || (obj->reservedBy && *obj->reservedBy!=c.id)){ failPlan(r); return; }
            obj->reservedBy=c.id; ++r.actionIndex; r.announced=false; break;
        case ActionType::MoveTo:
            if(--a.remainingTicks<=0){ if(obj) r.pos=obj->pos; ++r.actionIndex; r.announced=false; } break;
        case ActionType::Use:
            if(!obj){ failPlan(r); return; }
            c.needs.apply(obj->effectPerTick);
            if(--a.remainingTicks<=0){ ++r.actionIndex; r.announced=false; } break;
        case ActionType::EmergencyUse:
            c.needs.apply(emergencyUseEffectPerTick(r.goal));
            if(--a.remainingTicks<=0){
                if(r.goal==Goal::UseToilet){
                    r.pos=deterministicOutdoorReliefPosition(world_.seed,c.id,r.pos);
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

void Simulation::updatePregnanciesAndBirths()
{
    std::vector<CharacterId> dueParents;
    for(auto& character:world_.characters){
        PregnancyState* pregnancy=pregnancies_.activeFor(character.id);
        if(pregnancy==nullptr) continue;
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
        if(gestationalParent==nullptr || partner==nullptr) continue;

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
        outcome.child.baseMetabolism=outcome.child.metabolism;
        outcome.child.baseSleepTendency=outcome.child.sleepTendency;
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

        emit("birth: "+childName+" child of "+gestationalName+" and "+partnerName);
        world_.characters.push_back(std::move(outcome.child));
        runtime_[childId]=Runtime{};
    }
}

void Simulation::evaluateDailyFamilyTransitions()
{
    for(auto& character:world_.characters){
        if(character.alive) advanceAging(character,world_.minute);
    }

    for(std::size_t i=0;i<world_.characters.size();++i){
        for(std::size_t j=i+1;j<world_.characters.size();++j){
            Character& first=world_.characters[i];
            Character& second=world_.characters[j];
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
        Character* first=findFamilyCharacter(world_,candidate.first);
        Character* second=findFamilyCharacter(world_,candidate.second);
        if(first==nullptr || second==nullptr) continue;
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
        }
    }

    const std::vector<RomancePair> pregnancyPairs=romances_.all();
    for(const RomancePair& pair:pregnancyPairs){
        if(pair.stage!=RomanceStage::Married || pair.marriedMinute<0) continue;
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
    for(auto& c:world_.characters){
        c.needs.decay(ruleset_.needs,c.metabolism,c.sleepTendency);
        Runtime& r=runtime_[c.id];
        if(r.plan.empty() && world_.minute%5==0) beginPlan(c,r);
        if(!r.plan.empty()) advanceAction(c,r);
    }
    ++world_.minute;
    world_.environmentalResidues.advanceToMinute(world_.minute);
    if(world_.minute%(24*60)==0) regenerateCivilizationEnvironment(world_);
    advanceCivilizationKnowledgeTeaching();
    advanceAutonomousFamilyProgression();
}
void Simulation::runMinutes(int minutes){ for(int i=0;i<minutes;++i) step(); }
}
