#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "lifelens/AutonomyAuditReadModel.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

static void usage()
{
    std::cerr<<"Usage: ll_autonomy_audit [--seed POSITIVE_UINT64]"
             <<" [--days 1..1000] [--sample-minutes 1..1440]\n";
}

int main(int argc,char** argv)
{
    std::uint64_t seed=874213954;
    int days=1,sampleMinutes=60;
    try{
        for(int i=1;i<argc;i+=2){
            if(i+1>=argc){usage();return 2;}
            const std::string option=argv[i];
            const std::string value=argv[i+1];
            std::size_t consumed=0;
            if(option=="--seed"){
                seed=std::stoull(value,&consumed);
                if(consumed!=value.size()||seed==0)
                    throw std::invalid_argument("seed must be positive");
            }else if(option=="--days"||option=="--sample-minutes"){
                const long number=std::stol(value,&consumed);
                if(consumed!=value.size()
                   ||number<1
                   ||number>(option=="--days"?1000:1440))
                    throw std::out_of_range("days or sample-minutes");
                if(option=="--days")days=static_cast<int>(number);
                else sampleMinutes=static_cast<int>(number);
            }else{usage();return 2;}
        }
    }catch(const std::exception& e){
        std::cerr<<"Invalid autonomy audit argument: "<<e.what()<<"\n";
        usage();
        return 2;
    }

    Simulation simulation(seed);
    simulation.setupNewGame();
    AutonomyAuditSeries audit;
    if(!audit.observe(simulation.captureSnapshot())) return 3;
    int remaining=days*1440;
    while(remaining>0){
        const int step=std::min(sampleMinutes,remaining);
        simulation.runMinutes(step);
        if(!audit.observe(simulation.captureSnapshot())) return 4;
        remaining-=step;
    }
    std::cout<<audit.json()<<"\n";
    return 0;
}
