#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/SocialUtility.h"
#include "lifelens/TraitsPreferences.h"

using namespace lifelens;

int main(int argc,char** argv)
{
    WorldSeed seed=874213954;
    for(int i=1;i<argc;++i){
        const std::string arg=argv[i];
        if(arg=="--seed" && i+1<argc){
            seed=static_cast<WorldSeed>(std::strtoull(argv[++i],nullptr,10));
        }
    }

    Simulation sim(seed,0,CurrentWorldGenerationVersion);
    sim.setupNewGame();

    std::cout<<std::fixed<<std::setprecision(4);
    for(const Character& self:sim.world().characters){
        const TraitProfile traits=deriveTraitProfile(self.personality,self.genetics);
        const PreferenceProfile prefs=derivePreferenceProfile(self.personality,self.genetics);
        double bestApproach=0.0;
        CharacterId bestTarget=0;
        for(const Character& target:sim.world().characters){
            if(target.id==self.id) continue;
            const Relationship* relation=sim.relationships().find(self.id,target.id);
            const double initiative=firstContactInitiative(relation,traits,prefs);
            const double approach=scoreApproachIntent(
                sim.world(),self,target.id,sim.relationships());
            if(approach>bestApproach){
                bestApproach=approach;
                bestTarget=target.id;
            }
            std::cout
                <<"PAIR seed="<<seed
                <<" self="<<self.id
                <<" target="<<target.id
                <<" familiarity="<<(relation?relation->familiarity:0.0)
                <<" bond="<<(relation?relation->socialBond():0.0)
                <<" initiative="<<initiative
                <<" approach="<<approach
                <<"\n";
        }
        const auto physical=bestPhysicalUtility(sim.world(),self);
        std::cout
            <<"FOUNDER seed="<<seed
            <<" id="<<self.id
            <<" name="<<self.name
            <<" sociability="<<self.personality.sociability
            <<" curiosity="<<self.personality.curiosity
            <<" introversion="<<self.personality.introversion
            <<" openness="<<self.personality.openness
            <<" riskTolerance="<<self.personality.riskTolerance
            <<" socializing="<<prefs.socializing
            <<" solitude="<<prefs.solitude
            <<" novelty="<<prefs.novelty
            <<" boldness="<<traits.boldness
            <<" bestApproach="<<bestApproach
            <<" bestTarget="<<bestTarget
            <<" physicalUtility="<<physical.second
            <<" maxNeed="<<maximumResidentNeed(self)
            <<"\n";
    }
    return 0;
}
