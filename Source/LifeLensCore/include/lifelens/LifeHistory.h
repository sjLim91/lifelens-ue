#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include "Ids.h"

namespace lifelens {

enum class LifeEventType {
    Birth,
    LifeStageChanged,
    DatingStarted,
    Engaged,
    Married,
    CohabitationStarted,
    PregnancyStarted,
    ChildBorn,
    ParentingMilestone,
    Separated,
    Divorced,
    PartnerWidowed,
    HouseholdChanged,
    Death,
    Bereavement
};

struct LifeHistoryEntry {
    LifeEventType type=LifeEventType::Birth;
    int minute=0;
    std::vector<CharacterId> relatedCharacters;
    int value=0;
};

inline void recordLifeEvent(
    std::vector<LifeHistoryEntry>& history,
    LifeEventType type,
    int minute,
    std::vector<CharacterId> relatedCharacters={},
    int value=0)
{
    LifeHistoryEntry entry;
    entry.type=type;
    entry.minute=std::max(0,minute);
    entry.relatedCharacters=std::move(relatedCharacters);
    entry.value=value;
    history.push_back(std::move(entry));
}

inline std::size_t countLifeEvents(
    const std::vector<LifeHistoryEntry>& history,
    LifeEventType type)
{
    return static_cast<std::size_t>(std::count_if(
        history.begin(),history.end(),
        [type](const LifeHistoryEntry& entry){ return entry.type==type; }));
}

inline const LifeHistoryEntry* latestLifeEvent(
    const std::vector<LifeHistoryEntry>& history,
    LifeEventType type)
{
    for(auto it=history.rbegin();it!=history.rend();++it){
        if(it->type==type) return &(*it);
    }
    return nullptr;
}

inline bool hasLifeEvent(
    const std::vector<LifeHistoryEntry>& history,
    LifeEventType type)
{
    return latestLifeEvent(history,type)!=nullptr;
}

inline const char* lifeEventName(LifeEventType type)
{
    switch(type){
        case LifeEventType::Birth: return "Birth";
        case LifeEventType::LifeStageChanged: return "LifeStageChanged";
        case LifeEventType::DatingStarted: return "DatingStarted";
        case LifeEventType::Engaged: return "Engaged";
        case LifeEventType::Married: return "Married";
        case LifeEventType::CohabitationStarted: return "CohabitationStarted";
        case LifeEventType::PregnancyStarted: return "PregnancyStarted";
        case LifeEventType::ChildBorn: return "ChildBorn";
        case LifeEventType::ParentingMilestone: return "ParentingMilestone";
        case LifeEventType::Separated: return "Separated";
        case LifeEventType::Divorced: return "Divorced";
        case LifeEventType::PartnerWidowed: return "PartnerWidowed";
        case LifeEventType::HouseholdChanged: return "HouseholdChanged";
        case LifeEventType::Death: return "Death";
        case LifeEventType::Bereavement: return "Bereavement";
    }
    return "Unknown";
}

} // namespace lifelens
