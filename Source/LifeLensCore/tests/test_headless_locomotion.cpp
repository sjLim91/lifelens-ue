#include <cassert>
#include <iostream>

#include "lifelens/CoreNavigation.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

static bool same(GridPos a, GridPos b)
{
    return a.x == b.x && a.y == b.y;
}

static void assertNoTeleportStep(GridPos before, GridPos after)
{
    assert(manhattan(before, after) <= 1);
}

int main()
{
    {
        // The autonomous Core path used by Web must advance through real
        // authoritative cells instead of holding position and snapping to the
        // SmartObject on the final travel tick.
        Simulation simulation(77, 0, 1);
        simulation.setupDemo();

        Character& resident = simulation.world().characters.front();
        resident.needs.hunger = 0.0;
        resident.needs.thirst = 0.0;
        resident.needs.sleep = 1.0;
        resident.needs.bladder = 0.0;
        resident.needs.hygiene = 0.0;

        GridPos previous{};
        assert(simulation.runtimePosition(resident.id, previous));
        const GridPos start = previous;
        const GridPos bed = simulation.world().objects.front().pos;

        bool sawIntermediate = false;
        bool reachedBed = same(start, bed);

        for(int minute = 0; minute < 40 && !reachedBed; ++minute){
            simulation.step();

            GridPos current{};
            assert(simulation.runtimePosition(resident.id, current));
            assertNoTeleportStep(previous, current);

            if(!same(current, start) && !same(current, bed)){
                sawIntermediate = true;
            }
            reachedBed = same(current, bed);
            previous = current;
        }

        assert(reachedBed);
        assert(sawIntermediate || manhattan(start, bed) <= 1);
    }

    {
        // Emergency sanitation used to set Runtime::pos directly to the
        // deterministic outdoor relief target. It must now walk there.
        Simulation simulation(91, 0, 1);
        simulation.setupDemo();
        simulation.world().objects.clear();

        Character& resident = simulation.world().characters.front();
        resident.needs.hunger = 0.0;
        resident.needs.thirst = 0.0;
        resident.needs.sleep = 0.0;
        resident.needs.bladder = 1.0;
        resident.needs.hygiene = 0.0;

        GridPos previous{};
        assert(simulation.runtimePosition(resident.id, previous));

        for(int minute = 0; minute < 35; ++minute){
            simulation.step();

            GridPos current{};
            assert(simulation.runtimePosition(resident.id, current));
            assertNoTeleportStep(previous, current);
            previous = current;
        }
    }

    {
        // Route cells themselves must be continuous and traversable according
        // to Core world truth.
        World world(1234, 0, 1);
        std::vector<GridPos> route;
        const GridPos start{0, 0};
        GridPos target{6, 4};

        // Find a nearby deterministic target if the first one happens to sit on
        // a legacy hydrology footprint for this seed.
        if(!coreGroundTraversable(world, start)){
            std::cout << "start cell blocked for navigation fixture; skipping route fixture\n";
        }else{
            bool built = buildCoreGroundRoute(world, start, target, 0, route);
            if(!built){
                target = {4, -5};
                built = buildCoreGroundRoute(world, start, target, 0, route);
            }
            assert(built);

            GridPos previous = start;
            for(const GridPos step : route){
                assertNoTeleportStep(previous, step);
                assert(coreGroundTraversable(world, step));
                previous = step;
            }
            assert(same(previous, target));
        }
    }

    std::cout << "LifeLens authoritative headless locomotion: PASS\n";
    return 0;
}
