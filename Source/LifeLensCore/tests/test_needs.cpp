#include <cmath>
#include <iostream>
#include "lifelens/Needs.h"

int main(){
    auto near=[](double a,double b){return std::abs(a-b)<1e-9;};

    lifelens::Needs n{};
    for(int i=0;i<24*60;++i) n.decay();
    if(!(near(n.hunger,1.0)&&near(n.thirst,1.0)&&near(n.sleep,1.0)&&near(n.bladder,1.0)&&near(n.hygiene,1.0))){
        std::cerr<<"needs did not saturate after 24h\n"; return 1;
    }
    n.apply({-0.5,-0.4,-0.3,-0.7,-0.2});
    if(!(n.hunger<1.0&&n.thirst<1.0&&n.sleep<1.0&&n.bladder<1.0&&n.hygiene<1.0)){
        std::cerr<<"need effects did not reduce values\n"; return 2;
    }

    lifelens::NeedsRuleset custom{};
    custom.hungerPerMinute=0.10;
    custom.thirstPerMinute=0.20;
    custom.sleepPerMinute=0.30;
    custom.bladderPerMinute=0.40;
    custom.hygienePerMinute=0.50;
    lifelens::Needs tuned{};
    tuned.decay(custom,2.0,0.5);
    if(!(near(tuned.hunger,0.20)
         && near(tuned.thirst,0.40)
         && near(tuned.sleep,0.15)
         && near(tuned.bladder,0.80)
         && near(tuned.hygiene,0.50))){
        std::cerr<<"custom needs ruleset was not applied\n"; return 3;
    }

    std::cout<<"test_needs PASS\n";
    return 0;
}
