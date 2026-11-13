#pragma once
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "lifelens/WebClientBridge.h"
#include "lifelens/SimulationSnapshotCodec.h"
inline std::string causalFixtureBase64(const std::vector<std::uint8_t>& bytes) {
    static const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for(std::size_t i=0;i<bytes.size();i+=3){
        const auto n=(std::uint32_t(bytes[i])<<16)|(i+1<bytes.size()?std::uint32_t(bytes[i+1])<<8:0)|(i+2<bytes.size()?bytes[i+2]:0);
        out+=alphabet[(n>>18)&63];out+=alphabet[(n>>12)&63];out+=i+1<bytes.size()?alphabet[(n>>6)&63]:'=';out+=i+2<bytes.size()?alphabet[n&63]:'=';
    }
    return out;
}
inline bool exportCausalFixture(const lifelens::Simulation& simulation,lifelens::CharacterId actor,const char* name) {
    const char* directory=std::getenv("LIFELENS_CAUSAL_FIXTURE_DIR");if(!directory)return true;
    std::filesystem::create_directories(directory);
    std::vector<std::uint8_t> bytes;std::string error;
    if(!lifelens::encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error))return false;
    lifelens::WebClientBridge bridge;
    if(!bridge.restoreSnapshotBase64(causalFixtureBase64(bytes)))return false;
    const auto path=std::filesystem::path(directory)/name;
    std::ofstream binary(path.string()+".bin",std::ios::binary);binary.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    std::ofstream json(path.string()+".json");json<<bridge.cognitiveCausalJournalJson(std::to_string(actor));
    std::ofstream residents(path.string()+".residents.json");residents<<bridge.residentsJson();
    return binary.good()&&json.good()&&residents.good();
}
