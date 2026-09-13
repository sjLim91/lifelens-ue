#include <cmath>
#include <iostream>
#include "lifelens/Needs.h"

int main(){
    lifelens::Needs n{};
    for(int i=0;i<24*60;++i) n.decay();
    auto near=[](double a,double b){return std::abs(a-b)<1e-9;};
    if(!(near(n.hunger,1.0)&&near(n.thirst,1.0)&&near(n.sleep,1.0)&&near(n.bladder,1.0)&&near(n.hygiene,1.0))){
        std::cerr<<"needs did not saturate after 24h\n"; return 1;
    }
    n.apply({-0.5,-0.4,-0.3,-0.7,-0.2});
    if(!(n.hunger<1.0&&n.thirst<1.0&&n.sleep<1.0&&n.bladder<1.0&&n.hygiene<1.0)){
        std::cerr<<"need effects did not reduce values\n"; return 2;
    }
    std::cout<<"test_needs PASS\n";
    return 0;
}
