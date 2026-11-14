#pragma once
#include <unordered_set>
#include "World.h"

namespace lifelens {
inline constexpr char ArtifactTailMagic[]={'L','L','A','R','T','E','N','D'};
inline bool artifactUnit(double x){return std::isfinite(x)&&x>=0&&x<=1;}
inline bool validArtifactWorld(const World& w) {
    std::unordered_set<CharacterId> residents;
    for(const auto& c:w.characters)residents.insert(c.id);
    std::unordered_set<std::uint64_t> ids;
    const auto inventoryValid=[&](const Inventory& inv){
        if(inv.artifacts.size()>32 ||inv.artifactRecords.size()>32)return false;
        for(const auto& a:inv.artifacts){
            if(!a.id ||!ids.insert(a.id).second ||!residents.count(a.creator)
               ||a.createdMinute<0 ||a.createdMinute>w.minute ||!validArtifactDesign(a.design)
               ||!artifactUnit(a.durability) ||a.durability>measureArtifact(a.design).durability+1e-12
               ||a.uses<0 ||a.uses>100000000 ||a.repairs<0 ||a.repairs>6)return false;
        }
        std::unordered_set<std::uint64_t> records;
        for(const auto& r:inv.artifactRecords)
            if(!validArtifactDesign(r.design) ||!residents.count(r.author)
               ||r.minute<0 ||r.minute>w.minute ||!records.insert(artifactDesignId(r.design)).second)return false;
        return true;
    };
    for(const auto& c:w.characters){
        if(!inventoryValid(c.civilization.inventory))return false;
        const auto& l=c.civilization.invention;
        if(l.techniques.size()>64 ||l.ledger.size()>128)return false;
        std::unordered_set<std::uint64_t> designs;
        for(const auto& t:l.techniques){
            if(!validArtifactDesign(t.design) ||!designs.insert(artifactDesignId(t.design)).second
               ||!residents.count(t.origin) ||!residents.count(t.source)
               ||t.provenance<ArtifactKnowledgeSource::Experiment ||t.provenance>ArtifactKnowledgeSource::Record
               ||t.learnedMinute<0 ||t.learnedMinute>w.minute ||t.trials<0 ||t.successes<0
               ||t.trials>100000000 ||t.successes>t.trials ||t.uses<0 ||t.uses>100000000 ||t.demonstrations<0 ||t.demonstrations>100000000 ||!artifactUnit(t.confidence))return false;
        }
        int previous=-1;
        for(const auto& e:l.ledger){
            if(e.kind<ArtifactEventKind::Failed ||e.kind>ArtifactEventKind::Rediscovered
               ||!e.design ||e.minute<previous ||e.minute<0 ||e.minute>w.minute
               ||(e.other &&!residents.count(e.other)) ||e.rawUnits<0 ||e.rawUnits>12
               ||!std::isfinite(e.work) ||e.work<0 ||e.work>200
               ||!std::isfinite(e.effect) ||e.effect<0 ||e.effect>100000)return false;
            previous=e.minute;
        }
    }
    for(const auto& s:w.storageSites)if(!inventoryValid(s.inventory))return false;
    return true;
}
inline bool hasArtifactState(const World& w) {
    for(const auto& c:w.characters)
        if(!c.civilization.inventory.artifacts.empty() ||!c.civilization.inventory.artifactRecords.empty()
           ||!c.civilization.invention.techniques.empty() ||!c.civilization.invention.ledger.empty()
           ||c.civilization.invention.sequence)return true;
    for(const auto& s:w.storageSites)if(!s.inventory.artifacts.empty() ||!s.inventory.artifactRecords.empty())return true;
    return false;
}
template<class W> void writeArtifactDesign(W& w,const ArtifactDesign& d) {
    w.u32(static_cast<std::uint32_t>(d.parts.size()));
    for(const auto& p:d.parts){w.enumeration(p.material);w.enumeration(p.shape);w.enumeration(p.joint);w.i32(p.size);}
}
template<class R> bool readArtifactDesign(R& r,ArtifactDesign& d) {
    std::uint32_t n=0;if(!r.count(n)||!n||n>4)return false;
    for(std::uint32_t i=0;i<n;++i){ArtifactPart p;
        if(!r.enumeration(p.material)||!r.enumeration(p.shape)||!r.enumeration(p.joint)||!r.i32(p.size))return false;
        d.parts.push_back(p);
    }return validArtifactDesign(d);
}
template<class W> void writeArtifactInventory(W& w,const Inventory& inv) {
    w.u32(static_cast<std::uint32_t>(inv.artifacts.size()));
    for(const auto& a:inv.artifacts){
        w.u64(a.id);w.u64(a.creator);w.i32(a.createdMinute);writeArtifactDesign(w,a.design);
        w.real(a.durability);w.i32(a.uses);w.i32(a.repairs);
    }
    w.u32(static_cast<std::uint32_t>(inv.artifactRecords.size()));
    for(const auto& a:inv.artifactRecords){writeArtifactDesign(w,a.design);w.u64(a.author);w.i32(a.minute);}
}
template<class R> bool readArtifactInventory(R& r,Inventory& inv) {
    std::uint32_t n=0;if(!r.count(n)||n>32)return false;
    for(std::uint32_t i=0;i<n;++i){PhysicalArtifact a;
        if(!r.u64(a.id)||!r.u64(a.creator)||!r.i32(a.createdMinute)||!readArtifactDesign(r,a.design)
           ||!r.real(a.durability)||!r.i32(a.uses)||!r.i32(a.repairs))return false;
        inv.artifacts.push_back(std::move(a));
    }
    if(!r.count(n)||n>32)return false;
    for(std::uint32_t i=0;i<n;++i){ArtifactRecord a;
        if(!readArtifactDesign(r,a.design)||!r.u64(a.author)||!r.i32(a.minute))return false;
        inv.artifactRecords.push_back(std::move(a));
    }return true;
}
template<class W> void writeArtifactState(W& w,const World& world) {
    w.u32(1);w.u32(static_cast<std::uint32_t>(world.characters.size()));
    for(const auto& c:world.characters){
        w.u64(c.id);writeArtifactInventory(w,c.civilization.inventory);
        const auto& l=c.civilization.invention;w.u64(l.sequence);
        w.u32(static_cast<std::uint32_t>(l.techniques.size()));
        for(const auto& t:l.techniques){
            writeArtifactDesign(w,t.design);w.u64(t.origin);w.u64(t.source);w.enumeration(t.provenance);
            w.i32(t.learnedMinute);w.i32(t.trials);w.i32(t.successes);w.i32(t.uses);w.i32(t.demonstrations);w.real(t.confidence);
        }
        w.u32(static_cast<std::uint32_t>(l.ledger.size()));
        for(const auto& e:l.ledger){
            w.enumeration(e.kind);w.u64(e.artifact);w.u64(e.design);w.u64(e.other);
            w.i32(e.minute);w.i32(e.rawUnits);w.real(e.work);w.real(e.effect);
        }
    }
    w.u32(static_cast<std::uint32_t>(world.storageSites.size()));
    for(const auto& s:world.storageSites){w.u64(s.id);writeArtifactInventory(w,s.inventory);}
}
template<class R> bool readArtifactState(R& r,World& world) {
    std::uint32_t version=0,n=0;
    if(!r.u32(version)||version!=1||!r.count(n)||n!=world.characters.size())return false;
    std::unordered_set<CharacterId> seen;
    for(std::uint32_t i=0;i<n;++i){
        CharacterId id=0;if(!r.u64(id)||!seen.insert(id).second)return false;
        auto it=std::find_if(world.characters.begin(),world.characters.end(),[&](const Character& c){return c.id==id;});
        if(it==world.characters.end() ||!readArtifactInventory(r,it->civilization.inventory))return false;
        auto& l=it->civilization.invention;std::uint32_t count=0;
        if(!r.u64(l.sequence)||!r.count(count)||count>64)return false;
        for(std::uint32_t j=0;j<count;++j){ArtifactTechnique t;
            if(!readArtifactDesign(r,t.design)||!r.u64(t.origin)||!r.u64(t.source)||!r.enumeration(t.provenance)
               ||!r.i32(t.learnedMinute)||!r.i32(t.trials)||!r.i32(t.successes)||!r.i32(t.uses)
               ||!r.i32(t.demonstrations)||!r.real(t.confidence))return false;
            l.techniques.push_back(std::move(t));
        }
        if(!r.count(count)||count>128)return false;
        for(std::uint32_t j=0;j<count;++j){ArtifactEvent e;
            if(!r.enumeration(e.kind)||!r.u64(e.artifact)||!r.u64(e.design)||!r.u64(e.other)
               ||!r.i32(e.minute)||!r.i32(e.rawUnits)||!r.real(e.work)||!r.real(e.effect))return false;
            l.ledger.push_back(e);
        }
    }
    if(!r.count(n)||n!=world.storageSites.size())return false;
    std::unordered_set<StorageId> storages;
    for(std::uint32_t i=0;i<n;++i){StorageId id=0;if(!r.u64(id)||!storages.insert(id).second)return false;
        auto it=std::find_if(world.storageSites.begin(),world.storageSites.end(),[&](const StorageSite& s){return s.id==id;});
        if(it==world.storageSites.end()||!readArtifactInventory(r,it->inventory))return false;
    }return validArtifactWorld(world);
}
}
