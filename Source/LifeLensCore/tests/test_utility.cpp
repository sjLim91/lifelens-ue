#include <iostream>
#include "lifelens/UtilityAI.h"
#include "lifelens/Planner.h"

static bool checkSleepUrgentWakeThreshold(){
    lifelens::Character character;
    character.needs={0.71,0.71,0.95,0.71,0.10};
    if(lifelens::sleepInterruptedByUrgentNeed(character)) return false;

    character.needs.hunger=lifelens::SleepUrgentNeedWakeThreshold;
    if(!lifelens::sleepInterruptedByUrgentNeed(character)) return false;
    character.needs.hunger=0.10;

    character.needs.thirst=lifelens::SleepUrgentNeedWakeThreshold;
    if(!lifelens::sleepInterruptedByUrgentNeed(character)) return false;
    character.needs.thirst=0.10;

    character.needs.bladder=lifelens::SleepUrgentNeedWakeThreshold;
    return lifelens::sleepInterruptedByUrgentNeed(character);
}

int main(){
    if(!checkSleepUrgentWakeThreshold()){
        std::cerr<<"urgent survival needs must interrupt sleep\n"; return 5;
    }
    lifelens::World w(42);
    lifelens::Character c; c.id=7; c.name="test"; c.needs={0.90,0.20,0.20,0.20,0.20};
    w.characters.push_back(c);
    w.objects.push_back({1,lifelens::ObjectKind::Fridge,{1,0},std::nullopt,{-0.1,0,0,0,0},5});
    w.objects.push_back({2,lifelens::ObjectKind::Sink,{0,1},std::nullopt,{0,-0.1,0,0,-0.1},5});
    w.objects.push_back({3,lifelens::ObjectKind::Bed,{2,0},std::nullopt,{0,0,-0.1,0,0},5});
    w.objects.push_back({4,lifelens::ObjectKind::Toilet,{0,2},std::nullopt,{0,0,0,-0.1,0},5});
    w.characters.front().civilization.inventory.add({
        lifelens::ItemKind::RawMaterial,
        lifelens::MaterialKind::PlantFood,
        1,0.5,1.0});
    if(lifelens::chooseGoal(w,w.characters.front())!=lifelens::Goal::Eat){
        std::cerr<<"expected Eat for hunger 0.9\n"; return 1;
    }
    if(!w.characters.front().civilization.inventory.remove(
        lifelens::ItemKind::RawMaterial,
        lifelens::MaterialKind::PlantFood,
        1)) return 4;
    for(auto& o:w.objects) o.reservedBy=999;
    if(lifelens::chooseGoal(w,w.characters.front())!=lifelens::Goal::Idle){
        std::cerr<<"expected Idle when every usable object is reserved\n"; return 2;
    }

    for(auto& o:w.objects) o.reservedBy.reset();
    lifelens::UtilityAIRuleset tuned=lifelens::DefaultSimulationRuleset.utilityAI;
    tuned.idleScore=10.0;
    tuned.secondChoiceProbability=0.0;
    if(lifelens::chooseGoal(w,w.characters.front(),tuned)!=lifelens::Goal::Idle){
        std::cerr<<"custom utility ruleset did not override goal scoring\n"; return 3;
    }

    std::cout<<"test_utility PASS\n";
    return 0;
}
