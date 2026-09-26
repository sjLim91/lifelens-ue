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
    assert(residents.find("\"sex\":\"Male\"") != std::string::npos);
    assert(residents.find("\"sex\":\"Female\"") != std::string::npos);
    assert(residents.find("\"needs\":{") != std::string::npos);
    assert(residents.find("\"bladder\":") != std::string::npos);
    assert(residents.find("\"emotion\":{") != std::string::npos);
    assert(residents.find("\"personality\":{") != std::string::npos);
    assert(residents.find("\"traits\":{") != std::string::npos);
    assert(residents.find("\"preferences\":{") != std::string::npos);
    assert(residents.find("\"relationships\":[") != std::string::npos);
    assert(residents.find("\"family\":{") != std::string::npos);
    assert(residents.find("\"memories\":[") != std::string::npos);
    assert(residents.find("\"beliefs\":[") != std::string::npos);
    assert(residents.find("\"lifeStage\":") != std::string::npos);
    assert(residents.find("\"activityTargetName\":") != std::string::npos);
    assert(residents.find("\"actionContext\":{") != std::string::npos);
    assert(residents.find("\"hasSpatialTarget\":") != std::string::npos);
    assert(residents.find("\"targetResidentId\":") != std::string::npos);
    assert(residents.find("\"hasPosition\":true") != std::string::npos);

    const std::string environment = bridge.dynamicEnvironmentJson(0, 0);
    assert(environment.find("\"available\":true") != std::string::npos);
    assert(environment.find("\"airTemperatureC\":") != std::string::npos);
    assert(environment.find("\"precipitationIntensity01\":") != std::string::npos);
    assert(environment.find("\"cloudCover01\":") != std::string::npos);
    assert(environment.find("\"windIntensity01\":") != std::string::npos);
    assert(environment.find("\"visibility01\":") != std::string::npos);
    assert(environment.find("\"summary\":") != std::string::npos);

    const std::string terrain = bridge.terrainWindowJson(0, 0, 1);
    assert(terrain.find("\"available\":true") != std::string::npos);
    assert(terrain.find("\"radiusChunks\":1") != std::string::npos);
    assert(terrain.find("\"elevation01\":") != std::string::npos);
    assert(terrain.find("\"waterKind\":") != std::string::npos);
    assert(terrain.find("\"waterAvailability\":") != std::string::npos);
    assert(terrain.find("\"flowPotential\":") != std::string::npos);
    assert(terrain.find("\"drainageAccumulationPotential\":") != std::string::npos);
    assert(terrain.find("\"hasDownstream\":") != std::string::npos);
    assert(terrain.find("\"downstreamChunkX\":") != std::string::npos);
    assert(terrain.find("\"downstreamChunkY\":") != std::string::npos);
    assert(terrain.find("\"worldSeed\":") != std::string::npos);
    assert(terrain.find("\"biome\":") != std::string::npos);
    assert(terrain.find("\"forestCoverage01\":") != std::string::npos);
    assert(terrain.find("\"grassCoverage01\":") != std::string::npos);
    assert(terrain.find("\"shrubCoverage01\":") != std::string::npos);
    assert(terrain.find("\"rockCoverage01\":") != std::string::npos);
    assert(terrain.find("\"wetlandCoverage01\":") != std::string::npos);

    const std::string before = bridge.worldOverviewJson();
    bridge.runMinutes(5);
    const std::string after = bridge.worldOverviewJson();
    assert(before != after);

    return 0;
}
