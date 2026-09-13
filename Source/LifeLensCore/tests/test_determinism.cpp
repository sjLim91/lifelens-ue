#include <iostream>
#include <string>
#include <vector>
#include "lifelens/Simulation.h"

static std::vector<std::string> run(std::uint64_t seed){
    lifelens::Simulation sim(seed);
    sim.setupDemo();
    sim.runMinutes(3*24*60);
    return sim.logs();
}

int main(){
    const auto a=run(12345);
    const auto b=run(12345);
    const auto c=run(54321);
    if(a!=b){ std::cerr<<"same seed produced different logs\n"; return 1; }
    if(a==c){ std::cerr<<"different seeds produced identical logs\n"; return 2; }
    if(a.empty()){ std::cerr<<"simulation produced no events\n"; return 3; }
    std::cout<<"test_determinism PASS events="<<a.size()<<"\n";
    return 0;
}
