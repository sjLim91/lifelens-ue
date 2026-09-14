#pragma once

#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <utility>
#include <vector>

#include "WitnessRumor.h"

namespace lifelens {

constexpr char SocialKnowledgeSnapshotExtensionMagic[]={'L','L','K','N','W','0','0','1'};
constexpr std::uint32_t SocialKnowledgeSnapshotExtensionVersion=1;

inline bool validReceiptMemorySource(MemorySource source)
{
    return source==MemorySource::DirectWitness || source==MemorySource::ToldByOther;
}

template<typename WriterT>
void writeSocialFactSnapshot(WriterT& w,const SocialFact& fact)
{
    w.u64(fact.id);
    w.u64(fact.subject);
    w.string(fact.proposition);
    w.string(fact.where);
    w.i32(fact.eventMinute);
    w.boolean(fact.supports);
    w.real(fact.importance);
    w.real(fact.confidence);
    w.real(fact.emotionValence);
    w.real(fact.emotionIntensity);
}

template<typename ReaderT>
bool readSocialFactSnapshot(ReaderT& r,SocialFact& fact)
{
    return r.u64(fact.id)
        && r.u64(fact.subject)
        && r.string(fact.proposition)
        && r.string(fact.where)
        && r.i32(fact.eventMinute)
        && r.boolean(fact.supports)
        && r.real(fact.importance)
        && r.real(fact.confidence)
        && r.real(fact.emotionValence)
        && r.real(fact.emotionIntensity)
        && fact.valid()
        && fact.eventMinute>=0
        && fact.importance>=0.0 && fact.importance<=1.0
        && fact.confidence>=0.0 && fact.confidence<=1.0
        && fact.emotionValence>=-1.0 && fact.emotionValence<=1.0
        && fact.emotionIntensity>=0.0 && fact.emotionIntensity<=1.0;
}

template<typename WriterT>
void writeKnowledgeReceiptSnapshot(WriterT& w,const KnowledgeReceipt& receipt)
{
    w.u64(receipt.factId);
    w.u64(receipt.holder);
    w.u64(receipt.originWitness);
    w.u64(receipt.immediateSource);
    w.u64(receipt.subject);
    w.boolean(receipt.supports);
    w.real(receipt.confidence);
    w.i32(receipt.learnedMinute);
    w.enumeration(receipt.source);
    w.u32(static_cast<std::uint32_t>(receipt.transmissionPath.size()));
    for(CharacterId id:receipt.transmissionPath) w.u64(id);
}

template<typename ReaderT>
bool readKnowledgeReceiptSnapshot(ReaderT& r,KnowledgeReceipt& receipt)
{
    if(!r.u64(receipt.factId)
       || !r.u64(receipt.holder)
       || !r.u64(receipt.originWitness)
       || !r.u64(receipt.immediateSource)
       || !r.u64(receipt.subject)
       || !r.boolean(receipt.supports)
       || !r.real(receipt.confidence)
       || !r.i32(receipt.learnedMinute)
       || !r.enumeration(receipt.source)) return false;

    std::uint32_t pathCount=0;
    if(!r.count(pathCount) || pathCount==0) return false;
    receipt.transmissionPath.clear();
    receipt.transmissionPath.reserve(pathCount);
    for(std::uint32_t i=0;i<pathCount;++i){
        CharacterId id=0;
        if(!r.u64(id) || id==0) return false;
        receipt.transmissionPath.push_back(id);
    }

    return receipt.factId!=0
        && receipt.holder!=0
        && receipt.originWitness!=0
        && receipt.immediateSource!=0
        && receipt.subject!=0
        && receipt.confidence>=0.0 && receipt.confidence<=1.0
        && receipt.learnedMinute>=0
        && validReceiptMemorySource(receipt.source);
}

template<typename WriterT>
void writeSocialKnowledgeSnapshotExtension(WriterT& w,const SocialKnowledgeBook& book)
{
    w.raw(SocialKnowledgeSnapshotExtensionMagic,sizeof(SocialKnowledgeSnapshotExtensionMagic));
    w.u32(SocialKnowledgeSnapshotExtensionVersion);

    w.u32(static_cast<std::uint32_t>(book.facts().size()));
    for(const SocialFact& fact:book.facts()) writeSocialFactSnapshot(w,fact);

    w.u32(static_cast<std::uint32_t>(book.receipts().size()));
    for(const KnowledgeReceipt& receipt:book.receipts()) writeKnowledgeReceiptSnapshot(w,receipt);
}

template<typename ReaderT>
bool readSocialKnowledgeSnapshotExtension(ReaderT& r,SocialKnowledgeBook& book)
{
    char magic[sizeof(SocialKnowledgeSnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    if(!r.raw(magic,sizeof(magic))
       || std::memcmp(magic,SocialKnowledgeSnapshotExtensionMagic,sizeof(magic))!=0
       || !r.u32(version)
       || version!=SocialKnowledgeSnapshotExtensionVersion) return false;

    std::uint32_t factCount=0;
    if(!r.count(factCount)) return false;
    std::vector<SocialFact> facts;
    facts.reserve(factCount);
    std::unordered_set<SocialFactId> factIds;
    for(std::uint32_t i=0;i<factCount;++i){
        SocialFact fact;
        if(!readSocialFactSnapshot(r,fact) || !factIds.insert(fact.id).second) return false;
        facts.push_back(std::move(fact));
    }

    std::uint32_t receiptCount=0;
    if(!r.count(receiptCount)) return false;
    std::vector<KnowledgeReceipt> receipts;
    receipts.reserve(receiptCount);
    for(std::uint32_t i=0;i<receiptCount;++i){
        KnowledgeReceipt receipt;
        if(!readKnowledgeReceiptSnapshot(r,receipt)) return false;
        receipts.push_back(std::move(receipt));
    }

    SocialKnowledgeBook restored;
    if(!restored.restoreState(std::move(facts),std::move(receipts))) return false;
    book=std::move(restored);
    return true;
}

} // namespace lifelens
