#include <iostream>
#include <vector>

#include "lifelens/GenerationContinuity.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeResident(CharacterId id,LifeStage stage,double reproductive=1.0)
{
    Character c;
    c.id=id;
    c.name="Resident";
    c.lifeStage=stage;
    c.lifeCondition.reproductivePotential=reproductive;
    c.alive=true;
    return c;
}

int main()
{
    GenealogyBook genealogy;
    PregnancyBook pregnancies;

    // Formal New Game starts as a viable founding cohort rather than pretending
    // that an uncreated second generation already exists.
    Character a=makeResident(1,LifeStage::Adult);
    Character b=makeResident(2,LifeStage::Adult);
    Character c=makeResident(3,LifeStage::Adult);
    Character d=makeResident(4,LifeStage::Adult);
    std::vector<Character*> founders={&a,&b,&c,&d};
    auto founding=assessGenerationContinuity(founders,genealogy,pregnancies);
    CHECK(founding.status==ContinuityStatus::Founding);
    CHECK(founding.population.living==4);
    CHECK(founding.reproductiveAdults==4);
    CHECK(founding.maxGenerationDepth==0);
    CHECK(founding.nextGenerationAbsent);
    CHECK(!founding.atRisk());

    // A child creates a real next generation and moves the society into transition.
    Character child=makeResident(10,LifeStage::Child,0.0);
    CHECK(genealogy.registerBirth(child.id,a.id,b.id));
    std::vector<Character*> withChild={&a,&b,&c,&d,&child};
    auto transitioning=assessGenerationContinuity(withChild,genealogy,pregnancies);
    CHECK(transitioning.status==ContinuityStatus::Transitioning);
    CHECK(transitioning.population.livingMinors==1);
    CHECK(transitioning.livingWithParents==1);
    CHECK(transitioning.maxGenerationDepth==1);
    CHECK(!transitioning.nextGenerationAbsent);
    CHECK(transitioning.continuityScore>founding.continuityScore);

    // A grandchild proves that the society has crossed into a persistent
    // multi-generation lineage.
    child.lifeStage=LifeStage::YoungAdult;
    child.lifeCondition.reproductivePotential=1.0;
    Character partner=makeResident(11,LifeStage::YoungAdult);
    Character grandchild=makeResident(20,LifeStage::Baby,0.0);
    CHECK(genealogy.registerBirth(grandchild.id,child.id,partner.id));
    std::vector<Character*> multi={&a,&b,&c,&d,&child,&partner,&grandchild};
    auto stable=assessGenerationContinuity(multi,genealogy,pregnancies);
    CHECK(stable.status==ContinuityStatus::Stable);
    CHECK(stable.maxGenerationDepth==2);
    CHECK(stable.population.livingAdults>=2);
    CHECK(stable.continuityScore>transitioning.continuityScore);

    // Living people can still be at risk if no viable reproductive base or
    // younger cohort remains.
    Character elderA=makeResident(30,LifeStage::Elderly,0.0);
    Character elderB=makeResident(31,LifeStage::Elderly,0.0);
    GenealogyBook isolatedGenealogy;
    PregnancyBook noPregnancies;
    std::vector<Character*> agingOut={&elderA,&elderB};
    auto risk=assessGenerationContinuity(agingOut,isolatedGenealogy,noPregnancies);
    CHECK(risk.status==ContinuityStatus::AtRisk);
    CHECK(risk.reproductiveBaseLow);
    CHECK(risk.nextGenerationAbsent);
    CHECK(risk.atRisk());

    // An active pregnancy is a future-cohort signal, even before birth.
    Character parentA=makeResident(40,LifeStage::Adult,1.0);
    Character parentB=makeResident(41,LifeStage::Adult,1.0);
    PregnancyBook expecting;
    CHECK(expecting.start(parentA.id,parentB.id,100)!=nullptr);
    std::vector<Character*> expectingResidents={&parentA,&parentB};
    auto expectingReport=assessGenerationContinuity(expectingResidents,isolatedGenealogy,expecting);
    CHECK(expectingReport.status==ContinuityStatus::Transitioning);
    CHECK(expectingReport.activePregnancies==1);
    CHECK(!expectingReport.nextGenerationAbsent);

    // No living residents is an explicit terminal continuity state.
    elderA.alive=false;
    elderB.alive=false;
    auto extinct=assessGenerationContinuity(agingOut,isolatedGenealogy,noPregnancies);
    CHECK(extinct.status==ContinuityStatus::Extinct);
    CHECK(extinct.population.living==0);
    CHECK(extinct.continuityScore==0.0);
    CHECK(extinct.atRisk());

    // C2 long-run reliability: continuity must retain exact lineage depth well
    // beyond the old 16-generation reporting cap.
    GenealogyBook deepGenealogy;
    constexpr int DeepGenerations=64;
    CharacterId lineageParent=1000;
    for(int generation=1;generation<=DeepGenerations;++generation){
        const CharacterId child=1000+static_cast<CharacterId>(generation);
        const CharacterId sideParent=5000+static_cast<CharacterId>(generation);
        CHECK(deepGenealogy.registerBirth(
            child,lineageParent,sideParent));
        lineageParent=child;
    }
    Character deepDescendant=
        makeResident(lineageParent,LifeStage::Adult,1.0);
    std::vector<Character*> deepResidents={&deepDescendant};
    const auto deepReport=assessGenerationContinuity(
        deepResidents,deepGenealogy,noPregnancies);
    CHECK(deepReport.maxGenerationDepth==DeepGenerations);
    CHECK(!deepReport.lineageCycleDetected);

    // Malformed imported genealogy must never hang long-run analysis.
    GenealogyBook cyclic;
    CHECK(cyclic.registerBirth(2,1,101));
    CHECK(cyclic.registerBirth(1,2,102));
    const GenerationDepthIndex cyclicIndex=
        buildGenerationDepthIndex(cyclic);
    CHECK(cyclicIndex.cycleDetected);
    Character cyclicResident=makeResident(2,LifeStage::Adult,1.0);
    std::vector<Character*> cyclicResidents={&cyclicResident};
    const auto cyclicReport=assessGenerationContinuity(
        cyclicResidents,cyclic,noPregnancies);
    CHECK(cyclicReport.lineageCycleDetected);

    return 0;
}
