#pragma once
#include <cstdint>
#include <random>
#include <vector>
#include "Character.h"
#include "SmartObject.h"
#include "Civilization.h"
#include "EnvironmentalResidue.h"
#include "PrimitiveSanitation.h"
namespace lifelens {
struct World {
    int minute=7*60;
    std::uint64_t seed=1;
    std::mt19937_64 rng{1};
    std::vector<Character> characters;
    std::vector<SmartObject> objects;
    std::vector<ResourceNode> resourceNodes;
    std::vector<StorageSite> storageSites;
    std::vector<PrimitiveSanitationSite> primitiveSanitationSites;
    EnvironmentalResidueField environmentalResidues;

    // Runtime execution policy only. The binary snapshot codec deliberately
    // does not persist this flag; Unreal re-enables external execution after
    // starting/restoring Core while standalone Core tests remain autonomous.
    bool externalPhysicalExecution=false;

    explicit World(std::uint64_t s=1) : seed(s?s:1), rng(seed)
    {
        resetCivilizationEnvironment();
    }

    void resetCivilizationEnvironment()
    {
        // Natural starting environment. These are material opportunities, not
        // pre-unlocked techniques. Characters still begin with no recipe/tech
        // knowledge and must discover reproducible methods themselves.
        resourceNodes={
            {1,MaterialKind::Stone,160,160,false,0},
            {2,MaterialKind::Flint,90,90,false,0},
            {3,MaterialKind::Wood,140,180,true,8},
            {4,MaterialKind::Fiber,100,140,true,7},
            {5,MaterialKind::Clay,120,120,false,0},
            {6,MaterialKind::Water,240,300,true,30},
            {7,MaterialKind::PlantFood,80,120,true,10}
        };
        storageSites={{1,Inventory{}}};
        primitiveSanitationSites.clear();
        environmentalResidues.clear();
    }
};
}
