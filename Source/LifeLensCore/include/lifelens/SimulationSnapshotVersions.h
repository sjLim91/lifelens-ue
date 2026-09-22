#pragma once

#include <cstdint>

namespace lifelens {

// Single source of truth for the pre-release Core snapshot contract.
// Consumers and validators must reference these symbols instead of repeating
// numeric version literals.
constexpr std::uint32_t SimulationSnapshotVersion=3;
constexpr std::uint32_t SimulationSnapshotBinaryFormatVersion=8;

} // namespace lifelens
