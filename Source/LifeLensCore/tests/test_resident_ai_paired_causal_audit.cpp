#include <cassert>
#include <string>

#include "lifelens/ResidentAIPairedCausalAudit.h"

using namespace lifelens;

int main()
{
    const auto first=runResidentAIPairedAudit(4242001,1);
    assert(first.seed==4242001);
    assert(first.days==1);
    assert(first.matchedInitialResidents);
    assert(first.baseline.proposalsAttempted==0);
    assert(first.baseline.proposalsAccepted==0);
    assert(first.baseline.proposalsRejected==0);
    assert(first.proxy.proposalsAttempted>0);
    assert(first.proxy.proposalsAccepted>0);
    assert(first.proxy.proposalsAttempted==
           first.proxy.proposalsAccepted+first.proxy.proposalsRejected);
    assert(first.baseline.endMinute==first.proxy.endMinute);
    assert(first.baseline.livingResidents<=4);
    assert(first.proxy.livingResidents<=4);
    const std::string output=residentAIPairedReportJson(first);
    assert(output.find("fixed-deterministic-proxy-not-llm")!=std::string::npos);
    assert(output.find("\"matchedInitialResidents\":true")!=std::string::npos);
    assert(output.find("\"civilizationSuccessEvidence\":")!=std::string::npos);
    // Repeating the identical paired scenario reproduces every numeric
    // observation, including actual Core evidence and pending starts.
    const auto same=runResidentAIPairedAudit(4242001,1);
    assert(residentAIPairedReportJson(same)==output);

    // A different initial world has its own identity and independently
    // produces valid scripted inputs without affecting the baseline branch.
    const auto another=runResidentAIPairedAudit(874213954,1);
    assert(another.matchedInitialResidents);
    assert(another.baseline.proposalsAccepted==0);
    assert(another.proxy.proposalsAccepted>0);
    assert(another.seed!=first.seed);

    // The additional intervention tests ONLY the four newly connected
    // cognition intents. Accepted intents are not completed actions.
    const auto focused=runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focused.matchedInitialResidents);
    assert(focused.policy==ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focused.baseline.proposalsAttempted==0);
    assert(focused.proxy.proposalsAttempted>0);
    assert(focused.proxy.proposalsAccepted>0);
    assert(focused.proxy.proposalsAttempted==
           focused.proxy.proposalsAccepted+focused.proxy.proposalsRejected);
    const auto focusedJson=residentAIPairedReportJson(focused);
    assert(focusedJson.find("\"proxySelection\":\"four-core-opportunities\"")
        !=std::string::npos);
    for(const char* intent:CognitiveOpportunityIntentNames){
        assert(focusedJson.find(intent)!=std::string::npos);
    }
    assert(focusedJson.find("\"observedTradePendingStarts\":")
        !=std::string::npos);
    assert(focusedJson.find("\"observedParentingPendingStarts\":")
        !=std::string::npos);
    assert(focusedJson.find("\"observedKnowledgeTeachingPendingStarts\":")
        !=std::string::npos);
    assert(residentAIPairedReportJson(runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CoreOpportunityFour))
        ==focusedJson);
    const auto focusedOther=runResidentAIPairedAudit(
        874213954,1,ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focusedOther.seed!=focused.seed);
    assert(focusedOther.matchedInitialResidents);

    bool invalidRejected=false;
    try{ (void)runResidentAIPairedAudit(0,1); }
    catch(const std::invalid_argument&){invalidRejected=true;}
    assert(invalidRejected);
    invalidRejected=false;
    try{ (void)runResidentAIPairedAudit(1,1001); }
    catch(const std::invalid_argument&){invalidRejected=true;}
    assert(invalidRejected);
    return 0;
}
