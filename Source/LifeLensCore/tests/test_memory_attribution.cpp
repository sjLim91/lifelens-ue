#include <cassert>
#include "lifelens/MemoryAttribution.h"
using namespace lifelens;
int main(){
    MemoryState memory;
    auto empty=observeMemoryAttribution(memory,1000);
    assert(empty.entries==0 &&empty.textBytes==0 &&empty.exactDuplicates==0);
    MemoryRecord a;
    a.who=1;a.sourceCharacter=1;a.what="real meal";a.where="home";
    a.minute=0;a.tags={"action_experience","success"};
    memory.add(a);memory.add(a);
    auto b=a;b.source=MemorySource::ToldByOther;b.witnessed=false;
    memory.add(b);
    auto future=a;future.minute=2000;
    memory.add(future);
    const auto before=memory.entries;
    const auto result=observeMemoryAttribution(memory,1000);
    assert(result.entries==4);
    assert(result.exactDuplicates==1);
    assert(result.sources[0]==3 &&result.sources[1]==1 &&result.sources[2]==0);
    assert(result.witnessedDirect==3 &&result.futureEntries==1);
    assert(result.ageBuckets[0]==3);
    assert(result.primaryTags.at("action_experience").entries==4);
    assert(result.textBytes==4*(a.what.size()+a.where.size()
        +a.tags[0].size()+a.tags[1].size()));
    for(std::size_t i=0;i<before.size();++i)
        assert(sameMemoryRecord(before[i],memory.entries[i]));
    auto altered=a;altered.emotionIntensity=0.25;
    assert(!sameMemoryRecord(a,altered));
}
