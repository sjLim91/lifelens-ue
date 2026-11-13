#pragma once

#include <cstdint>

#include "Ids.h"

namespace lifelens {

// Optional per-issued-Civilization-token provenance. No model authority,
// inventing a goal or trusting an action log: the accepted thought is still
// validated against the Core journal before this link may be attached.
struct ResidentCognitiveActionLink {
    std::uint64_t token=0;
    CharacterId actor=0;
    std::uint64_t contextFingerprint=0;
    int thoughtMinute=-1;
    int issuedMinute=-1;

    bool active() const { return token!=0; }
    void clear() { *this=ResidentCognitiveActionLink{}; }
};

inline bool validResidentCognitiveActionLink(
    const ResidentCognitiveActionLink& link)
{
    return link.token!=0 &&link.actor!=0 &&link.contextFingerprint!=0
        &&link.thoughtMinute>=0 &&link.issuedMinute>=link.thoughtMinute
        &&link.issuedMinute-link.thoughtMinute<=1440;
}

} // namespace lifelens
