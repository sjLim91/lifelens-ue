#include <cassert>
#include <iostream>
#include <string>

#include "lifelens/WebClientBridge.h"

using namespace lifelens;

static std::string firstResidentId(const std::string& residentsJson)
{
    const std::string marker="\"id\":\"";
    const std::size_t begin=residentsJson.find(marker);
    if(begin==std::string::npos) return {};
    const std::size_t valueBegin=begin+marker.size();
    const std::size_t end=residentsJson.find('"',valueBegin);
    if(end==std::string::npos) return {};
    return residentsJson.substr(valueBegin,end-valueBegin);
}

int main()
{
    WebClientBridge bridge;
    assert(bridge.newGame("8421901","",CurrentWorldGenerationVersion));

    const std::string residents=bridge.residentsJson();
    const std::string actor=firstResidentId(residents);
    assert(!actor.empty());

    const std::string reflection=
        bridge.cognitiveRequestJson(actor,"Reflection");
    assert(reflection!="null");
    assert(reflection.find("\"actor\":\""+actor+"\"")!=std::string::npos);
    assert(reflection.find("\"trigger\":\"Reflection\"")!=std::string::npos);
    assert(reflection.find("\"needs\":{")!=std::string::npos);
    assert(reflection.find("\"personality\":{")!=std::string::npos);
    assert(reflection.find("\"emotion\":{")!=std::string::npos);
    assert(reflection.find("\"memories\":[")!=std::string::npos);
    assert(reflection.find("\"beliefs\":[")!=std::string::npos);
    assert(reflection.find("\"relationships\":[")!=std::string::npos);
    assert(reflection.find("\"allowedIntents\":[")!=std::string::npos);
    assert(reflection.find("\"ImproveFoodSecurity\"")!=std::string::npos);
    assert(reflection.find("\"CooperateWithResident\"")!=std::string::npos);
    assert(reflection.find("\"MigrateHousehold\"")!=std::string::npos);

    const std::string scarcity=
        bridge.cognitiveRequestJson(actor,"ResourceScarcity");
    assert(scarcity.find(
        "\"trigger\":\"ResourceScarcity\"")!=std::string::npos);

    assert(bridge.cognitiveRequestJson(actor,"None")=="null");
    assert(bridge.cognitiveRequestJson(actor,"FutureTrigger")=="null");
    assert(bridge.cognitiveRequestJson("0","Reflection")=="null");
    assert(bridge.cognitiveRequestJson(
        "18446744073709551615","Reflection")=="null");
    assert(bridge.cognitiveRequestJson("not-an-id","Reflection")=="null");

    WebClientBridge empty;
    assert(empty.cognitiveRequestJson(actor,"Reflection")=="null");

    std::cout
        <<"cognitive Web bridge exposes bounded Core-owned read-only context\n";
    return 0;
}
