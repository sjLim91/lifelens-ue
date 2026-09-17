#include <cassert>
#include <cmath>

#include "lifelens/EmotionRuntime.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    Character neutral;
    neutral.needs={0.20,0.20,0.20,0.20,0.20};
    advanceEmotionOneMinute(neutral);
    assert(neutral.emotion.intensity()==0.0);

    Character pressured;
    pressured.personality.emotionalStability=0.30;
    pressured.needs={0.92,0.20,0.20,0.20,0.20};
    for(int i=0;i<180;++i) advanceEmotionOneMinute(pressured);
    assert(pressured.emotion.anxiety>0.04);
    assert(pressured.emotion.fear>0.0);

    const EmotionState beforeRelief=pressured.emotion;
    const Needs beforeNeeds=pressured.needs;
    pressured.needs.hunger=0.25;
    applyNeedResolutionEmotion(pressured,beforeNeeds,Goal::Eat);
    assert(pressured.emotion.relief>beforeRelief.relief);
    assert(pressured.emotion.anxiety<beforeRelief.anxiety);

    Character failing;
    failing.personality.impulsiveness=0.80;
    applyActionFailureEmotion(failing,1);
    const double firstAnger=failing.emotion.anger;
    applyActionFailureEmotion(failing,3);
    assert(failing.emotion.anger>firstAnger);
    assert(failing.emotion.anxiety>0.0);

    Character crafter;
    crafter.personality.ambition=0.90;
    applyCivilizationOutcomeEmotion(crafter,true,true);
    assert(crafter.emotion.pride>0.0);
    assert(crafter.emotion.joy>0.0);

    Character biased;
    biased.needs.hunger=0.70;
    World world(7);
    world.objects.push_back({1,ObjectKind::Fridge,{0,0},std::nullopt,{-0.1,0,0,0,0},5});
    const double baseline=scoreGoal(world,biased,Goal::Eat);
    biased.emotion.anxiety=1.0;
    biased.emotion.fear=1.0;
    biased.emotion.refreshSummary();
    const double distressed=scoreGoal(world,biased,Goal::Eat);
    assert(distressed>baseline);
    assert(distressed<=baseline*1.08+1e-12);

    Simulation simulation(991);
    simulation.setupDemo();
    simulation.world().objects.clear();
    simulation.world().resourceNodes.clear();
    simulation.world().storageSites.clear();
    Character& resident=simulation.world().characters.front();
    resident.needs.hunger=0.95;
    resident.civilization.inventory=Inventory{};
    simulation.runMinutes(90);
    assert(resident.emotion.anxiety>0.0);

    const SimulationStateSnapshot snapshot=simulation.captureSnapshot();
    Simulation restored(1);
    assert(restored.restoreSnapshot(snapshot));
    assert(restored.world().characters.front().emotion.anxiety==resident.emotion.anxiety);

    simulation.runMinutes(30);
    restored.runMinutes(30);
    assert(restored.world().characters.front().emotion.anxiety==
           simulation.world().characters.front().emotion.anxiety);

    return 0;
}
