#include "lifelens/WebClientBridge.h"

#include <cassert>
#include <string>

using namespace lifelens;

int main()
{
    WebClientBridge bridge;
    assert(!bridge.hasSimulation());
    assert(bridge.worldOverviewJson().find("\"available\":false")
        != std::string::npos);

    assert(bridge.newGame("1234567890123456789", "", CurrentWorldGenerationVersion));
    assert(bridge.hasSimulation());

    const std::string overview = bridge.worldOverviewJson();
    assert(overview.find("\"available\":true") != std::string::npos);
    assert(overview.find("\"worldSeed\":\"1234567890123456789\"")
        != std::string::npos);
    assert(overview.find("\"totalResidents\":4") != std::string::npos);

    const std::string residents = bridge.residentsJson();
    assert(residents.find("\"residents\":[") != std::string::npos);
    assert(residents.find("\"needs\":{") != std::string::npos);
    assert(residents.find("\"hasPosition\":true") != std::string::npos);

    const std::string terrain = bridge.terrainWindowJson(0, 0, 1);
    assert(terrain.find("\"available\":true") != std::string::npos);
    assert(terrain.find("\"radiusChunks\":1") != std::string::npos);
    assert(terrain.find("\"elevation01\":") != std::string::npos);
    assert(terrain.find("\"waterKind\":") != std::string::npos);

    const std::string before = bridge.worldOverviewJson();
    bridge.runMinutes(5);
    const std::string after = bridge.worldOverviewJson();
    assert(before != after);

    return 0;
}
