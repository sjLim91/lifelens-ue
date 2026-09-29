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
    assert(overview.find("\"lifeStages\":{") != std::string::npos);
    assert(overview.find("\"datingCouples\":") != std::string::npos);
    assert(overview.find("\"engagedCouples\":") != std::string::npos);
    assert(overview.find("\"marriedCouples\":") != std::string::npos);
    assert(overview.find("\"separatedCouples\":") != std::string::npos);

    const std::string residents = bridge.residentsJson();
    assert(residents.find("\"residents\":[") != std::string::npos);
    assert(residents.find("\"sex\":\"Male\"") != std::string::npos);
    assert(residents.find("\"sex\":\"Female\"") != std::string::npos);
    assert(residents.find("\"needs\":{") != std::string::npos);
    assert(residents.find("\"bladder\":") != std::string::npos);
    assert(residents.find("\"emotion\":{") != std::string::npos);
    assert(residents.find("\"personality\":{") != std::string::npos);
    assert(residents.find("\"genetics\":{") != std::string::npos);
    assert(residents.find("\"heightPotential\":") != std::string::npos);
    assert(residents.find("\"lifeCondition\":{") != std::string::npos);
    assert(residents.find("\"physicalHealth\":") != std::string::npos);
    assert(residents.find("\"development\":{") != std::string::npos);
    assert(residents.find("\"emotionalSecurity\":") != std::string::npos);
    assert(residents.find("\"traits\":{") != std::string::npos);
    assert(residents.find("\"preferences\":{") != std::string::npos);
    assert(residents.find("\"relationships\":[") != std::string::npos);
    assert(residents.find("\"family\":{") != std::string::npos);
    assert(residents.find("\"hasRomanceHistory\":") != std::string::npos);
    assert(residents.find("\"isGestationalParent\":") != std::string::npos);
    assert(residents.find("\"household\":") != std::string::npos);
    assert(residents.find("\"pregnancy\":") != std::string::npos);
    assert(residents.find("\"memories\":[") != std::string::npos);
    assert(residents.find("\"beliefs\":[") != std::string::npos);
    assert(residents.find("\"lifeStage\":") != std::string::npos);
    assert(residents.find("\"activityTargetName\":") != std::string::npos);
    assert(residents.find("\"presentation\":{") != std::string::npos);
    assert(residents.find("\"phase\":") != std::string::npos);
    assert(residents.find("\"civilizationMaterial\":") != std::string::npos);
    assert(residents.find("\"facilityAction\":") != std::string::npos);
    assert(residents.find("\"civilizationItem\":") != std::string::npos);
    assert(residents.find("\"civilizationTechnique\":") != std::string::npos);
    assert(residents.find("\"civilizationQuantity\":") != std::string::npos);
    assert(residents.find("\"civilizationResourceNode\":") != std::string::npos);
    assert(residents.find("\"civilizationStorage\":") != std::string::npos);
    assert(residents.find("\"facilityId\":") != std::string::npos);
    assert(residents.find("\"facilityKind\":") != std::string::npos);
    assert(residents.find("\"knowledgeTeachingTechnique\":") != std::string::npos);
    assert(residents.find("\"issuedMinute\":") != std::string::npos);
    assert(residents.find("\"civilization\":{") != std::string::npos);
    assert(residents.find("\"inventory\":[") != std::string::npos);
    assert(residents.find("\"techniques\":[") != std::string::npos);
    assert(residents.find("\"lifeHistory\":[") != std::string::npos);
    assert(residents.find("\"hasTargetGrid\":") != std::string::npos);
    assert(residents.find("\"hasObjectTarget\":") != std::string::npos);
    assert(residents.find("\"emergencyFallback\":") != std::string::npos);
    assert(residents.find("\"contextActionToken\":") != std::string::npos);
    assert(residents.find("\"hasPosition\":true") != std::string::npos);

    const std::string environment = bridge.dynamicEnvironmentJson(0, 0);
    assert(environment.find("\"available\":true") != std::string::npos);
    assert(environment.find("\"baselineTemperature01\":") != std::string::npos);
    assert(environment.find("\"baselineMoisture01\":") != std::string::npos);
    assert(environment.find("\"airTemperatureC\":") != std::string::npos);
    assert(environment.find("\"seasonalTemperatureModifierC\":") != std::string::npos);
    assert(environment.find("\"dailyTemperatureModifierC\":") != std::string::npos);
    assert(environment.find("\"precipitationIntensity01\":") != std::string::npos);
    assert(environment.find("\"cloudCover01\":") != std::string::npos);
    assert(environment.find("\"windIntensity01\":") != std::string::npos);
    assert(environment.find("\"visibility01\":") != std::string::npos);
    assert(environment.find("\"summary\":") != std::string::npos);
    assert(environment.find("\"calendar\":{") != std::string::npos);
    assert(environment.find("\"season\":") != std::string::npos);
    assert(environment.find("\"daylight01\":") != std::string::npos);

    const std::string terrain = bridge.terrainWindowJson(0, 0, 1);
    assert(terrain.find("\"available\":true") != std::string::npos);
    assert(terrain.find("\"radiusChunks\":1") != std::string::npos);
    assert(terrain.find("\"elevation01\":") != std::string::npos);
    assert(terrain.find("\"gradientX\":") != std::string::npos);
    assert(terrain.find("\"gradientY\":") != std::string::npos);
    assert(terrain.find("\"salinity\":") != std::string::npos);
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
    assert(terrain.find("\"humanTraces\":{\"total\":0,\"entries\":[]}") != std::string::npos);

    const std::string social = bridge.recentSocialEventsJson(16);
    assert(social.find("\"available\":true") != std::string::npos);
    assert(social.find("\"events\":[") != std::string::npos);

    const std::string civilization = bridge.civilizationWorldJson(16);
    assert(civilization.find("\"available\":true") != std::string::npos);
    assert(civilization.find("\"resources\":[") != std::string::npos);
    assert(civilization.find("\"gridX\":") != std::string::npos);
    assert(civilization.find("\"hasAccessGrid\":") != std::string::npos);
    assert(civilization.find("\"accessGridX\":") != std::string::npos);
    assert(civilization.find("\"accessGridY\":") != std::string::npos);
    assert(civilization.find("\"storages\":[") != std::string::npos);
    assert(civilization.find("\"facilities\":[") != std::string::npos);
    assert(civilization.find("\"recentDiscoveries\":[") != std::string::npos);

    const std::string worldObjects = bridge.worldObjectsJson();
    assert(worldObjects.find("\"available\":true") != std::string::npos);
    assert(worldObjects.find("\"smartObjects\":[") != std::string::npos);
    assert(worldObjects.find("\"sanitationSites\":[") != std::string::npos);

    const std::string before = bridge.worldOverviewJson();
    bridge.runMinutes(5);
    const std::string after = bridge.worldOverviewJson();
    assert(before != after);

    return 0;
}
