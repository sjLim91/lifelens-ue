#pragma once
#include <map>
#include <ostream>
#include "World.h"

namespace lifelens {
inline void appendArtifactDesignJson(std::ostream& out,const ArtifactDesign& d){
    out<<"[";
    for(std::size_t i=0;i<d.parts.size();++i){const auto& p=d.parts[i];if(i)out<<",";
        out<<"{\"material\":\""<<materialName(p.material)<<"\",\"shape\":"<<static_cast<int>(p.shape)
            <<",\"joint\":"<<static_cast<int>(p.joint)<<",\"size\":"<<p.size<<"}";}
    out<<"]";
}
inline void appendArtifactJson(std::ostream& out,const PhysicalArtifact& a){
    const auto p=measureArtifact(a.design);
    out<<"{\"id\":\""<<a.id<<"\",\"designId\":\""<<artifactDesignId(a.design)
        <<"\",\"creatorId\":\""<<a.creator<<"\",\"createdMinute\":"<<a.createdMinute
        <<",\"durability\":"<<a.durability<<",\"uses\":"<<a.uses<<",\"repairs\":"<<a.repairs
        <<",\"cutting\":"<<p.cutting<<",\"digging\":"<<p.digging<<",\"carrying\":"<<p.carrying
        <<",\"heating\":"<<p.heating<<",\"parts\":";
    appendArtifactDesignJson(out,a.design);out<<"}";
}
inline void appendResidentInventionJson(std::ostream& out,const Character* c){
    out<<"{\"artifacts\":[";
    if(c){for(std::size_t i=0;i<c->civilization.inventory.artifacts.size();++i){if(i)out<<",";
        appendArtifactJson(out,c->civilization.inventory.artifacts[i]);}}
    out<<"],\"techniques\":[";
    if(c){const auto& ts=c->civilization.invention.techniques;
        for(std::size_t i=0;i<ts.size();++i){if(i)out<<",";const auto& t=ts[i];
            out<<"{\"designId\":\""<<artifactDesignId(t.design)<<"\",\"originId\":\""<<t.origin
                <<"\",\"sourceId\":\""<<t.source<<"\",\"source\":"<<static_cast<int>(t.provenance)
                <<",\"trials\":"<<t.trials<<",\"successes\":"<<t.successes<<",\"uses\":"<<t.uses
                <<",\"demonstrations\":"<<t.demonstrations<<",\"confidence\":"<<t.confidence
                <<",\"reproducible\":"<<(t.reproducible()?"true":"false")<<",\"parts\":";
            appendArtifactDesignJson(out,t.design);out<<"}";}}
    out<<"],\"events\":[";
    if(c){const auto& events=c->civilization.invention.ledger;
        const auto begin=events.size()>12?events.size()-12:0;
        for(std::size_t i=begin;i<events.size();++i){if(i>begin)out<<",";const auto& e=events[i];
            out<<"{\"kind\":"<<static_cast<int>(e.kind)<<",\"artifactId\":\""<<e.artifact
                <<"\",\"designId\":\""<<e.design<<"\",\"otherId\":\""<<e.other
                <<"\",\"minute\":"<<e.minute<<",\"rawUnits\":"<<e.rawUnits
                <<",\"work\":"<<e.work<<",\"effect\":"<<e.effect<<"}";}}
    out<<"],\"recordCount\":"<<(c?c->civilization.inventory.artifactRecords.size():0)<<"}";
}
struct ArtifactCulture {
    std::uint64_t design=0;
    int livingKnowers=0, historicalKnowers=0, livingMakers=0, workingArtifacts=0, uses=0, repairs=0, records=0;
};
inline std::vector<ArtifactCulture> observeArtifactCultures(const World& w){
    std::map<std::uint64_t,ArtifactCulture> groups;
    for(const auto& c:w.characters){
        for(const auto& t:c.civilization.invention.techniques){
            auto& g=groups[artifactDesignId(t.design)];g.design=artifactDesignId(t.design);
            if(t.reproducible()){++g.historicalKnowers;if(c.alive)++g.livingKnowers;}
            if(c.alive&&t.successes>0)++g.livingMakers;
        }
        for(const auto& a:c.civilization.inventory.artifacts){auto& g=groups[artifactDesignId(a.design)];
            g.design=artifactDesignId(a.design);g.workingArtifacts+=c.alive&&a.durability>0.02;
            g.uses+=a.uses;g.repairs+=a.repairs;}
        for(const auto& r:c.civilization.inventory.artifactRecords){auto& g=groups[artifactDesignId(r.design)];
            g.design=artifactDesignId(r.design);++g.records;}
    }
    for(const auto& s:w.storageSites){
        for(const auto& a:s.inventory.artifacts){auto& g=groups[artifactDesignId(a.design)];
            g.design=artifactDesignId(a.design);g.workingArtifacts+=a.durability>0.02;g.uses+=a.uses;g.repairs+=a.repairs;}
        for(const auto& r:s.inventory.artifactRecords){auto& g=groups[artifactDesignId(r.design)];g.design=artifactDesignId(r.design);++g.records;}
    }
    std::vector<ArtifactCulture> result;for(const auto& pair:groups)result.push_back(pair.second);return result;
}
inline void appendArtifactCulturesJson(std::ostream& out,const World& w){
    out<<"[";bool first=true;
    for(const auto& g:observeArtifactCultures(w)){if(!first)out<<",";first=false;
        out<<"{\"designId\":\""<<g.design<<"\",\"livingKnowers\":"<<g.livingKnowers
            <<",\"historicalKnowers\":"<<g.historicalKnowers<<",\"livingMakers\":"<<g.livingMakers
            <<",\"workingArtifacts\":"<<g.workingArtifacts<<",\"uses\":"<<g.uses
            <<",\"repairs\":"<<g.repairs<<",\"records\":"<<g.records<<"}";}
    out<<"]";
}
// Construction patterns describe actual completed, used structures. A matching
// material bill is observational evidence, never a village-level unlock.
inline void appendConstructionPatternsJson(std::ostream& out,const World& w){
    struct Pattern {int completed=0,operational=0,uses=0;std::map<int,int> materials;};
    std::map<int,Pattern> patterns;
    for(const auto& f:w.facilities){
        if(f.completedMinute<0 ||f.constructionWork<f.requiredWork)continue;
        auto& p=patterns[static_cast<int>(f.kind)];++p.completed;
        p.operational+=f.state==FacilityState::Operational&&f.active&&f.durability>0;
        p.uses+=f.usageCount;
        for(const auto& input:f.requirements)if(input.delivered>0)
            p.materials[static_cast<int>(input.material)]+=input.delivered;
    }
    out<<"[";bool first=true;
    for(const auto& pair:patterns){if(!first)out<<",";first=false;const auto& p=pair.second;
        out<<"{\"facilityKind\":"<<pair.first<<",\"completed\":"<<p.completed
            <<",\"operational\":"<<p.operational<<",\"uses\":"<<p.uses<<",\"materials\":[";
        bool firstMaterial=true;
        for(const auto& input:p.materials){if(!firstMaterial)out<<",";firstMaterial=false;
            out<<"{\"material\":\""<<materialName(static_cast<MaterialKind>(input.first))
                <<"\",\"delivered\":"<<input.second<<"}";}
        out<<"]}";
    }out<<"]";
}
}
