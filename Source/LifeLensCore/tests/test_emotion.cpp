#include <cassert>
#include <cmath>
#include "lifelens/Emotion.h"

using namespace lifelens;

static bool near(double a,double b,double eps=1e-9){return std::fabs(a-b)<=eps;}

int main()
{
    EmotionState e;
    assert(near(e.valence,0.0));
    assert(near(e.arousal,0.0));
    assert(near(e.intensity(),0.0));

    applyEmotionEvent(e,EmotionEventType::PositiveSocial);
    assert(e.joy>0.0);
    assert(e.affection>0.0);
    assert(e.valence>0.0);

    const double joyBeforeDecay=e.joy;
    const double affectionBeforeDecay=e.affection;
    e.decay(0.25);
    assert(e.joy<joyBeforeDecay);
    assert(e.affection<affectionBeforeDecay);

    EmotionState rejected;
    applyEmotionEvent(rejected,EmotionEventType::Rejection);
    assert(rejected.sadness>0.0);
    assert(rejected.embarrassment>0.0);
    assert(rejected.anxiety>0.0);
    assert(rejected.valence<0.0);

    EmotionState betrayal;
    betrayal.affection=0.80;
    betrayal.refreshSummary();
    const double affectionBeforeBetrayal=betrayal.affection;
    applyEmotionEvent(betrayal,EmotionEventType::Betrayal);
    assert(betrayal.anger>0.0);
    assert(betrayal.jealousy>0.0);
    assert(betrayal.affection<affectionBeforeBetrayal);
    assert(betrayal.arousal>0.0);

    EmotionState threat;
    applyEmotionEvent(threat,EmotionEventType::Threat,2.0);
    assert(threat.fear>0.0);
    assert(threat.anxiety>0.0);
    assert(threat.intensity()<=1.0);

    EmotionState loss;
    applyEmotionEvent(loss,EmotionEventType::Loss);
    assert(loss.grief>loss.sadness);
    assert(loss.valence<0.0);

    EmotionState saturated;
    EmotionDelta d;
    d.joy=5.0;
    d.anger=5.0;
    saturated.apply(d);
    assert(near(saturated.joy,1.0));
    assert(near(saturated.anger,1.0));
    assert(saturated.arousal<=1.0);
    assert(saturated.valence>=-1.0 && saturated.valence<=1.0);

    return 0;
}
