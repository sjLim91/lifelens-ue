#pragma once

#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <vector>

#include "Aging.h"
#include "Character.h"
#include "Genetics.h"

namespace lifelens {

inline double clampFounderValue(double value,double low,double high)
{
    return std::max(low,std::min(high,value));
}

inline Character generateFounder(
    CharacterId id,
    const std::string& name,
    Sex sex,
    std::mt19937_64& rng,
    int currentMinute)
{
    Character founder;
    founder.id=id;
    founder.name=name;
    founder.sex=sex;
    founder.personality=Personality::generate(rng);
    founder.genetics=generateFounderGenetics(rng);

    std::normal_distribution<double> needDistribution(0.20,0.07);
    auto need=[&](){ return clampFounderValue(needDistribution(rng),0.06,0.38); };
    founder.needs={need(),need(),need(),need(),need()};

    std::normal_distribution<double> bodyRate(1.0,0.08);
    founder.baseMetabolism=clampFounderValue(bodyRate(rng),0.82,1.18);
    founder.baseSleepTendency=clampFounderValue(bodyRate(rng),0.82,1.18);
    founder.metabolism=founder.baseMetabolism;
    founder.sleepTendency=founder.baseSleepTendency;

    // Civilization starts as personal capability, not globally unlocked recipes.
    // Skills are deterministic derivatives of the founder's own traits; all
    // technique knowledge remains Unknown at New Game.
    founder.civilization.character=id;
    founder.civilization.gatheringSkill=clampFounderValue(
        0.30+founder.personality.patience*0.24+founder.personality.adaptability*0.22+
        founder.personality.conscientiousness*0.14,0.18,0.90);
    founder.civilization.craftingSkill=clampFounderValue(
        0.24+founder.personality.openness*0.22+founder.personality.patience*0.22+
        founder.personality.conscientiousness*0.18,0.16,0.90);
    founder.civilization.learningSkill=clampFounderValue(
        0.24+founder.genetics.learningPotential*0.24+founder.personality.curiosity*0.22+
        founder.personality.openness*0.18,0.18,0.94);

    // Formal New Game founders start as independent adults. Keeping the birth
    // minute authoritative lets the existing Growth/Aging system advance them
    // naturally instead of storing a separate mutable age field.
    std::uniform_int_distribution<int> ageYears(25,34);
    std::uniform_int_distribution<int> birthdayOffset(0,LifeMinutesPerYear-1);
    const int age=ageYears(rng);
    founder.hasBirthMinute=true;
    founder.birthMinute=currentMinute-(age*LifeMinutesPerYear)-birthdayOffset(rng);
    founder.lifeStage=lifeStageForAgeYears(age);
    founder.lifeCondition=lifeConditionForAge(age,founder.genetics.healthPotential,0);
    founder.alive=true;
    founder.deathMinute=-1;
    return founder;
}

inline std::vector<Character> generateInitialFounders(std::mt19937_64& rng,int currentMinute)
{
    // Keep the pool in Core for the first authoritative slice. A later
    // data-driven layer may replace the content source without changing the
    // deterministic founder-generation contract.
    std::vector<std::string> maleNames={
        "Minjun","Doyun","Seojun","Jiho","Hyunwoo","Taeyun",
        "Junho","Siwoo","Gunwoo","Joon","Minseok","Jaeho"
    };
    std::vector<std::string> femaleNames={
        "Seoyun","Hayun","Jia","Sua","Minseo","Yerin",
        "Chaewon","Naeun","Jiwon","Yuna","Soyeon","Eunji"
    };

    std::shuffle(maleNames.begin(),maleNames.end(),rng);
    std::shuffle(femaleNames.begin(),femaleNames.end(),rng);

    std::vector<Character> founders;
    founders.reserve(4);
    founders.push_back(generateFounder(1,maleNames[0],Sex::Male,rng,currentMinute));
    founders.push_back(generateFounder(2,maleNames[1],Sex::Male,rng,currentMinute));
    founders.push_back(generateFounder(3,femaleNames[0],Sex::Female,rng,currentMinute));
    founders.push_back(generateFounder(4,femaleNames[1],Sex::Female,rng,currentMinute));
    return founders;
}

} // namespace lifelens
