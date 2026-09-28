#pragma once

#include <cmath>
#include <string>
#include <unordered_map>
#include "World.h"

namespace lifelens {

enum class HumanTraceKind { ResourceUse, Residue, Facility };

// A read-only projection of persistent world state, never an activity guess.
struct HumanTraceObservation {
    std::string id;
    HumanTraceKind kind = HumanTraceKind::ResourceUse;
    GridPos pos{};
    CharacterId sourceCharacter = 0;
    MaterialKind material = MaterialKind::Unknown;
    int quantity = 0;
    int baselineQuantity = 0;
    bool renewable = false;
    double amount = 0.0;
    double intensity = 0.0;
    int radiusTiles = 0;
    FacilityKind facilityKind = FacilityKind::PrimitiveStorage;
    FacilityState facilityState = FacilityState::Planned;
    double progress01 = 0.0;
    int deliveredMaterialUnits = 0;
    int requiredMaterialUnits = 0;
    bool active = false;
    bool lit = false;
    bool cropPlanted = false;
    double cropGrowth01 = 0.0;
    double cropMoisture01 = 0.0;
    double cropCare01 = 0.0;
    int cropHarvestUnits = 0;
};

struct HumanTraceWindowObservation {
    std::size_t total = 0;
    std::vector<HumanTraceObservation> entries;
};

inline HumanTraceWindowObservation buildHumanTraceWindowObservation(
    const World& world, ChunkCoord center, int radiusChunks)
{
    HumanTraceWindowObservation result;
    const int radius = std::clamp(radiusChunks, 0, 16);
    const auto inWindow = [&](GridPos pos) {
        // Floating division/floor also handles negative coordinates correctly.
        const double x = std::floor(double(pos.x) / WorldChunkSpanGridCells);
        const double y = std::floor(double(pos.y) / WorldChunkSpanGridCells);
        return std::abs(x - center.x) <= radius && std::abs(y - center.y) <= radius;
    };

    // Compare only materialized natural patches with their own initial state.
    // Merely having quantity < maxQuantity is not evidence of human use.
    std::unordered_map<ResourceNodeId, const ResourceNode*> nodes;
    for (const auto& node : world.resourceNodes) {
        if (inWindow(node.pos)) nodes.emplace(node.id, &node);
    }
    for (const auto& chunk : world.generatedNaturalChunks) {
        for (const auto& patch : chunk.resourcePatches) {
            const auto found = nodes.find(patch.nodeId);
            if (found == nodes.end() || patch.baselineQuantity <= 0) continue;
            const auto& node = *found->second;
            if (node.material != patch.material || node.pos.x != patch.pos.x
                || node.pos.y != patch.pos.y || node.quantity < 0
                || node.quantity >= patch.baselineQuantity) continue;
            // Drawing a bare patch where water was collected would invent land.
            if (node.material == MaterialKind::Water) continue;
            HumanTraceObservation trace;
            trace.id = "resource:" + std::to_string(node.id);
            trace.pos = node.pos;
            trace.material = node.material;
            trace.quantity = node.quantity;
            trace.baselineQuantity = patch.baselineQuantity;
            trace.renewable = node.renewable;
            result.entries.push_back(trace);
        }
    }
    for (const auto& residue : world.environmentalResidues.all()) {
        if (!inWindow(residue.pos) || residue.kind != EnvironmentalResidueKind::HumanWaste
            || residue.amount <= 0.01 || residue.intensity <= 0.01) continue;
        HumanTraceObservation trace;
        trace.id = "residue:" + std::to_string(residue.id);
        trace.kind = HumanTraceKind::Residue;
        trace.pos = residue.pos;
        trace.sourceCharacter = residue.sourceCharacter;
        trace.amount = residue.amount;
        trace.intensity = residue.intensity;
        trace.radiusTiles = residue.radiusTiles;
        result.entries.push_back(trace);
    }
    for (const auto& facility : world.facilities) {
        if (!inWindow(facility.pos) || facility.id == 0) continue;
        int delivered = 0, required = 0;
        for (const auto& requirement : facility.requirements) {
            delivered += requirement.delivered;
            required += requirement.required;
        }
        // An unstarted plan has not left a physical trace.
        if (delivered <= 0 && facility.constructionWork <= 0.0) continue;
        HumanTraceObservation trace;
        trace.id = "facility:" + std::to_string(facility.id);
        trace.kind = HumanTraceKind::Facility;
        trace.pos = facility.pos;
        trace.sourceCharacter = facility.lastWorkedBy;
        trace.facilityKind = facility.kind;
        trace.facilityState = facility.state;
        trace.progress01 = facility.requiredWork > 0.0
            ? std::clamp(facility.constructionWork / facility.requiredWork, 0.0, 1.0) : 0.0;
        trace.deliveredMaterialUnits = delivered;
        trace.requiredMaterialUnits = required;
        trace.active = facilityOperationalAndActive(facility);
        trace.lit = trace.active && facility.lit;
        trace.cropPlanted = facility.cropPlanted;
        trace.cropGrowth01 = facility.cropGrowth01;
        trace.cropMoisture01 = facility.cropMoisture01;
        trace.cropCare01 = facility.cropCare01;
        trace.cropHarvestUnits = facility.cropHarvestUnits;
        result.entries.push_back(trace);
    }

    // Bound the Web payload after filtering, so distant state cannot hide local
    // traces. Stable nearest-first ordering prevents markers shuffling on ticks.
    const double cx = (double(center.x) + 0.5) * WorldChunkSpanGridCells;
    const double cy = (double(center.y) + 0.5) * WorldChunkSpanGridCells;
    const auto distance = [&](const HumanTraceObservation& trace) {
        const double dx = trace.pos.x - cx, dy = trace.pos.y - cy;
        return dx * dx + dy * dy;
    };
    std::sort(result.entries.begin(), result.entries.end(), [&](const auto& a, const auto& b) {
        const double da = distance(a), db = distance(b);
        return da == db ? a.id < b.id : da < db;
    });
    result.total = result.entries.size();
    if (result.entries.size() > 64) result.entries.resize(64);
    return result;
}

} // namespace lifelens
