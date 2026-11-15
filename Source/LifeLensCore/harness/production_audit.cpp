#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/ResidentProductionObservation.h"
using namespace lifelens;
int main(int argc,char** argv) {
    const auto seed=argc>1?std::strtoull(argv[1],nullptr,10):874213954ULL;
    const int days=argc>2?std::atoi(argv[2]):30;if(days<1 ||days>1000)return 2;
    Simulation sim(seed);sim.setupNewGame();
    std::unordered_set<std::uint64_t> seen,finished;
    std::map<std::uint64_t,std::pair<std::vector<std::uint8_t>,int>> starts;
    std::string error;bool exported=false;
    for(int minute=0;minute<days*1440;++minute) {
        sim.step();
        for(const auto& c:sim.world().characters) {
            const auto* p=sim.residentProductionPlan(c.id);if(!p)continue;
            if(seen.insert(p->id).second) {
                std::vector<std::uint8_t> bytes;
                if(!encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error)){std::cerr<<error<<" at "<<sim.world().minute<<'\n';return 3;}
                if(std::getenv("LIFELENS_PRODUCTION_FIXTURE_DIR") &&!exported) {
                    if(starts.size()>=8)starts.erase(starts.begin());
                    starts[p->id]={std::move(bytes),sim.world().minute};
                }
            }
            if(!p->open() &&finished.insert(p->id).second) {
                std::cout<<"{\"profile\":\"untouched-new-game\",\"seed\":\""<<seed<<"\",\"actor\":\""<<c.id<<"\",\"plan\":";
                appendProductionPlanJson(std::cout,p);std::cout<<"}\n"<<std::flush;
                if(!exported &&p->state==ProductionPlanState::Completed &&p->steps.size()>=2 &&starts.count(p->id)) {
                    if(const char* dir=std::getenv("LIFELENS_PRODUCTION_FIXTURE_DIR")) {
                        std::filesystem::create_directories(dir);
                        std::vector<std::uint8_t> result;
                        if(!encodeSimulationSnapshot(sim.captureSnapshot(),result,&error))return 4;
                        const auto put=[&](const char* name,const auto& bytes){std::ofstream f(std::filesystem::path(dir)/name,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());};
                        put("start.bin",starts.at(p->id).first);put("result.bin",result);
                        std::ofstream(std::filesystem::path(dir)/"minutes.txt")<<sim.world().minute-starts.at(p->id).second;
                        std::ofstream plan(std::filesystem::path(dir)/"plan.json");appendProductionPlanJson(plan,p);
                        exported=true;starts.clear();
                    }
                }
                starts.erase(p->id);
            }
        }
        if((minute+1)%1440==0 &&((minute+1)/1440==1 ||(minute+1)/1440==7 ||(minute+1)/1440==30 ||(minute+1)/1440==100 ||minute+1==days*1440)) {
            std::vector<std::uint8_t> bytes,again;SimulationStateSnapshot decoded;
            if(!encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error) ||!decodeSimulationSnapshot(bytes,decoded,&error)) {std::cerr<<error<<'\n';return 5;}
            Simulation restored(1);if(!restored.restoreSnapshot(decoded,&error) ||!encodeSimulationSnapshot(restored.captureSnapshot(),again,&error) ||again!=bytes){std::cerr<<error<<'\n';return 6;}
            std::cout<<"{\"profile\":\"untouched-census\",\"seed\":\""<<seed<<"\",\"day\":"<<(minute+1)/1440
                     <<",\"goals\":"<<seen.size()<<",\"closed\":"<<finished.size()<<",\"binaryRoundtrip\":true}\n"<<std::flush;
        }
    }
}
