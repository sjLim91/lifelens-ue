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
    // Unique social fact IDs must not exhaust the bounded category budget
    // and hide the provenance of the dominant social-record growth.
    MemoryState facts;
    for(int i=0;i<100;++i){
        auto fact=a;fact.tags={"fact:"+std::to_string(i+1),"direct-witness"};
        facts.add(fact);
    }
    auto told=a;told.tags={"fact:101","rumor"};
    told.source=MemorySource::ToldByOther;told.witnessed=false;facts.add(told);
    auto environment=a;environment.tags={"environment"};facts.add(environment);
    const auto originalFacts=facts.entries;
    const auto grouped=observeMemoryAttribution(facts,1000);
    assert(grouped.primaryTags.size()==3);
    assert(grouped.primaryTags.at("social_fact:direct").entries==100);
    assert(grouped.primaryTags.at("social_fact:told").entries==1);
    assert(grouped.primaryTags.at("environment").entries==1);
    assert(grouped.exactDuplicates==0);
    std::uint64_t categoryEntries=0,categoryBytes=0;
    for(const auto& group:grouped.primaryTags){
        categoryEntries+=group.second.entries;
        categoryBytes+=group.second.textBytes;
    }
    assert(categoryEntries==grouped.entries &&categoryBytes==grouped.textBytes);
    for(std::size_t i=0;i<originalFacts.size();++i)
        assert(sameMemoryRecord(originalFacts[i],facts.entries[i]));
    auto malformed=a;malformed.tags={"fact:not-an-id"};
    assert(memoryAttributionCategoryName(malformed)=="fact:not-an-id");
    auto altered=a;altered.emotionIntensity=0.25;
    assert(!sameMemoryRecord(a,altered));
}
