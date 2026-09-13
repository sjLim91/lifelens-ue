#pragma once

#include <algorithm>
#include <random>

namespace lifelens {

inline double clampGenetic(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

struct GeneticsProfile {
    // Normalized continuous traits. Presentation systems can map these to meshes/materials later.
    double faceShape=0.5;
    double eyePigment=0.5;
    double hairPigment=0.5;
    double skinTone=0.5;
    double heightPotential=0.5;
    double buildPotential=0.5;
    double healthPotential=0.5;
    double learningPotential=0.5;
    double temperamentSensitivity=0.5;
};

inline GeneticsProfile generateFounderGenetics(std::mt19937_64& rng)
{
    std::normal_distribution<double> distribution(0.5,0.16);
    auto sample=[&](){ return clampGenetic(distribution(rng)); };
    GeneticsProfile g;
    g.faceShape=sample();
    g.eyePigment=sample();
    g.hairPigment=sample();
    g.skinTone=sample();
    g.heightPotential=sample();
    g.buildPotential=sample();
    g.healthPotential=sample();
    g.learningPotential=sample();
    g.temperamentSensitivity=sample();
    return g;
}

inline double inheritTrait(double a,double b,std::mt19937_64& rng,double variation=0.08)
{
    std::normal_distribution<double> mutation(0.0,std::max(0.0,variation));
    // Each parent contributes equally; mutation supplies realistic sibling variation.
    return clampGenetic(0.5*clampGenetic(a)+0.5*clampGenetic(b)+mutation(rng));
}

inline GeneticsProfile inheritGenetics(
    const GeneticsProfile& parentA,
    const GeneticsProfile& parentB,
    std::mt19937_64& rng,
    double variation=0.08)
{
    GeneticsProfile child;
    child.faceShape=inheritTrait(parentA.faceShape,parentB.faceShape,rng,variation);
    child.eyePigment=inheritTrait(parentA.eyePigment,parentB.eyePigment,rng,variation);
    child.hairPigment=inheritTrait(parentA.hairPigment,parentB.hairPigment,rng,variation);
    child.skinTone=inheritTrait(parentA.skinTone,parentB.skinTone,rng,variation);
    child.heightPotential=inheritTrait(parentA.heightPotential,parentB.heightPotential,rng,variation);
    child.buildPotential=inheritTrait(parentA.buildPotential,parentB.buildPotential,rng,variation);
    child.healthPotential=inheritTrait(parentA.healthPotential,parentB.healthPotential,rng,variation*0.65);
    child.learningPotential=inheritTrait(parentA.learningPotential,parentB.learningPotential,rng,variation*0.75);
    child.temperamentSensitivity=inheritTrait(parentA.temperamentSensitivity,parentB.temperamentSensitivity,rng,variation);
    return child;
}

} // namespace lifelens
