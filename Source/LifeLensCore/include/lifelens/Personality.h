#pragma once
#include <algorithm>
#include <random>
namespace lifelens {
struct Personality {
    double introversion=0.5, conscientiousness=0.5, openness=0.5, agreeableness=0.5;
    double emotionalStability=0.5, empathy=0.5, impulsiveness=0.5, riskTolerance=0.5;
    double ambition=0.5, patience=0.5, sociability=0.5, curiosity=0.5, orderliness=0.5, adaptability=0.5;
    static Personality generate(std::mt19937_64& rng) {
        std::normal_distribution<double> d(0.5,0.15);
        auto c=[&](){return std::max(0.0,std::min(1.0,d(rng)));};
        Personality p; p.introversion=c(); p.conscientiousness=c(); p.openness=c(); p.agreeableness=c();
        p.emotionalStability=c(); p.empathy=c(); p.impulsiveness=c(); p.riskTolerance=c(); p.ambition=c();
        p.patience=c(); p.sociability=c(); p.curiosity=c(); p.orderliness=c(); p.adaptability=c(); return p;
    }
};
}
