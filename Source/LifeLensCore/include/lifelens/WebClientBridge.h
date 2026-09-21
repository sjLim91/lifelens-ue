#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Simulation.h"

namespace lifelens {

/**
 * Thin platform-neutral adapter for browser/WASM and other lightweight clients.
 *
 * It exposes stable JSON snapshots instead of leaking C++ object layout across
 * the ABI boundary. Unreal does not depend on this adapter.
 */
class WebClientBridge {
public:
    WebClientBridge();

    bool newGame(
        const std::string& worldSeedText,
        const std::string& populationSeedText = "",
        WorldGenerationVersion generationVersion =
            CurrentWorldGenerationVersion);

    bool hasSimulation() const { return simulation_ != nullptr; }

    void runMinutes(int minutes);

    std::string worldOverviewJson() const;
    std::string residentsJson() const;
    std::string terrainWindowJson(
        int centerChunkX,
        int centerChunkY,
        int radiusChunks) const;

private:
    std::unique_ptr<Simulation> simulation_;
    WorldSeed worldSeed_ = 1;
    PopulationSeed populationSeed_ = 0;
    WorldGenerationVersion generationVersion_ =
        CurrentWorldGenerationVersion;
};

} // namespace lifelens
