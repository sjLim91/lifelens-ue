#include "lifelens/Simulation.h"
#include "lifelens/InitialPopulation.h"
#include <iomanip>
#include <sstream>
namespace lifelens {
Simulation::Simulation(std::uint64_t seed):world_(seed){}

void Simulation::setupDemo(){
    world_.characters.clear(); world_.objects.clear(); relationships_=RelationshipBook{}; genealogy_=GenealogyBook{}; romances_=RomanceBook{}; households_=HouseholdBook{}; pregnancies_=PregnancyBook{}; runtime_.clear(); logs_.clear(); world_.minute=7*60;
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
    world_.characters.clear(); world_.objects.clear(); relationships_=RelationshipBook{}; genealogy_=GenealogyBook{}; romances_=RomanceBook{}; households_=HouseholdBook{}; pregnancies_=PregnancyBook{}; runtime_.clear(); logs_.clear(); world_.minute=7*60;

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
    runtime_.clear();
    logs_.clear();
    world_.minute=8*60;

    // A formal New Game must be reproducible from WorldSeed. Re-seeding here
    // ensures repeated setup with the same world seed cannot inherit RNG state
    // from a previous run.
    world_.rng.seed(world_.seed);
    world_.characters=generateInitialFounders(world_.rng,world_.minute);

    // Minimal shared living-space affordances keep the new population runnable
    // by the existing Needs/Planner loop without assigning social/family roles.
    world_.objects.push_back({1,ObjectKind::Bed,{1,1},std::nullopt,{0,0,-0.055,0,0},16});
    world_.objects.push_back({2,ObjectKind::Bed,{2,1},std::nullopt,{0,0,-0.055,0,0},16});
    world_.objects.push_back({3,ObjectKind::Bed,{3,1},std::nullopt,{0,0,-0.055,0,0},16});
    world_.objects.push_back({4,ObjectKind::Bed,{4,1},std::nullopt,{0,0,-0.055,0,0},16});
    world_.objects.push_back({5,ObjectKind::Toilet,{5,1},std::nullopt,{0,0,0,-0.12,0},7});
    world_.objects.push_back({6,ObjectKind::Toilet,{5,2},std::nullopt,{0,0,0,-0.12,0},7});
    world_.objects.push_back({7,ObjectKind::Sink,{5,3},std::nullopt,{0,-0.085,0,0,-0.055},8});
    world_.objects.push_back({8,ObjectKind::Sink,{5,4},std::nullopt,{0,-0.085,0,0,-0.055},8});
    world_.objects.push_back({9,ObjectKind::Fridge,{1,5},std::nullopt,{-0.075,0,0,0,0},9});
    world_.objects.push_back({10,ObjectKind::Chair,{1,3},std::nullopt,{0,0,0,0,0},5});
    world_.objects.push_back({11,ObjectKind::Chair,{2,3},std::nullopt,{0,0,0,0,0},5});
    world_.objects.push_back({12,ObjectKind::Chair,{3,3},std::nullopt,{0,0,0,0,0},5});
    world_.objects.push_back({13,ObjectKind::Chair,{4,3},std::nullopt,{0,0,0,0,0},5});

    // Founders are strangers / very low familiarity. Every direction exists so
    // future social events can evolve independently without a forced couple.
    std::uniform_real_distribution<double> familiarity(0.0,0.04);
    for(const Character& from:world_.characters){
        runtime_[from.id]=Runtime{};
        for(const Character& to:world_.characters){
            if(from.id==to.id) continue;
            Relationship& relation=relationships_.getOrCreate(from.id,to.id);
            relation.familiarity=familiarity(world_.rng);
        }
    }

    emit("new game start seed="+std::to_string(world_.seed)+" founders=4");
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

bool Simulation::trySocialDecision(Character& c,Runtime& r){
    if(world_.minute<r.socialCooldownUntilMinute) return false;

    const UnifiedUtilityDecision decision=chooseUnifiedUtilityDecision(world_,c,relationships_);
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
    if(world_.minute>=r.penaltyUntilMinute && trySocialDecision(c,r)) return;

    r.socialActive=false;
    r.socialIntent=SocialIntent::None;
    r.socialTarget=0;

    Goal chosen=(world_.minute<r.penaltyUntilMinute)?Goal::Idle:chooseGoal(world_,c);
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

void Simulation::step(){
    for(auto& c:world_.characters){
        c.needs.decay(c.metabolism,c.sleepTendency);
        Runtime& r=runtime_[c.id];
        if(r.plan.empty() && world_.minute%5==0) beginPlan(c,r);
        if(!r.plan.empty()) advanceAction(c,r);
    }
    ++world_.minute;
}
void Simulation::runMinutes(int minutes){ for(int i=0;i<minutes;++i) step(); }
}
