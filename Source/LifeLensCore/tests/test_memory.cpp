#include <cassert>
#include <cmath>
#include <vector>
#include "lifelens/Memory.h"

using namespace lifelens;

static bool near(double a,double b,double eps=1e-9){return std::fabs(a-b)<=eps;}

int main()
{
    MemoryRecord direct;
    direct.who=20;
    direct.what="helped with food";
    direct.where="kitchen";
    direct.minute=100;
    direct.importance=0.7;
    direct.confidence=0.9;
    direct.emotionValence=0.6;
    direct.emotionIntensity=0.5;
    direct.witnessed=true;
    direct.source=MemorySource::DirectWitness;
    direct.tags={"help","food","positive"};
    direct.normalize();

    MemoryRecord told=direct;
    told.source=MemorySource::ToldByOther;
    told.witnessed=false;
    told.sourceCharacter=30;

    MemoryRecord inferred=direct;
    inferred.source=MemorySource::Inferred;
    inferred.witnessed=false;

    const int later=100+1440*5;
    assert(direct.effectiveConfidence(later)>told.effectiveConfidence(later));
    assert(told.effectiveConfidence(later)>inferred.effectiveConfidence(later));
    assert(direct.effectiveConfidence(later)<direct.confidence);

    MemoryRecord important=direct;
    important.importance=1.0;
    important.emotionIntensity=1.0;
    important.decayPerDay=0.08;

    MemoryRecord trivial=direct;
    trivial.importance=0.05;
    trivial.emotionIntensity=0.0;
    trivial.decayPerDay=0.08;

    assert(important.effectiveConfidence(later)>trivial.effectiveConfidence(later));

    MemoryState state;
    state.add(direct);

    MemoryRecord conflict;
    conflict.who=40;
    conflict.what="argument in hallway";
    conflict.where="hallway";
    conflict.minute=later-30;
    conflict.importance=0.65;
    conflict.confidence=0.95;
    conflict.emotionValence=-0.8;
    conflict.emotionIntensity=0.9;
    conflict.tags={"conflict","anger"};
    state.add(conflict);

    MemoryRecord rumor;
    rumor.who=20;
    rumor.what="heard a rumor";
    rumor.where="livingroom";
    rumor.minute=later-10;
    rumor.importance=0.2;
    rumor.confidence=0.7;
    rumor.witnessed=false;
    rumor.source=MemorySource::ToldByOther;
    rumor.tags={"rumor"};
    state.add(rumor);

    const MemoryRecord* conflictRecall=state.bestRecall(later,40,{"conflict"});
    assert(conflictRecall!=nullptr);
    assert(conflictRecall->who==40);
    assert(conflictRecall->what=="argument in hallway");

    const MemoryRecord* helpRecall=state.bestRecall(later,20,{"help","food"});
    assert(helpRecall!=nullptr);
    assert(helpRecall->who==20);
    assert(helpRecall->what=="helped with food");

    const auto conflictSet=state.recallAbove(later,0.45,40,{"conflict"});
    assert(!conflictSet.empty());
    assert(conflictSet.front()->who==40);

    MemoryRecord clamped;
    clamped.importance=2.0;
    clamped.confidence=-1.0;
    clamped.emotionValence=5.0;
    clamped.emotionIntensity=4.0;
    clamped.decayPerDay=-2.0;
    clamped.normalize();
    assert(near(clamped.importance,1.0));
    assert(near(clamped.confidence,0.0));
    assert(near(clamped.emotionValence,1.0));
    assert(near(clamped.emotionIntensity,1.0));
    assert(near(clamped.decayPerDay,0.0));

    return 0;
}
