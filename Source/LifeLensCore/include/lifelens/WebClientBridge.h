#include <map>
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "Simulation.h"
#include "EarthRegionScenario.h"

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

    // Explicit opt-in: anchors the *existing* local Core simulation to a
    // natural-Earth location for observer navigation. Its terrain/climate is
    // still procedural; it does NOT simulate geographically conditioned land.
    bool newEarthGame(
        const std::string& worldSeedText,
        double latitudeDeg,
        double longitudeDeg,
        WorldGenerationVersion generationVersion);
    std::string earthScenarioJson() const;
    std::string earthSolarJson() const;

    bool hasSimulation() const { return simulation_ != nullptr; }

    void runMinutes(int minutes);

    // Canonical full Core snapshot, encoded as portable base64 for the WASM
    // string boundary. Never approximate life state from observer JSON.
    std::string saveSnapshotBase64() const;
    bool restoreSnapshotBase64(const std::string& encoded);

    std::string worldOverviewJson() const;
    std::string residentRuntimeJson() const;
    std::string residentsJson() const;
    std::string cognitiveRequestJson(
        const std::string& residentIdText,
        const std::string& triggerText) const;
    // Accept one free-local AI suggestion; Core reobserves and validates its
    // current context. This does not authorize any direct world mutation.
    bool submitLocalCognitiveProposal(
        const std::string& residentIdText,
        int observedMinute,
        const std::string& observedWorldSeed,
        const std::string& intentText,
        double priority,
        const std::string& targetResidentText,
        const std::string& rationaleDigest,
        const std::string& observedTriggerText = "Reflection",
        const std::string& observedFingerprintText = "");
    // Observational only. These locally composed goals never execute or
    // authorize behavior; all data derives from bounded Core cognition.
    std::string autonomousGoalCandidatesJson(
        const std::string& residentIdText,
        const std::string& triggerText) const;
    std::string dynamicEnvironmentJson(
        int centerChunkX,
        int centerChunkY) const;
    std::string recentSocialEventsJson(std::size_t maxEvents = 32) const;
    std::string civilizationWorldJson(
        std::size_t maxRecentDiscoveries = 32) const;
    std::string civilizationWorldWindowJson(
        std::size_t maxRecentDiscoveries,
        int centerChunkX,
        int centerChunkY,
        int radiusChunks) const;
    std::string worldObjectsJson() const;
    std::string humanTracesWindowJson(
        int centerChunkX,
        int centerChunkY,
        int radiusChunks) const;
    std::string terrainWindowJson(
        int centerChunkX,
        int centerChunkY,
        int radiusChunks) const;

private:
    // Bounded immutable terrain samples; never a simulation state cache.
    mutable std::map<std::string,std::string> elevationSampleCache_;
    std::unique_ptr<Simulation> simulation_;
    std::unique_ptr<EarthScenarioGeoCatalog> earthScenario_;
    SurfaceRegionId activeEarthOriginRegionId_=0;
    WorldSeed worldSeed_ = 1;
    PopulationSeed populationSeed_ = 0;
    WorldGenerationVersion generationVersion_ =
        CurrentWorldGenerationVersion;
};

} // namespace lifelens

