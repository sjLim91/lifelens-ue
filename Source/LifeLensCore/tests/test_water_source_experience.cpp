// 감사용 브랜치: 실제 Core 로직은 #666 exact head와 같음.
#include <cassert>
#include <cmath>
#include <vector>
#include "lifelens/WaterSourceExperience.h"
#include "lifelens/UtilityAI.h"

using namespace lifelens;

int main()
{
    Character first;
    first.id=1;
    first.name="First";
    Character second;
    second.id=2;
    second.name="Second";

    const std::vector<ExperiencedWaterAccess> sources={
        {100,{1,0}},{200,{2,0}}};
    GridPos target{};
    assert(chooseExperiencedWaterAccess(first,{0,0},500,sources,target));
    assert(target.x==1 && target.y==0);

    // Only real firsthand evidence changes the preferred water source.
    MemoryRecord observed;
    observed.where=environmentalGridLabel({1,0});
    observed.what="experienced unsanitary surroundings";
    observed.minute=480;
    observed.confidence=1.0;
    observed.importance=1.0;
    observed.source=MemorySource::DirectWitness;
    observed.witnessed=true;
    observed.tags={"environment","contamination","avoidance"};
    first.memory.add(observed);
    assert(chooseExperiencedWaterAccess(first,{0,0},500,sources,target));
    assert(target.x==2 && target.y==0);

    // Another resident did not personally witness it and chooses differently.
    assert(chooseExperiencedWaterAccess(second,{0,0},500,sources,target));
    assert(target.x==1 && target.y==0);

    // Hearsay is not direct sensory knowledge of that location.
    Character hearsay=second;
    observed.source=MemorySource::Inferred;
    hearsay.memory.add(observed);
    assert(chooseExperiencedWaterAccess(hearsay,{0,0},500,sources,target));
    assert(target.x==1 && target.y==0);

    // A very distant alternative must not make a starving/thirsty resident
    // abandon the only nearby source, even if they distrust that location.
    const std::vector<ExperiencedWaterAccess> longDetour={
        {100,{1,0}},{200,{50,0}}};
    assert(chooseExperiencedWaterAccess(first,{0,0},500,longDetour,target));
    assert(target.x==1 && target.y==0);

    // The undesirable source remains usable when there is no other option.
    assert(chooseExperiencedWaterAccess(
        first,{0,0},500,{{100,{1,0}}},target));
    assert(target.x==1 && target.y==0);

    // Memory confidence decays; ancient hearsay cannot permanently override
    // available water. A direct memory may still affect later decisions.
    assert(directlyRememberedWaterHazard(first,{1,0},500)
           >directlyRememberedWaterHazard(first,{1,0},500+4000*1440));

    // No candidate or null resource is not actionable. No fabricated water.
    assert(!chooseExperiencedWaterAccess(first,{0,0},500,{},target));
    assert(!chooseExperiencedWaterAccess(first,{0,0},500,
        {{0,{1,0}}},target));

    // The water NEED itself is unchanged by the location's remembered hazard.
    World world(33);
    first.needs.thirst=0.60;
    second.needs.thirst=0.60;
    assert(std::abs(scoreGoal(world,first,Goal::Drink)
                    -scoreGoal(world,second,Goal::Drink))<1e-12);
    return 0;
}
