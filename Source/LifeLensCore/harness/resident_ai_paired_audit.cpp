#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "lifelens/ResidentAIPairedCausalAudit.h"

int main(int argc,char** argv)
{
    lifelens::WorldSeed seed=874213954;
    int days=1;
    lifelens::ResidentAIPairedPolicy policy=
        lifelens::ResidentAIPairedPolicy::OriginalFour;
    try{
        for(int i=1;i<argc;i+=2){
            if(i+1>=argc)
                throw std::invalid_argument("missing option value");
            const std::string option=argv[i], value=argv[i+1];
            std::size_t consumed=0;
            if(option=="--seed"){
                seed=std::stoull(value,&consumed);
                if(seed==0 ||consumed!=value.size())
                    throw std::invalid_argument("invalid seed");
            }else if(option=="--days"){
                const long parsed=std::stol(value,&consumed);
                if(consumed!=value.size() ||parsed<1 ||parsed>1000)
                    throw std::invalid_argument("days must be 1..1000");
                days=static_cast<int>(parsed);
            }else if(option=="--policy"){
                if(value=="original") {
                    policy=lifelens::ResidentAIPairedPolicy::OriginalFour;
                }else if(value=="opportunities"){
                    policy=lifelens::ResidentAIPairedPolicy::CoreOpportunityFour;
                }else throw std::invalid_argument(
                    "policy must be original or opportunities");
            }else{
                throw std::invalid_argument("unknown option: "+option);
            }
        }
        const auto report=lifelens::runResidentAIPairedAudit(
            seed,days,policy);
        std::cout<<lifelens::residentAIPairedReportJson(report)<<"\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"Invalid resident AI paired audit: "<<e.what()<<"\n"
                 <<"Usage: ll_resident_ai_paired_audit"
                 <<" [--seed POSITIVE_UINT64] [--days 1..1000]"
             <<" [--policy original|opportunities]\n";
        return 2;
    }
}
