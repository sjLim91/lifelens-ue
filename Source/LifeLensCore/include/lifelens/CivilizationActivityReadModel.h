#pragma once

#include <cstdint>

#include "Civilization.h"

namespace lifelens {

enum class CivilizationActivityKind {
    None,
    Gather,
    Store,
    Experiment,
    Craft,
    Retrieve
};

inline CivilizationActivityKind civilizationActivityKindFromEvent(CivilizationEventType type)
{
    switch(type){
        case CivilizationEventType::Gathered: return CivilizationActivityKind::Gather;
        case CivilizationEventType::Stored: return CivilizationActivityKind::Store;
        case CivilizationEventType::Retrieved: return CivilizationActivityKind::Retrieve;
        case CivilizationEventType::ExperimentFailed:
        case CivilizationEventType::Discovered: return CivilizationActivityKind::Experiment;
        case CivilizationEventType::Crafted: return CivilizationActivityKind::Craft;
        default: return CivilizationActivityKind::None;
    }
}

inline const char* civilizationActivityKindName(CivilizationActivityKind kind)
{
    switch(kind){
        case CivilizationActivityKind::Gather: return "Gather";
        case CivilizationActivityKind::Store: return "Store";
        case CivilizationActivityKind::Retrieve: return "Retrieve";
        case CivilizationActivityKind::Experiment: return "Experiment";
        case CivilizationActivityKind::Craft: return "Craft";
        case CivilizationActivityKind::None:
        default: return "None";
    }
}

struct ResidentCivilizationActivityObservation {
    CharacterId residentId=0;
    bool active=false;
    CivilizationActivityKind kind=CivilizationActivityKind::None;
    CivilizationEventType eventType=CivilizationEventType::Gathered;
    MaterialKind material=MaterialKind::Unknown;
    ItemKind item=ItemKind::RawMaterial;
    TechniqueId technique=TechniqueId::None;
    int quantity=0;
    int minute=-1;

    // Stable Core identifiers are exposed even though current resource/storage
    // records do not yet own authoritative world-grid positions.
    ResourceNodeId resourceNode=0;
    StorageId storage=0;

    bool success=false;

    // Only contexts with a real Core-authored position may set this true.
    // Current sanitation-site creation/improvement can provide such a target;
    // ordinary Gather/Store/Retrieve must not invent scenery coordinates in Presentation.
    bool hasSpatialTarget=false;
    int targetGridX=0;
    int targetGridY=0;
    std::uint64_t sanitationSiteId=0;
};

} // namespace lifelens
