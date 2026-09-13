#include <cassert>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    Simulation sim(777);
    sim.setupSocialDemo();

    assert(sim.world().characters.size() == 2);
    assert(sim.relationships().find(1, 2) != nullptr);
    assert(sim.relationships().find(2, 1) != nullptr);

    const std::size_t logsBefore = sim.logs().size();
    sim.runMinutes(6);

    assert(sim.logs().size() > logsBefore);

    bool sawSocialDecision = false;
    for (const auto& line : sim.logs()) {
        if (line.find("Approach") != std::string::npos ||
            line.find("Comfort") != std::string::npos ||
            line.find("Repair") != std::string::npos ||
            line.find("Avoid") != std::string::npos) {
            sawSocialDecision = true;
            break;
        }
    }
    assert(sawSocialDecision);

    const Character& a = sim.world().characters[0];
    const Character& b = sim.world().characters[1];
    assert(!a.memory.entries.empty() || !b.memory.entries.empty());

    const Relationship* aToB = sim.relationships().find(1, 2);
    const Relationship* bToA = sim.relationships().find(2, 1);
    assert(aToB != nullptr && bToA != nullptr);
    assert(aToB->familiarity >= 0.88 || bToA->familiarity >= 0.82);

    // Same seed/setup must remain deterministic.
    Simulation sim2(777);
    sim2.setupSocialDemo();
    sim2.runMinutes(6);
    assert(sim.logs() == sim2.logs());

    return 0;
}
