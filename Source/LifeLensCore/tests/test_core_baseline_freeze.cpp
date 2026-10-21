#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SimulationSnapshotVersions.h"
#include "lifelens/SimulationRuleset.h"

using namespace lifelens;

// M0-05: changing a persistence/ruleset generation is a reviewed
// compatibility event, not an incidental effect of unrelated feature work.
static_assert(SimulationSnapshotVersion==3,
    "Core state snapshot version changed: review baseline and migration");
static_assert(SimulationSnapshotBinaryFormatVersion==8,
    "Core binary version changed: review baseline and old saves");
static_assert(CurrentSimulationRulesetVersion==1,
    "Ruleset version changed: review replay and migration contract");

static std::vector<std::uint8_t> bytesOf(const Simulation& simulation)
{
    std::vector<std::uint8_t> out;
    std::string error;
    assert(encodeSimulationSnapshot(
        simulation.captureSnapshot(),out,&error));
    assert(error.empty() &&!out.empty());
    return out;
}

int main()
{
    assert(validSimulationRuleset(DefaultSimulationRuleset));

    // Independent same-seed worlds must replay IDENTICAL full Core binary
    // truth through a day of wildlife, physical needs and social decisions.
    // A successful archive roundtrip must continue with the same state too.
    constexpr std::array<std::uint64_t,3> seeds={{
        2,42,874213955
    }};
    for(const auto seed:seeds){
        Simulation first(seed,0,4),identical(seed,0,4);
        first.setupNewGame();
        identical.setupNewGame();
        assert(first.world().seed==seed &&identical.world().seed==seed);
        assert(sameSimulationRuleset(
            first.captureSnapshot().ruleset,DefaultSimulationRuleset));
        first.runMinutes(1440);
        identical.runMinutes(1440);
        const auto saved=bytesOf(first);
        assert(saved==bytesOf(identical));

        SimulationStateSnapshot decoded;
        std::string error;
        assert(decodeSimulationSnapshot(saved,decoded,&error));
        assert(error.empty());
        assert(decoded.version==SimulationSnapshotVersion);
        assert(sameSimulationRuleset(decoded.ruleset,DefaultSimulationRuleset));

        Simulation restored(seed,0,4);
        assert(restored.restoreSnapshot(decoded,&error));
        assert(error.empty());
        // Snapshot save/restore cannot alter the authoritative state.
        assert(bytesOf(restored)==saved);
        first.runMinutes(240);
        identical.runMinutes(240);
        restored.runMinutes(240);
        assert(bytesOf(first)==bytesOf(identical));
        assert(bytesOf(first)==bytesOf(restored));
    }
    return 0;
}
