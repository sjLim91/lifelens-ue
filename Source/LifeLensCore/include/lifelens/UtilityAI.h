#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <utility>
#include "Character.h"
#include "World.h"
namespace lifelens {
enum class Goal { Eat, Drink, Sleep, UseToilet, Wash, Idle };
inline const char* goalName(Goal g) {
    switch(g){case Goal::Eat:return "Eat";case Goal::Drink:return "Drink";case Goal::Sleep:return "Sleep";case Goal::UseToilet:return "UseToilet";case Goal::Wash:return "Wash";default:return "Idle";}
}
inline ObjectKind objectKindFor(Goal g) {
    switch(g){case Goal::Eat:return ObjectKind::Fridge;case Goal::Drink:return ObjectKind::Sink;case Goal::Sleep:return ObjectKind::Bed;case Goal::UseToilet:return ObjectKind::Toilet;case Goal::Wash:return ObjectKind::Sink;default:return ObjectKind::Chair;}
}
inline bool objectAvailableFor(const World& w, Goal g, CharacterId who) {
    if(g==Goal::Idle) return true;
    const auto kind=objectKindFor(g);
    for(const auto& o:w.objects) if(o.kind==kind && (!o.reservedBy || *o.reservedBy==who)) return true;
    return false;
}
inline double needForGoal(const Character& c, Goal g) {
    switch(g){case Goal::Eat:return c.needs.hunger;case Goal::Drink:return c.needs.thirst;case Goal::Sleep:return c.needs.sleep;case Goal::UseToilet:return c.needs.bladder;case Goal::Wash:return c.needs.hygiene;default:return 0.0;}
}
inline double utilityCurve(double n) {
    const double base=std::pow(std::clamp(n,0.0,1.0),4.0);
    const double urgent=n>=0.70 ? (n-0.70)*1.8 : 0.0;
    return base+urgent;
}
inline double scoreGoal(const World& w,const Character& c,Goal g) {
    if(g==Goal::Idle) return 0.035;
    if(!objectAvailableFor(w,g,c.id)) return 0.0;
    double s=utilityCurve(needForGoal(c,g));
    if(g==Goal::Sleep){ const int hour=(w.minute/60)%24; if(hour>=22||hour<6) s*=1.35; }
    if(g==Goal::Wash) s*=0.85+0.35*c.personality.conscientiousness;
    if(g==Goal::Sleep) s*=0.9+0.25*c.personality.introversion;
    return s;
}
inline Goal chooseGoal(World& w,const Character& c) {
    std::array<Goal,6> gs={Goal::Eat,Goal::Drink,Goal::Sleep,Goal::UseToilet,Goal::Wash,Goal::Idle};
    std::array<std::pair<double,Goal>,6> scored{};
    for(std::size_t i=0;i<gs.size();++i) scored[i]={scoreGoal(w,c,gs[i]),gs[i]};
    std::sort(scored.begin(),scored.end(),[](auto a,auto b){return a.first>b.first;});
    if(scored[0].second==Goal::Idle || scored[1].first<=0.0) return scored[0].second;
    std::uniform_real_distribution<double> pick(0.0,1.0);
    return pick(w.rng)<0.08 ? scored[1].second : scored[0].second;
}
}
