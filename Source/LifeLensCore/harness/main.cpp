#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include "lifelens/Simulation.h"

int main(int argc,char** argv){
    int days=1; std::uint64_t seed=42; bool tickLog=false;
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--days" && i+1<argc) days=std::max(1,std::atoi(argv[++i]));
        else if(a=="--seed" && i+1<argc) seed=std::strtoull(argv[++i],nullptr,10);
        else if(a=="--tick-log") tickLog=true;
    }
    lifelens::Simulation sim(seed);
    sim.onEvent([](const std::string& line){std::cout<<line<<'\n';});
    sim.setupDemo();
    const int total=days*24*60;
    for(int i=0;i<total;++i){
        sim.step();
        if(tickLog && !sim.world().characters.empty()){
            const auto& n=sim.world().characters.front().needs;
            std::cout<<"[tick "<<sim.world().minute<<"] H="<<n.hunger<<" T="<<n.thirst<<" S="<<n.sleep<<" B="<<n.bladder<<" Hy="<<n.hygiene<<'\n';
        }
    }
    if(!sim.world().characters.empty()){
        const auto& n=sim.world().characters.front().needs;
        std::cout<<"[Day "<<days<<" END] "<<std::fixed<<std::setprecision(2)
                 <<"Hunger "<<n.hunger<<" Thirst "<<n.thirst<<" Sleep "<<n.sleep
                 <<" Bladder "<<n.bladder<<" Hygiene "<<n.hygiene<<'\n';
    }
    return 0;
}
