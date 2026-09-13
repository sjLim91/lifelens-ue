#include <iostream>
#include "lifelens/LifeHistory.h"
using namespace lifelens;

static int fail(const char* m){ std::cerr << m << '\n'; return 1; }

int main(){
    std::vector<LifeHistoryEntry> history;
    recordLifeEvent(history,LifeEventType::Birth,0,{10,11});
    recordLifeEvent(history,LifeEventType::DatingStarted,100,{2});
    recordLifeEvent(history,LifeEventType::Married,200,{2});
    recordLifeEvent(history,LifeEventType::ChildBorn,300,{3});
    recordLifeEvent(history,LifeEventType::ChildBorn,400,{4});
    recordLifeEvent(history,LifeEventType::Bereavement,500,{2});

    if(history.size()!=6) return fail("size");
    if(countLifeEvents(history,LifeEventType::ChildBorn)!=2) return fail("count");
    if(!hasLifeEvent(history,LifeEventType::Married)) return fail("has");
    if(hasLifeEvent(history,LifeEventType::Divorced)) return fail("unexpected");

    const auto* latest=latestLifeEvent(history,LifeEventType::ChildBorn);
    if(latest==nullptr || latest->minute!=400 || latest->relatedCharacters.size()!=1 || latest->relatedCharacters[0]!=4) return fail("latest");
    if(std::string(lifeEventName(LifeEventType::PregnancyStarted))!="PregnancyStarted") return fail("name");

    std::vector<LifeHistoryEntry> clamped;
    recordLifeEvent(clamped,LifeEventType::HouseholdChanged,-50,{},7);
    if(clamped[0].minute!=0 || clamped[0].value!=7) return fail("normalize");
    return 0;
}
