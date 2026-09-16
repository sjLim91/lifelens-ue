#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SimulationSnapshot.h"

namespace lifelens {

// Pre-release project: only the current on-disk format is supported.
// There are no shipped user saves to migrate.
constexpr std::uint32_t SimulationSnapshotBinaryFormatVersion=7;

bool encodeSimulationSnapshot(
    const SimulationStateSnapshot& snapshot,
    std::vector<std::uint8_t>& outBytes,
    std::string* error=nullptr);

bool decodeSimulationSnapshot(
    const std::vector<std::uint8_t>& bytes,
    SimulationStateSnapshot& outSnapshot,
    std::string* error=nullptr);

} // namespace lifelens
