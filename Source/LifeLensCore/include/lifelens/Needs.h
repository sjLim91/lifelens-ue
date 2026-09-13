#pragma once
#include <algorithm>
namespace lifelens {
struct NeedsDelta { double hunger=0, thirst=0, sleep=0, bladder=0, hygiene=0; };
struct Needs {
    double hunger=0, thirst=0, sleep=0, bladder=0, hygiene=0;
    static double clamp01(double v) { return std::max(0.0, std::min(1.0, v)); }
    void apply(const NeedsDelta& d) {
        hunger=clamp01(hunger+d.hunger); thirst=clamp01(thirst+d.thirst);
        sleep=clamp01(sleep+d.sleep); bladder=clamp01(bladder+d.bladder); hygiene=clamp01(hygiene+d.hygiene);
    }
    void decay(double metabolism=1.0, double sleepTendency=1.0) {
        apply({0.0010*metabolism,0.0013*metabolism,0.0008*sleepTendency,0.0011*metabolism,0.0007});
    }
};
}
