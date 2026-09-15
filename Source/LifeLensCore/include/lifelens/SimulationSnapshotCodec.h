#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SimulationSnapshot.h"

namespace lifelens {

constexpr std::uint32_t SimulationSnapshotBinaryFormatVersion=5;
constexpr std::uint32_t MinimumSupportedSimulationSnapshotBinaryFormatVersion=1;

bool encodeSimulationSnapshot(
    const SimulationStateSnapshot& snapshot,
    std::vector<std::uint8_t>& outBytes,
    std::string* error=nullptr);

bool decodeSimulationSnapshot(
    const std::vector<std::uint8_t>& bytes,
    SimulationStateSnapshot& outSnapshot,
    std::string* error=nullptr);

} // namespace lifelens
