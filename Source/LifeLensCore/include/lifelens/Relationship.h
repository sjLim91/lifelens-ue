#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>
#include "Ids.h"

namespace lifelens {

inline double clampRelationship(double value)
{
    return std::max(0.0, std::min(1.0, value));
}

struct RelationshipDelta {
    double affection=0.0;
    double trust=0.0;
    double respect=0.0;
    double comfort=0.0;
    double familiarity=0.0;
    double attraction=0.0;
    double romanticInterest=0.0;
    double sexualAttraction=0.0;
    double commitment=0.0;
    double conflict=0.0;
    double jealousy=0.0;
    double fear=0.0;
    double grudge=0.0;
};

struct Relationship {
    // Relationship state is directional: how `from` currently feels about `to`.
    CharacterId from=0;
    CharacterId to=0;

    // SPEC 31 dimensions. Values are normalized to [0, 1].
    double affection=0.0;
    double trust=0.0;
    double respect=0.0;
    double comfort=0.0;
    double familiarity=0.0;
    double attraction=0.0;
    double romanticInterest=0.0;
    double sexualAttraction=0.0;
    double commitment=0.0;
    double conflict=0.0;
    double jealousy=0.0;
    double fear=0.0;
    double grudge=0.0;

    void apply(const RelationshipDelta& d)
    {
        affection=clampRelationship(affection+d.affection);
        trust=clampRelationship(trust+d.trust);
        respect=clampRelationship(respect+d.respect);
        comfort=clampRelationship(comfort+d.comfort);
        familiarity=clampRelationship(familiarity+d.familiarity);
        attraction=clampRelationship(attraction+d.attraction);
        romanticInterest=clampRelationship(romanticInterest+d.romanticInterest);
        sexualAttraction=clampRelationship(sexualAttraction+d.sexualAttraction);
        commitment=clampRelationship(commitment+d.commitment);
        conflict=clampRelationship(conflict+d.conflict);
        jealousy=clampRelationship(jealousy+d.jealousy);
        fear=clampRelationship(fear+d.fear);
        grudge=clampRelationship(grudge+d.grudge);
    }

    double socialBond() const
    {
        const double positive=(affection+trust+respect+comfort+familiarity)/5.0;
        const double negative=(conflict+fear+grudge)/3.0;
        return clampRelationship(positive-(0.65*negative));
    }

    double romancePotential() const
    {
        const double positive=(attraction+romanticInterest+affection+trust+comfort+familiarity)/6.0;
        const double negative=(conflict+fear+grudge+jealousy)/4.0;
        return clampRelationship(positive-(0.55*negative));
    }
};

enum class RelationshipEvent {
    SharedPositiveExperience,
    Helped,
    Comforted,
    Conflict,
    Betrayal,
    Rejection,
    Apology,
    Intimacy,
    CommitmentMade
};

inline RelationshipDelta relationshipDeltaFor(RelationshipEvent event, double intensity=1.0)
{
    const double k=clampRelationship(intensity);
    RelationshipDelta d;
    switch(event){
        case RelationshipEvent::SharedPositiveExperience:
            d.affection=0.05*k; d.comfort=0.04*k; d.familiarity=0.07*k; d.trust=0.02*k; break;
        case RelationshipEvent::Helped:
            d.affection=0.04*k; d.trust=0.06*k; d.respect=0.05*k; d.familiarity=0.03*k; break;
        case RelationshipEvent::Comforted:
            d.affection=0.06*k; d.trust=0.05*k; d.comfort=0.07*k; d.familiarity=0.03*k; break;
        case RelationshipEvent::Conflict:
            d.affection=-0.04*k; d.trust=-0.03*k; d.comfort=-0.05*k; d.conflict=0.10*k; d.grudge=0.03*k; break;
        case RelationshipEvent::Betrayal:
            d.affection=-0.10*k; d.trust=-0.22*k; d.respect=-0.10*k; d.comfort=-0.08*k;
            d.conflict=0.14*k; d.fear=0.05*k; d.grudge=0.16*k; break;
        case RelationshipEvent::Rejection:
            d.comfort=-0.04*k; d.romanticInterest=-0.12*k; d.conflict=0.03*k; d.grudge=0.02*k; break;
        case RelationshipEvent::Apology:
            d.trust=0.04*k; d.respect=0.03*k; d.comfort=0.03*k; d.conflict=-0.07*k; d.grudge=-0.06*k; break;
        case RelationshipEvent::Intimacy:
            d.affection=0.07*k; d.comfort=0.05*k; d.familiarity=0.04*k; d.attraction=0.05*k;
            d.romanticInterest=0.07*k; d.sexualAttraction=0.07*k; break;
        case RelationshipEvent::CommitmentMade:
            d.affection=0.05*k; d.trust=0.06*k; d.commitment=0.16*k; d.romanticInterest=0.04*k; break;
    }
    return d;
}

class RelationshipBook {
public:
    Relationship& getOrCreate(CharacterId from, CharacterId to)
    {
        for(auto& r:items_) if(r.from==from && r.to==to) return r;
        items_.push_back(Relationship{});
        Relationship& r=items_.back();
        r.from=from; r.to=to;
        return r;
    }

    const Relationship* find(CharacterId from, CharacterId to) const
    {
        for(const auto& r:items_) if(r.from==from && r.to==to) return &r;
        return nullptr;
    }

    std::size_t size() const { return items_.size(); }
    const std::vector<Relationship>& all() const { return items_; }

private:
    std::vector<Relationship> items_;
};

}
