#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include "lifelens/ArtifactObservation.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
using namespace lifelens;
int main(int argc,char** argv){
    const auto seed=argc>1?std::strtoull(argv[1],nullptr,10):874213954ULL;
    const int days=argc>2?std::atoi(argv[2]):1000;
    if(days<1||days>3000)return 2;
    Simulation simulation(seed);simulation.setupNewGame();
    const int start=simulation.world().minute;
    const auto began=std::chrono::steady_clock::now();
    // Unmodified production New Game: no supplied inventory, thoughts, needs,
    // technique, geometry, injury, competing agent or environmental mutations.
    for(int day=1;day<=days;++day){
        for(int minute=0;minute<1440;++minute)simulation.step();
        if(day!=100&&day!=365&&day!=days)continue;
        const auto& world=simulation.world();
        int living=0,trials=0,successes=0,uses=0,demonstrations=0,repairs=0,objects=0;
        for(const auto& c:world.characters){
            living+=c.alive;
            for(const auto& t:c.civilization.invention.techniques){trials+=t.trials;successes+=t.successes;
                uses+=t.uses;demonstrations+=t.demonstrations;}
            for(const auto& a:c.civilization.inventory.artifacts){++objects;repairs+=a.repairs;}
        }
        std::vector<std::uint8_t> bytes;std::string error;
        if(!encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error)){
            std::cerr<<error<<'\n';return 3;
        }
        SimulationStateSnapshot decoded;
        if(!decodeSimulationSnapshot(bytes,decoded,&error)){std::cerr<<error<<'\n';return 4;}
        Simulation replay(1);if(!replay.restoreSnapshot(decoded,&error)){std::cerr<<error<<'\n';return 5;}
        std::vector<std::uint8_t> again;
        if(!encodeSimulationSnapshot(replay.captureSnapshot(),again,&error)||bytes!=again)return 6;
        std::cout<<"{\"profile\":\"unmodified-new-game\",\"seed\":\""<<seed<<"\",\"day\":"<<day
            <<",\"minute\":"<<world.minute<<",\"elapsedMinutes\":"<<world.minute-start
            <<",\"living\":"<<living<<",\"residents\":"<<world.characters.size()
            <<",\"trials\":"<<trials<<",\"successes\":"<<successes<<",\"uses\":"<<uses
            <<",\"demonstrations\":"<<demonstrations<<",\"repairs\":"<<repairs<<",\"artifacts\":"<<objects
            <<",\"facilities\":"<<world.facilities.size()<<",\"snapshotBytes\":"<<bytes.size()
            <<",\"binaryRoundtrip\":true,\"wallSeconds\":"
            <<std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count()
            <<",\"cultures\":";appendArtifactCulturesJson(std::cout,world);std::cout<<"}\n"<<std::flush;
    }
}
