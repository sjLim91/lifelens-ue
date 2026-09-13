#include <cassert>
#include <cmath>
#include "lifelens/Belief.h"

using namespace lifelens;

int main()
{
    BeliefState state;
    assert(state.find(20,"stole food")==nullptr);

    MemoryRecord direct;
    direct.who=20;
    direct.minute=100;
    direct.importance=0.9;
    direct.confidence=0.95;
    direct.witnessed=true;
    direct.source=MemorySource::DirectWitness;
    direct.tags={"theft","food"};
    direct.normalize();

    BeliefRecord& first=state.ingestMemory(direct,"stole food",true,100);
    assert(first.stance>0.0);
    assert(first.confidence>0.0);
    assert(first.supportCount==1);
    const double directConfidence=first.confidence;

    BeliefState rumorState;
    MemoryRecord rumor=direct;
    rumor.witnessed=false;
    rumor.source=MemorySource::ToldByOther;
    rumor.sourceCharacter=30;
    rumor.normalize();
    BeliefRecord& rumorBelief=rumorState.ingestMemory(rumor,"stole food",true,100);
    assert(rumorBelief.confidence<directConfidence);

    MemoryRecord contradiction=direct;
    contradiction.importance=1.0;
    contradiction.confidence=1.0;
    contradiction.minute=120;
    contradiction.normalize();

    const double stanceBefore=first.stance;
    state.ingestMemory(contradiction,"stole food",false,120);
    const BeliefRecord* updated=state.find(20,"stole food");
    assert(updated!=nullptr);
    assert(updated->contradictionCount==1);
    assert(updated->stance<stanceBefore);
    assert(updated->lastUpdatedMinute==120);

    MemoryRecord inferred=direct;
    inferred.source=MemorySource::Inferred;
    inferred.witnessed=false;
    inferred.normalize();

    BeliefState inferredState;
    const BeliefRecord& inferredBelief=inferredState.ingestMemory(inferred,"hid supplies",true,100);
    assert(inferredBelief.confidence<directConfidence);

    MemoryRecord old=direct;
    old.minute=0;
    old.decayPerDay=0.15;
    old.normalize();

    MemoryRecord fresh=direct;
    fresh.minute=1440*20;
    fresh.decayPerDay=0.15;
    fresh.normalize();

    BeliefState oldState;
    BeliefState freshState;
    const int now=1440*20;
    const auto& oldBelief=oldState.ingestMemory(old,"visited storage",true,now);
    const auto& freshBelief=freshState.ingestMemory(fresh,"visited storage",true,now);
    assert(oldBelief.confidence<freshBelief.confidence);

    BeliefRecord& separate=state.getOrCreate(40,"stole food");
    separate.applyEvidence(true,0.5,130);
    assert(state.find(40,"stole food")!=nullptr);
    assert(state.find(20,"stole food")!=state.find(40,"stole food"));

    return 0;
}
