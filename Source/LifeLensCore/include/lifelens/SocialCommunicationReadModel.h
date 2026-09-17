#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "SocialCognition.h"

namespace lifelens {

enum class SocialPresentationLevel {
    Everyday,
    Meaningful,
    Important
};

struct SocialCommunicationObservation {
    std::uint64_t sequence = 0;
    CharacterId actor = 0;
    CharacterId target = 0;
    SocialEventType type = SocialEventType::PositiveInteraction;
    double intensity = 0.0;
    double importance = 0.0;
    int minute = 0;
    std::string where;
    SocialPresentationLevel presentationLevel = SocialPresentationLevel::Everyday;
    bool successful = true;
};

inline SocialPresentationLevel socialPresentationLevel(SocialEventType type)
{
    switch (type) {
        case SocialEventType::PositiveInteraction:
            return SocialPresentationLevel::Everyday;

        case SocialEventType::Help:
        case SocialEventType::Comfort:
        case SocialEventType::Apology:
            return SocialPresentationLevel::Meaningful;

        case SocialEventType::Conflict:
        case SocialEventType::Betrayal:
        case SocialEventType::Rejection:
        case SocialEventType::Intimacy:
        case SocialEventType::Commitment:
            return SocialPresentationLevel::Important;
    }
    return SocialPresentationLevel::Everyday;
}

inline SocialCommunicationObservation makeSocialCommunicationObservation(
    const SocialEvent& event,
    std::uint64_t sequence)
{
    SocialCommunicationObservation observation;
    observation.sequence = sequence;
    observation.actor = event.actor;
    observation.target = event.recipient;
    observation.type = event.type;
    observation.intensity = clampSocial01(event.intensity);
    observation.importance = clampSocial01(socialEventImportance(event.type));
    observation.minute = std::max(0, event.minute);
    observation.where = event.where;
    observation.presentationLevel = socialPresentationLevel(event.type);
    observation.successful = true;
    return observation;
}

inline std::vector<SocialCommunicationObservation> recentSocialCommunicationTail(
    const std::vector<SocialCommunicationObservation>& events,
    std::size_t maxEvents)
{
    if (maxEvents == 0 || events.empty()) {
        return {};
    }

    const std::size_t count = std::min(maxEvents, events.size());
    return std::vector<SocialCommunicationObservation>(
        events.end() - static_cast<std::ptrdiff_t>(count),
        events.end());
}

} // namespace lifelens
