#include "lifelens/Simulation.h"
#include <iomanip>
#include <sstream>
namespace lifelens {
Simulation::Simulation(std::uint64_t seed):world_(seed){}

void Simulation::setupDemo(){
    world_.characters.clear(); world_.objects.clear(); runtime_.clear(); logs_.clear(); world_.minute=7*60;
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

std::string Simulation::stamp() const{
    const int absolute=world_.minute; const int day=absolute/(24*60)+1; const int md=absolute%(24*60);
    std::ostringstream s; s<<"[Day "<<day<<" "<<std::setfill('0')<<std::setw(2)<<(md/60)<<":"<<std::setw(2)<<(md%60)<<"] "; return s.str();
}
void Simulation::emit(const std::string& message){ const std::string line=stamp()+message; logs_.push_back(line); for(auto& cb:callbacks_) cb(line); }
void Simulation::onEvent(EventCallback cb){ callbacks_.push_back(std::move(cb)); }
SmartObject* Simulation::objectById(ObjectId id){ for(auto& o:world_.objects) if(o.id==id) return &o; return nullptr; }
void Simulation::failPlan(Runtime& r){ r.plan.clear(); r.actionIndex=0; r.announced=false; ++r.consecutiveFailures; if(r.consecutiveFailures>=3){r.penaltyUntilMinute=world_.minute+30;r.consecutiveFailures=0;} }

void Simulation::beginPlan(Character& c,Runtime& r){
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
    if(r.actionIndex>=r.plan.size()){ r.plan.clear(); r.actionIndex=0; }
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
