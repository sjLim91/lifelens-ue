#pragma once

#include <algorithm>

namespace lifelens {

inline double clampDevelopment(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

struct ChildDevelopment {
    // Long-running developmental state shaped by caregiving and experience.
    double attachment=0.50;
    double confidence=0.50;
    double stress=0.10;
    double socialSkill=0.20;
    double emotionalSecurity=0.50;
    double disciplineInternalization=0.10;
    double learningSupport=0.20;
    double health=1.00;

    void normalize()
    {
        attachment=clampDevelopment(attachment);
        confidence=clampDevelopment(confidence);
        stress=clampDevelopment(stress);
        socialSkill=clampDevelopment(socialSkill);
        emotionalSecurity=clampDevelopment(emotionalSecurity);
        disciplineInternalization=clampDevelopment(disciplineInternalization);
        learningSupport=clampDevelopment(learningSupport);
        health=clampDevelopment(health);
    }
};

} // namespace lifelens
