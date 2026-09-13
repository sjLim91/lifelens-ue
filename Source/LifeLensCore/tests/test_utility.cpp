#include <iostream>
#include "lifelens/UtilityAI.h"

int main(){
    lifelens::World w(42);
    lifelens::Character c; c.id=7; c.name="test"; c.needs={0.90,0.20,0.20,0.20,0.20};
    w.characters.push_back(c);
    w.objects.push_back({1,lifelens::ObjectKind::Fridge,{1,0},std::nullopt,{-0.1,0,0,0,0},5});
    w.objects.push_back({2,lifelens::ObjectKind::Sink,{0,1},std::nullopt,{0,-0.1,0,0,-0.1},5});
    w.objects.push_back({3,lifelens::ObjectKind::Bed,{2,0},std::nullopt,{0,0,-0.1,0,0},5});
    w.objects.push_back({4,lifelens::ObjectKind::Toilet,{0,2},std::nullopt,{0,0,0,-0.1,0},5});
    if(lifelens::chooseGoal(w,w.characters.front())!=lifelens::Goal::Eat){
        std::cerr<<"expected Eat for hunger 0.9\n"; return 1;
    }
    for(auto& o:w.objects) o.reservedBy=999;
    if(lifelens::chooseGoal(w,w.characters.front())!=lifelens::Goal::Idle){
        std::cerr<<"expected Idle when every usable object is reserved\n"; return 2;
    }
    std::cout<<"test_utility PASS\n";
    return 0;
}
