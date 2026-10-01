#include <iostream>
#include "lifelens/UtilityAI.h"
#include "lifelens/Planner.h"

static bool checkSleepUrgentWakeThreshold(){
    lifelens::Character character;

    // Merely entering the ordinary urgent band must not wake someone whose
    // fatigue is materially worse. This is the anti-thrash side of the
    // relative-dominance contract.
    character.needs={0.72,0.72,0.95,0.72,0.10};
    if(lifelens::sleepInterruptedByUrgentNeed(character)) return false;

    // Below the wake threshold never interrupts, even when fatigue is already
    // relatively low.
    character.needs={0.71,0.10,0.60,0.10,0.10};
    if(lifelens::sleepInterruptedByUrgentNeed(character)) return false;

    // At/above the wake threshold, the competing need interrupts only when it
    // also exceeds current fatigue by the shared dominance margin.
    character.needs={0.72,0.10,0.60,0.10,0.10};
    if(!lifelens::sleepInterruptedByUrgentNeed(character)) return false;

    character.needs={0.10,0.72,0.60,0.10,0.10};
    if(!lifelens::sleepInterruptedByUrgentNeed(character)) return false;

    character.needs={0.10,0.10,0.60,0.72,0.10};
    if(!lifelens::sleepInterruptedByUrgentNeed(character)) return false;

    // Exactly equal to fatigue + margin is intentionally stable; a genuine
    // dominance, not a tie, is required to tear down the sleep session.
    character.needs={0.72,0.10,0.67,0.10,0.10};
    if(lifelens::sleepInterruptedByUrgentNeed(character)) return false;
    character.needs.hunger=0.721;
    return lifelens::sleepInterruptedByUrgentNeed(character);
}

int main(){
    if(!checkSleepUrgentWakeThreshold()){
        std::cerr<<"sleep wake relative-dominance contract failed\n"; return 5;
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

    // D-023: food availability alone must not script an immediate Eat.
    // With the same moderate Hunger and the same single carried provision,
    // resident disposition can make one person eat now while another preserves
    // the last unit. The emergency boundary still wins for survival.
    lifelens::UtilityAIRuleset autonomous=lifelens::DefaultSimulationRuleset.utilityAI;
    autonomous.secondChoiceProbability=0.0;

    lifelens::World foodChoiceWorld(424242);
    lifelens::Character eager;
    eager.id=31;
    eager.needs={0.45,0.05,0.05,0.05,0.05};
    eager.personality.impulsiveness=1.0;
    eager.personality.patience=0.0;
    eager.personality.conscientiousness=0.0;
    eager.personality.orderliness=0.0;
    eager.civilization.inventory.add({
        lifelens::ItemKind::RawMaterial,
        lifelens::MaterialKind::PlantFood,
        1,0.8,1.0});

    lifelens::Character saver=eager;
    saver.id=32;
    saver.personality.impulsiveness=0.0;
    saver.personality.patience=1.0;
    saver.personality.conscientiousness=1.0;
    saver.personality.orderliness=1.0;

    const double eagerEatScore=lifelens::scoreGoal(
        foodChoiceWorld,eager,lifelens::Goal::Eat,autonomous);
    const double saverEatScore=lifelens::scoreGoal(
        foodChoiceWorld,saver,lifelens::Goal::Eat,autonomous);
    if(!(eagerEatScore>autonomous.idleScore
         && saverEatScore<autonomous.idleScore)){
        std::cerr<<"moderate hunger must allow personality-driven eat timing\n";
        return 6;
    }
    if(lifelens::chooseGoal(foodChoiceWorld,eager,autonomous)
       !=lifelens::Goal::Eat){
        std::cerr<<"impulsive resident should choose Eat at moderate hunger\n";
        return 7;
    }
    if(lifelens::chooseGoal(foodChoiceWorld,saver,autonomous)
       !=lifelens::Goal::Idle){
        std::cerr<<"disciplined resident should be able to preserve last food\n";
        return 8;
    }

    lifelens::Character perishable=saver;
    perishable.id=33;
    perishable.civilization.inventory.agePlantFoodOneDay(0.70,0.01);
    if(perishable.civilization.inventory.averagePlantFoodFreshness()>=0.20){
        std::cerr<<"perishable-food setup did not create spoilage pressure\n";
        return 9;
    }
    if(lifelens::chooseGoal(foodChoiceWorld,perishable,autonomous)
       !=lifelens::Goal::Eat){
        std::cerr<<"near-spoilage food should weaken reserve discipline\n";
        return 10;
    }

    saver.needs.hunger=0.92;
    if(lifelens::chooseGoal(foodChoiceWorld,saver,autonomous)
       !=lifelens::Goal::Eat){
        std::cerr<<"critical hunger must override food-reserve discipline\n";
        return 11;
    }

    std::cout<<"test_utility PASS\n";
    return 0;
}
