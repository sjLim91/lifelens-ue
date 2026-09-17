#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <utility>
#include "Character.h"
#include "EmotionRuntime.h"
#include "SimulationRuleset.h"
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
inline bool emergencyAffordanceAvailableFor(const Character& c,Goal g) {
    switch(g){
        case Goal::Sleep:
        case Goal::UseToilet:
        case Goal::Wash:
            return true;
        case Goal::Eat:
            return c.civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::PlantFood)>0;
        case Goal::Drink:
            return c.civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Water)>0;
        case Goal::Idle:
        default:
            return true;
    }
}
inline bool actionAvailableFor(const World& w,const Character& c,Goal g) {
    return objectAvailableFor(w,g,c.id) || emergencyAffordanceAvailableFor(c,g);
}
inline double needForGoal(const Character& c, Goal g) {
    switch(g){case Goal::Eat:return c.needs.hunger;case Goal::Drink:return c.needs.thirst;case Goal::Sleep:return c.needs.sleep;case Goal::UseToilet:return c.needs.bladder;case Goal::Wash:return c.needs.hygiene;default:return 0.0;}
}
inline bool hourInWindow(int hour,int startHour,int endHour) {
    if(startHour<endHour) return hour>=startHour && hour<endHour;
    return hour>=startHour || hour<endHour;
}
inline double utilityCurve(double n,const UtilityAIRuleset& rules) {
    const double clamped=std::clamp(n,0.0,1.0);
    const double base=std::pow(clamped,rules.needExponent);
    const double urgent=clamped>=rules.urgentThreshold
        ? (clamped-rules.urgentThreshold)*rules.urgentSlope
        : 0.0;
    return base+urgent;
}
inline double utilityCurve(double n) {
    return utilityCurve(n,DefaultSimulationRuleset.utilityAI);
}
inline double scoreGoal(const World& w,const Character& c,Goal g,const UtilityAIRuleset& rules) {
    if(g==Goal::Idle) return rules.idleScore;
    if(!actionAvailableFor(w,c,g)) return 0.0;
    double s=utilityCurve(needForGoal(c,g),rules);
    if(g==Goal::Sleep){
        const int hour=(w.minute/60)%24;
        if(hourInWindow(hour,rules.sleepNightStartHour,rules.sleepNightEndHour)) s*=rules.sleepNightMultiplier;
    }
    if(g==Goal::Wash) s*=rules.washBaseMultiplier+rules.washConscientiousnessMultiplier*c.personality.conscientiousness;
    if(g==Goal::Sleep){
        s*=rules.sleepBaseMultiplier+rules.sleepIntroversionMultiplier*c.personality.introversion;
        s*=emotionSleepUtilityMultiplier(c);
    }
    return s;
}
inline double scoreGoal(const World& w,const Character& c,Goal g) {
    return scoreGoal(w,c,g,DefaultSimulationRuleset.utilityAI);
}
inline Goal chooseGoal(World& w,const Character& c,const UtilityAIRuleset& rules) {
    std::array<Goal,6> gs={Goal::Eat,Goal::Drink,Goal::Sleep,Goal::UseToilet,Goal::Wash,Goal::Idle};
    std::array<std::pair<double,Goal>,6> scored{};
    for(std::size_t i=0;i<gs.size();++i) scored[i]={scoreGoal(w,c,gs[i],rules),gs[i]};
    std::sort(scored.begin(),scored.end(),[](auto a,auto b){return a.first>b.first;});
    if(scored[0].second==Goal::Idle || scored[1].first<=0.0) return scored[0].second;
    std::uniform_real_distribution<double> pick(0.0,1.0);
    return pick(w.rng)<rules.secondChoiceProbability ? scored[1].second : scored[0].second;
}
inline Goal chooseGoal(World& w,const Character& c) {
    return chooseGoal(w,c,DefaultSimulationRuleset.utilityAI);
}
}
