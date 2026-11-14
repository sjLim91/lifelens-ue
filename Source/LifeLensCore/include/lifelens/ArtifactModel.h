#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "MaterialModel.h"
#include "Ids.h"

namespace lifelens {

// A bounded structural grammar, never a list of named finished recipes.
enum class ArtifactShape { Rod, Edge, Point, Bowl, Plate, Cord };
enum class ArtifactJoint { Integral, Bound, Inserted };
enum class ArtifactFunction { Cutting, Digging, Carrying, Heating };
struct ArtifactPart {
    MaterialKind material=MaterialKind::Unknown;
    ArtifactShape shape=ArtifactShape::Rod;
    ArtifactJoint joint=ArtifactJoint::Integral;
    int size=1; // raw units; also determines mass/reach/capacity
};
struct ArtifactDesign { std::vector<ArtifactPart> parts; };
struct ArtifactPerformance {
    double cutting=0, digging=0, carrying=0, heating=0;
    double durability=0, mass=0, work=0, heatResistance=0;
};
inline std::uint64_t artifactMix(std::uint64_t n) {
    n+=0x9e3779b97f4a7c15ULL; n=(n^(n>>30))*0xbf58476d1ce4e5b9ULL;
    n=(n^(n>>27))*0x94d049bb133111ebULL; return n^(n>>31);
}
inline bool validArtifactDesign(const ArtifactDesign& d) {
    if(d.parts.empty() ||d.parts.size()>4) return false;
    for(std::size_t i=0;i<d.parts.size();++i){
        const auto& p=d.parts[i];
        if(p.material<=MaterialKind::Unknown ||p.material>MaterialKind::Bronze
           ||p.material==MaterialKind::Water ||p.material==MaterialKind::PlantFood
           ||p.shape<ArtifactShape::Rod ||p.shape>ArtifactShape::Cord
           ||p.joint<ArtifactJoint::Integral ||p.joint>ArtifactJoint::Inserted
           ||p.size<1 ||p.size>3 ||(i==0 &&p.joint!=ArtifactJoint::Integral)) return false;
        if(i>0 &&p.joint==ArtifactJoint::Integral) return false;
    }
    return true;
}
inline std::uint64_t artifactDesignId(const ArtifactDesign& d) {
    std::uint64_t id=0x4152544946414354ULL;
    for(const auto& p:d.parts) id=artifactMix(id^(
        static_cast<std::uint64_t>(p.material)*4096+
        static_cast<unsigned>(p.shape)*128+static_cast<unsigned>(p.joint)*8+p.size));
    return id ? id : 1;
}
inline ArtifactPerformance measureArtifact(const ArtifactDesign& d) {
    ArtifactPerformance r;
    if(!validArtifactDesign(d)) return r;
    double joint=1, reach=1, binding=0, brittleness=0;
    bool hasBound=false;
    for(const auto& p:d.parts){
        const auto m=materialProperties(p.material);
        r.mass+=p.size*(0.4+1.6*m.hardness);
        r.work+=4+3*p.size+4*m.hardness;
        brittleness+=m.brittleness;
        if(p.shape==ArtifactShape::Cord) binding=std::max(binding,m.flexibility);
        if(p.shape==ArtifactShape::Rod) reach=std::max(reach,1.0+0.18*p.size);
        if(p.joint==ArtifactJoint::Bound) hasBound=true;
        if(p.joint==ArtifactJoint::Inserted) joint*=0.62+0.28*m.hardness;
        if(p.shape==ArtifactShape::Edge)
            r.cutting=std::max(r.cutting,m.sharpnessPotential*(0.4+0.6*m.hardness));
        if(p.shape==ArtifactShape::Point)
            r.digging=std::max(r.digging,m.hardness*(1-0.45*m.brittleness));
        if(p.shape==ArtifactShape::Bowl){
            // Porous stone/ore and raw charcoal do not become watertight vessels.
            double containment=p.material==MaterialKind::Clay?0.75:
                (p.material==MaterialKind::Hide?0.65:
                (p.material==MaterialKind::Wood?0.42:0.0));
            r.carrying=std::max(r.carrying,containment*p.size);
        }
        if(p.shape==ArtifactShape::Plate){
            r.heating=std::max(r.heating,m.heatResistance*(1-m.combustibility));
            r.heatResistance=std::max(r.heatResistance,m.heatResistance);
        }
    }
    if(hasBound) joint*=binding>=0.7 ? 0.55+0.4*binding : 0.08;
    r.cutting=std::clamp(r.cutting*joint*reach,0.0,1.0);
    r.digging=std::clamp(r.digging*joint*reach,0.0,1.0);
    r.carrying*=joint; r.heating*=joint;
    r.durability=std::clamp(0.85-0.35*brittleness/d.parts.size(),0.1,0.95)*joint;
    return r;
}
inline double artifactFunctionValue(const ArtifactPerformance& p,ArtifactFunction f) {
    switch(f){
        case ArtifactFunction::Cutting:return p.cutting;
        case ArtifactFunction::Digging:return p.digging;
        case ArtifactFunction::Carrying:return p.carrying;
        case ArtifactFunction::Heating:return p.heating;
    } return 0;
}
struct PhysicalArtifact {
    std::uint64_t id=0;
    CharacterId creator=0;
    int createdMinute=0;
    ArtifactDesign design;
    double durability=0;
    int uses=0, repairs=0;
};
enum class ArtifactKnowledgeSource { Experiment, Demonstration, Record };
struct ArtifactTechnique {
    ArtifactDesign design;
    CharacterId origin=0, source=0;
    ArtifactKnowledgeSource provenance=ArtifactKnowledgeSource::Experiment;
    int learnedMinute=0, trials=0, successes=0, uses=0, demonstrations=0;
    double confidence=0;
    bool reproducible() const { return successes>0 ||demonstrations>=2; }
};
struct ArtifactRecord {
    ArtifactDesign design;
    CharacterId author=0;
    int minute=0;
};
enum class ArtifactEventKind { Failed, Created, Used, Broken, Repaired, Taught, Rejected, Recorded, Rediscovered };
struct ArtifactEvent {
    ArtifactEventKind kind=ArtifactEventKind::Failed;
    std::uint64_t artifact=0, design=0;
    CharacterId other=0;
    int minute=0, rawUnits=0;
    double work=0, effect=0;
};
struct ArtifactLearning {
    std::vector<ArtifactTechnique> techniques;
    std::vector<ArtifactEvent> ledger;
    std::uint64_t sequence=0;
    void record(ArtifactEvent e) {
        if(ledger.size()>=128) ledger.erase(ledger.begin());
        ledger.push_back(e);
    }
    ArtifactTechnique* find(std::uint64_t id) {
        for(auto& t:techniques) if(artifactDesignId(t.design)==id) return &t;
        return nullptr;
    }
};
}
