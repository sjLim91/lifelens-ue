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
        .function("residentsJson", &lifelens::WebClientBridge::residentsJson)
        .function("worldPresentationJson", &lifelens::WebClientBridge::worldPresentationJson)
        .function("dynamicEnvironmentJson", &lifelens::WebClientBridge::dynamicEnvironmentJson)
        .function("terrainWindowJson", &lifelens::WebClientBridge::terrainWindowJson);
}
