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

    // Long-run action_experience has several different sources; primaryTag
    // counts alone cannot identify which Core learning lane grew.
    MemoryState attempts;
    MemoryRecord remote=a;
    remote.tags={"action_experience","CoreContext:Trade","failure"};
    remote.where="core-context-token:451";
    remote.minute=990;
    attempts.add(remote);
    MemoryRecord local=remote;
    local.tags[2]="success";
    local.where="core-local-exchange-fact:98";
    local.minute=995;
    attempts.add(local);
    MemoryRecord report=remote;
    report.tags[1]="CoreContext:Teaching";
    report.source=MemorySource::ToldByOther;
    report.witnessed=false;
    report.sourceCharacter=2;
    attempts.add(report);
    MemoryRecord oldSource=remote;
    oldSource.tags[1]="Civilization:Gather:Water";
    oldSource.sourceCharacter=0;
    oldSource.where="personal experience";
    attempts.add(oldSource);
    const auto pristine=attempts.entries;
    const auto audit=observeMemoryAttribution(attempts,1000);
    assert(audit.entries==4);
    assert(audit.primaryTags.at("action_experience").entries==4);
    assert(audit.actionKinds.size()==3);
    const auto& trade=audit.actionKinds.at("CoreContext:Trade");
    assert(trade.entries==2 &&trade.successful==1 &&trade.failed==1);
    assert(trade.witnessedDirect==2 &&trade.indirectOrUnverified==0);
    assert(trade.claimedContextTokens==1
        &&trade.claimedLocalExchangeFacts==1);
    assert(trade.utf8TextBytes>0);
    const auto& reported=audit.actionKinds.at("CoreContext:Teaching");
    assert(reported.entries==1 &&reported.failed==1);
    assert(reported.witnessedDirect==0
        &&reported.indirectOrUnverified==1);
    const auto& legacy=audit.actionKinds.at("Civilization:Gather:Water");
    assert(legacy.witnessedDirect==0
        &&legacy.indirectOrUnverified==1);
    assert(legacy.claimedContextTokens==0);
    assert(attempts.entries.size()==pristine.size());
    for(std::size_t i=0;i<pristine.size();++i)
        assert(sameMemoryRecord(attempts.entries[i],pristine[i]));

    // A 100+ unique social-fact ID population must not exhaust the 64
    // reporting categories or disguise dominant social-memory growth.
    MemoryState facts;
    for(int i=0;i<100;++i){
        auto sample=a;
        sample.tags={"fact:"+std::to_string(i+1),"direct-witness"};
        facts.add(sample);
    }
    auto told=a;told.tags={"fact:101","rumor"};
    told.source=MemorySource::ToldByOther;
    told.witnessed=false;
    facts.add(told);
    auto inferred=a;inferred.tags={"fact:102","inference"};
    inferred.source=MemorySource::Inferred;
    inferred.witnessed=false;
    facts.add(inferred);
    auto other=a;other.tags={"environment"};
    facts.add(other);
    const auto exactRecords=facts.entries;
    const auto grouped=observeMemoryAttribution(facts,1000);
    assert(grouped.primaryTags.size()==4);
    assert(grouped.primaryTags.at("social_fact:direct").entries==100);
    assert(grouped.primaryTags.at("social_fact:told").entries==1);
    assert(grouped.primaryTags.at("social_fact:inferred").entries==1);
    assert(grouped.primaryTags.at("environment").entries==1);
    std::uint64_t categories=0,textBytes=0;
    for(const auto& category:grouped.primaryTags){
        categories+=category.second.entries;
        textBytes+=category.second.textBytes;
    }
    assert(categories==grouped.entries &&textBytes==grouped.textBytes);
    assert(grouped.exactDuplicates==0);
    for(std::size_t i=0;i<exactRecords.size();++i)
        assert(sameMemoryRecord(exactRecords[i],facts.entries[i]));
    auto malformed=a;malformed.tags={"fact:not-an-id"};
    assert(memoryAttributionCategoryName(malformed)=="fact:not-an-id");
}
