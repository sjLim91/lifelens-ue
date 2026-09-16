#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SocialUtility.h"
#include "lifelens/TraitsPreferences.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static bool near(double a,double b)
{
    return std::abs(a-b)<1e-12;
}

static bool sameTraits(const TraitProfile& a,const TraitProfile& b)
{
    return near(a.resilience,b.resilience)
        && near(a.creativity,b.creativity)
        && near(a.discipline,b.discipline)
        && near(a.compassion,b.compassion)
        && near(a.adaptability,b.adaptability)
        && near(a.boldness,b.boldness)
        && near(a.perseverance,b.perseverance)
        && near(a.resourcefulness,b.resourcefulness);
}

static bool samePreferences(const PreferenceProfile& a,const PreferenceProfile& b)
{
    return near(a.socializing,b.socializing)
        && near(a.solitude,b.solitude)
        && near(a.exploration,b.exploration)
        && near(a.crafting,b.crafting)
        && near(a.gathering,b.gathering)
        && near(a.comfort,b.comfort)
        && near(a.novelty,b.novelty)
        && near(a.order,b.order);
}

static Character dispositionExtreme(CharacterId id,bool high)
{
    Character character;
    character.id=id;
    character.civilization.character=id;
    const double value=high ? 1.0 : 0.0;
    const double inverse=high ? 0.0 : 1.0;

    character.personality.introversion=inverse;
    character.personality.conscientiousness=value;
    character.personality.openness=value;
    character.personality.agreeableness=value;
    character.personality.emotionalStability=value;
    character.personality.empathy=value;
    character.personality.impulsiveness=inverse;
    character.personality.riskTolerance=value;
    character.personality.ambition=value;
    character.personality.patience=value;
    character.personality.sociability=value;
    character.personality.curiosity=value;
    character.personality.orderliness=value;
    character.personality.adaptability=value;
    character.genetics.healthPotential=value;
    character.genetics.learningPotential=value;
    return character;
}

int main()
{
    constexpr WorldSeed WorldSeedValue=20260916;
    constexpr PopulationSeed PopulationSeedValue=424242;

    Simulation first(WorldSeedValue,PopulationSeedValue);
    Simulation second(WorldSeedValue,PopulationSeedValue);
    Simulation differentPopulation(WorldSeedValue,PopulationSeedValue+1);
    first.setupNewGame();
    second.setupNewGame();
    differentPopulation.setupNewGame();

    CHECK(first.world().characters.size()==4);
    CHECK(second.world().characters.size()==4);
    CHECK(differentPopulation.world().characters.size()==4);

    bool sawPopulationVariation=false;
    for(std::size_t i=0;i<first.world().characters.size();++i){
        const Character& a=first.world().characters[i];
        const Character& b=second.world().characters[i];
        const Character& c=differentPopulation.world().characters[i];

        const TraitProfile aTraits=deriveTraitProfile(a.personality,a.genetics);
        const TraitProfile bTraits=deriveTraitProfile(b.personality,b.genetics);
        const TraitProfile cTraits=deriveTraitProfile(c.personality,c.genetics);
        const PreferenceProfile aPreferences=derivePreferenceProfile(a.personality,a.genetics);
        const PreferenceProfile bPreferences=derivePreferenceProfile(b.personality,b.genetics);
        const PreferenceProfile cPreferences=derivePreferenceProfile(c.personality,c.genetics);

        CHECK(validTraitProfile(aTraits));
        CHECK(validPreferenceProfile(aPreferences));
        CHECK(sameTraits(aTraits,bTraits));
        CHECK(samePreferences(aPreferences,bPreferences));

        if(!sameTraits(aTraits,cTraits) || !samePreferences(aPreferences,cPreferences)){
            sawPopulationVariation=true;
        }
    }
    CHECK(sawPopulationVariation);

    // No duplicate mutable serialization is required: personality/genetics are
    // persistent Core state, so the named trait/preference read profiles must
    // reproduce exactly after snapshot encode/decode.
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(first.captureSnapshot(),bytes,&error));
    CHECK(error.empty());

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(decoded.world.characters.size()==first.world().characters.size());

    for(std::size_t i=0;i<first.world().characters.size();++i){
        const Character& before=first.world().characters[i];
        const Character& after=decoded.world.characters[i];
        CHECK(sameTraits(
            deriveTraitProfile(before.personality,before.genetics),
            deriveTraitProfile(after.personality,after.genetics)));
        CHECK(samePreferences(
            derivePreferenceProfile(before.personality,before.genetics),
            derivePreferenceProfile(after.personality,after.genetics)));
    }

    // Named Traits/Preferences are not decorative Observer data. They bias the
    // actual unified social/civilization utility path within a deliberately
    // narrow +/-10% civilization band.
    Character high=dispositionExtreme(1001,true);
    Character low=dispositionExtreme(1002,false);

    for(const CivilizationIntent intent : {
        CivilizationIntent::Gather,
        CivilizationIntent::Store,
        CivilizationIntent::Experiment,
        CivilizationIntent::Craft}) {
        CivilizationUtilityDecision highCandidate;
        highCandidate.intent=intent;
        highCandidate.utility=0.50;
        CivilizationUtilityDecision lowCandidate=highCandidate;
        const CivilizationUtilityDecision highBiased=applyCivilizationDispositionBias(high,highCandidate);
        const CivilizationUtilityDecision lowBiased=applyCivilizationDispositionBias(low,lowCandidate);
        CHECK(highBiased.utility>lowBiased.utility);
        CHECK(highBiased.utility<=0.55+1e-12);
        CHECK(lowBiased.utility>=0.45-1e-12);
    }

    World socialWorld(WorldSeedValue,PopulationSeedValue);
    RelationshipBook emptyRelationships;
    const double highApproach=scoreApproachIntent(socialWorld,high,9999,emptyRelationships);
    const double lowApproach=scoreApproachIntent(socialWorld,low,9999,emptyRelationships);
    CHECK(highApproach>lowApproach);

    // Emergency provision gathering remains survival authority and deliberately
    // bypasses the personality disposition multiplier.
    World emergencyWorld(WorldSeedValue,PopulationSeedValue);
    emergencyWorld.resourceNodes.push_back({777,MaterialKind::PlantFood,20,20,true,1,{3,4}});
    high.needs.hunger=0.90;
    low.needs.hunger=0.90;
    const CivilizationUtilityDecision highEmergency=urgentSurvivalProvisionGatherDecision(emergencyWorld,high);
    const CivilizationUtilityDecision lowEmergency=urgentSurvivalProvisionGatherDecision(emergencyWorld,low);
    CHECK(highEmergency.intent==CivilizationIntent::Gather);
    CHECK(lowEmergency.intent==CivilizationIntent::Gather);
    CHECK(highEmergency.material==MaterialKind::PlantFood);
    CHECK(lowEmergency.material==MaterialKind::PlantFood);
    CHECK(near(highEmergency.utility,lowEmergency.utility));

    std::cout << "traits/preferences passed\n";
    return 0;
}
