#pragma once
#include <cstdint>
#include <random>
#include <vector>
#include "Character.h"
#include "SmartObject.h"
#include "Civilization.h"
namespace lifelens {
struct World {
    int minute=7*60;
    std::uint64_t seed=1;
    std::mt19937_64 rng{1};
    std::vector<Character> characters;
    std::vector<SmartObject> objects;
    std::vector<ResourceNode> resourceNodes;
    std::vector<StorageSite> storageSites;
    explicit World(std::uint64_t s=1) : seed(s?s:1), rng(seed) {}
};
}
