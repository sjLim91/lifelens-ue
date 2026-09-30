#include <emscripten/bind.h>

#include "lifelens/WebClientBridge.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(lifelens_web_core)
{
    class_<lifelens::WebClientBridge>("LifeLensWebClient")
        .constructor<>()
        .function("newGame", &lifelens::WebClientBridge::newGame)
        .function("hasSimulation", &lifelens::WebClientBridge::hasSimulation)
        .function("runMinutes", &lifelens::WebClientBridge::runMinutes)
        .function("worldOverviewJson", &lifelens::WebClientBridge::worldOverviewJson)
        .function("residentRuntimeJson", &lifelens::WebClientBridge::residentRuntimeJson)
        .function("residentsJson", &lifelens::WebClientBridge::residentsJson)
        .function("dynamicEnvironmentJson", &lifelens::WebClientBridge::dynamicEnvironmentJson)
        .function("recentSocialEventsJson", &lifelens::WebClientBridge::recentSocialEventsJson)
        .function("civilizationWorldJson", &lifelens::WebClientBridge::civilizationWorldJson)
        .function("civilizationWorldWindowJson", &lifelens::WebClientBridge::civilizationWorldWindowJson)
        .function("worldObjectsJson", &lifelens::WebClientBridge::worldObjectsJson)
        .function("humanTracesWindowJson", &lifelens::WebClientBridge::humanTracesWindowJson)
        .function("terrainWindowJson", &lifelens::WebClientBridge::terrainWindowJson);
}
