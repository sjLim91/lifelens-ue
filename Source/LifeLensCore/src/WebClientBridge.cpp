#include "lifelens/WebClientBridge.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>

#include "lifelens/ContinuousTerrain.h"
#include "lifelens/Hydrology.h"

namespace lifelens {
namespace {

std::string escapeJson(const std::string& value)
{
    std::ostringstream out;
    for (const unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    out << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c)
                        << std::dec << std::setw(0);
                } else {
                    out << static_cast<char>(c);
                }
                break;
        }
    }
    return out.str();
}

std::uint64_t parseUnsigned64(
    const std::string& text,
    std::uint64_t fallback)
{
    if (text.empty()) return fallback;

    std::uint64_t value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 10);
    if (result.ec != std::errc{} || result.ptr != end) {
        return fallback;
    }
    return value;
}

void appendDouble(std::ostringstream& out, double value)
{
    out << std::fixed << std::setprecision(6) << value;
}

const char* activityKindName(ObservedActivityKind kind)
{
    switch (kind) {
        case ObservedActivityKind::Idle: return "Idle";
        case ObservedActivityKind::Physical: return "Physical";
        case ObservedActivityKind::Social: return "Social";
    }
    return "Idle";
}

} // namespace

WebClientBridge::WebClientBridge() = default;

bool WebClientBridge::newGame(
    const std::string& worldSeedText,
    const std::string& populationSeedText,
    WorldGenerationVersion generationVersion)
{
    worldSeed_ = normalizeWorldSeed(
        parseUnsigned64(worldSeedText, 1));
    populationSeed_ = populationSeedText.empty()
        ? 0
        : static_cast<PopulationSeed>(
            parseUnsigned64(populationSeedText, 0));
    generationVersion_ =
        normalizeWorldGenerationVersion(generationVersion);

    simulation_ = std::make_unique<Simulation>(
        worldSeed_,
        populationSeed_,
        generationVersion_);
    simulation_->setupNewGame();
    return true;
}

void WebClientBridge::runMinutes(int minutes)
{
    if (!simulation_ || minutes <= 0) return;
    simulation_->runMinutes(minutes);
}

std::string WebClientBridge::worldOverviewJson() const
{
    if (!simulation_) return "{\"available\":false}";

    const WorldOverviewObservation overview =
        simulation_->observeWorldOverview();
    const WorldGenesisIdentity identity =
        simulation_->world().genesisIdentity();

    std::ostringstream out;
    out << "{";
    out << "\"available\":true,";
    out << "\"worldSeed\":\"" << identity.worldSeed << "\",";
    out << "\"populationSeed\":\"" << identity.populationSeed << "\",";
    out << "\"generationVersion\":" << identity.generationVersion << ",";
    out << "\"minute\":" << overview.minute << ",";
    out << "\"totalResidents\":" << overview.totalResidents << ",";
    out << "\"livingResidents\":" << overview.livingResidents << ",";
    out << "\"deceasedResidents\":" << overview.deceasedResidents << ",";
    out << "\"households\":" << overview.households << ",";
    out << "\"activeCouples\":" << overview.activeCouples << ",";
    out << "\"activePregnancies\":" << overview.activePregnancies << ",";
    out << "\"majorLifeEvents\":" << overview.majorLifeEvents;
    out << "}";
    return out.str();
}

std::string WebClientBridge::residentsJson() const
{
    if (!simulation_) return "{\"available\":false,\"residents\":[]}";

    const std::vector<ResidentObservation> residents =
        simulation_->observeAllResidents();

    std::ostringstream out;
    out << "{\"available\":true,\"residents\":[";
    bool first = true;
    for (const ResidentObservation& resident : residents) {
        if (!first) out << ",";
        first = false;

        GridPos position{};
        const bool hasPosition =
            simulation_->runtimePosition(resident.id, position);

        out << "{";
        out << "\"id\":\"" << resident.id << "\",";
        out << "\"name\":\"" << escapeJson(resident.name) << "\",";
        out << "\"activityKind\":\""
            << activityKindName(resident.activityKind) << "\",";
        out << "\"activityLabel\":\""
            << escapeJson(resident.activityLabel) << "\",";
        out << "\"emotion\":{";
        out << "\"valence\":"; appendDouble(out, resident.emotionValence); out << ",";
        out << "\"arousal\":"; appendDouble(out, resident.emotionArousal); out << ",";
        out << "\"intensity\":"; appendDouble(out, resident.emotionIntensity);
        out << "},";
        out << "\"needs\":{";
        out << "\"hunger\":"; appendDouble(out, resident.needs.hunger); out << ",";
        out << "\"thirst\":"; appendDouble(out, resident.needs.thirst); out << ",";
        out << "\"sleep\":"; appendDouble(out, resident.needs.sleep); out << ",";
        out << "\"bladder\":"; appendDouble(out, resident.needs.bladder); out << ",";
        out << "\"hygiene\":"; appendDouble(out, resident.needs.hygiene);
        out << "},";
        out << "\"hasPosition\":" << (hasPosition ? "true" : "false");
        if (hasPosition) {
            out << ",\"gridX\":" << position.x;
            out << ",\"gridY\":" << position.y;
        }
        out << "}";
    }
    out << "]}";
    return out.str();
}

std::string WebClientBridge::terrainWindowJson(
    int centerChunkX,
    int centerChunkY,
    int radiusChunks) const
{
    if (!simulation_) return "{\"available\":false,\"chunks\":[]}";

    const WorldGenesisIdentity identity =
        simulation_->world().genesisIdentity();
    const int radius = std::max(0, std::min(radiusChunks, 16));

    std::ostringstream out;
    out << "{\"available\":true,";
    out << "\"centerChunkX\":" << centerChunkX << ",";
    out << "\"centerChunkY\":" << centerChunkY << ",";
    out << "\"radiusChunks\":" << radius << ",";
    out << "\"chunks\":[";

    bool first = true;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            const ChunkCoord coord{
                centerChunkX + x,
                centerChunkY + y};
            const GridPos origin = chunkOriginGrid(coord);
            const GridPos center{
                origin.x + WorldChunkSpanGridCells / 2,
                origin.y + WorldChunkSpanGridCells / 2};

            const ContinuousTerrainSample terrain =
                deriveContinuousTerrainSample(identity, center);
            const HydrologyFacts water =
                deriveHydrologyFacts(identity, coord);

            if (!first) out << ",";
            first = false;
            out << "{";
            out << "\"x\":" << coord.x << ",";
            out << "\"y\":" << coord.y << ",";
            out << "\"elevation01\":"; appendDouble(out, terrain.elevation01); out << ",";
            out << "\"gradientX\":"; appendDouble(out, terrain.gradientXPerGrid); out << ",";
            out << "\"gradientY\":"; appendDouble(out, terrain.gradientYPerGrid); out << ",";
            out << "\"waterKind\":\"" << surfaceWaterKindName(water.surfaceKind) << "\",";
            out << "\"salinity\":\"" << waterSalinityName(water.salinity) << "\",";
            out << "\"waterAvailability\":"; appendDouble(out, water.surfaceAvailability);
            out << "}";
        }
    }

    out << "]}";
    return out.str();
}

} // namespace lifelens
