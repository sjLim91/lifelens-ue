#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
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

    std::cout << "traits/preferences passed\n";
    return 0;
}
