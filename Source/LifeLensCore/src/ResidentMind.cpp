#include "lifelens/ResidentMind.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>
#include "lifelens/MemoryAttribution.h"
#include "lifelens/NaturalPhysicalObstacle.h"
#include "lifelens/ResidentLocalPerception.h"
#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/CoreContextOutcomeEvidence.h"

namespace lifelens {
namespace {
constexpr std::size_t MaxFacts=12;
constexpr std::size_t MaxPerceptionMemories=48;
constexpr std::size_t MaxSummaries=24;
constexpr std::size_t MaxEvidencePerSummary=8;

bool intersects(GridPos from,GridPos to,double x,double y,double rx,double ry)
{
    double low=0.0,high=1.0;
    const auto axis=[&](double origin,double delta,double center,double half){
        if(std::abs(delta)<1e-12) return origin>=center-half &&origin<=center+half;
        double a=(center-half-origin)/delta,b=(center+half-origin)/delta;
        if(a>b) std::swap(a,b);
        low=std::max(low,a);high=std::min(high,b);
        return low<=high;
    };
    return axis(from.x,static_cast<double>(to.x)-from.x,x,rx)
        &&axis(from.y,static_cast<double>(to.y)-from.y,y,ry)
        &&high>0.001 &&low<0.999;
}

bool validEpisode(const MemoryRecord& m,CharacterId actor,int minute)
{
    return m.minute>=0 &&m.minute<=minute &&m.sourceCharacter!=0
        &&(m.source!=MemorySource::DirectWitness ||m.sourceCharacter==actor)
        &&std::isfinite(m.confidence) &&m.confidence>0.0 &&m.confidence<=1.0
        &&std::isfinite(m.importance) &&m.importance>=0.0 &&m.importance<=1.0
        &&std::isfinite(m.decayPerDay) &&m.decayPerDay>=0.0;
}

bool verifiedAction(const Character& actor,const MemoryRecord& m,int minute)
{
    if(m.source!=MemorySource::DirectWitness ||!m.witnessed
       ||m.sourceCharacter!=actor.id ||m.tags.size()!=3
       ||m.tags[0]!="action_experience"
       ||(m.tags[2]!="success" &&m.tags[2]!="failure")) return false;
    const auto& key=m.tags[1];
    if(key.rfind("Civilization:",0)==0)
        return residentAIVerifiedGatherOutcome(actor,m,key,minute).has_value();
    if(key.rfind("CoreContext:",0)==0 &&m.who!=actor.id &&m.who!=0
       &&m.where.rfind("core-context-token:",0)==0){
        for(const auto code:{CoreContextOutcomeCode::TeachingAdvanced,
            CoreContextOutcomeCode::TeachingComprehensionFailed,
            CoreContextOutcomeCode::TeachingClarityFailed,
            CoreContextOutcomeCode::TeachingRejected,
            CoreContextOutcomeCode::ParentingPerformed,
            CoreContextOutcomeCode::ParentingRejected,
            CoreContextOutcomeCode::TradeExchanged,
            CoreContextOutcomeCode::TradeExchangeRejected}){
            if(coreContextOutcomeActualSuccess(code)==(m.tags[2]=="success")
               &&m.what=="실제 행동 "+key+" 대상 "+std::to_string(m.who)
                   +" 결과: "+coreContextOutcomeCodeName(code)) return true;
        }
    }
    return m.who==actor.id
        &&(key=="Eat" ||key=="Drink" ||key=="Sleep" ||key=="Wash" ||key=="UseToilet")
        &&m.what==(m.tags[2]=="success"?"시도한 행동에 성공했다":"시도한 행동에 실패했다");
}

std::string domain(const MemoryRecord& m)
{
    if(m.tags.empty()) return "ordinary";
    if(m.tags.front()=="action_experience" &&m.tags.size()>=2){
        if(m.tags[1].rfind("Civilization:Gather:",0)==0) return "resource";
        if(m.tags[1].rfind("Civilization:Craft:",0)==0
           ||m.tags[1].rfind("Civilization:Experiment:",0)==0) return "tool";
        if(m.tags[1].rfind("CoreContext:",0)==0) return "person";
        return "action";
    }
    if(m.tags.front()=="mind_perception_v1" &&m.tags.size()>=3)
        return m.tags[2];
    for(const auto& tag:m.tags){
        if(tag=="predator" ||tag=="contamination" ||tag=="injury") return "danger";
    }
    if(m.where.rfind("resource-node:",0)==0) return "resource";
    return "ordinary";
}
} // namespace

const char* perceptionChannelName(PerceptionChannel value)
{
    switch(value){
        case PerceptionChannel::Vision:return "시각";
        case PerceptionChannel::Hearing:return "청각";
        case PerceptionChannel::Trace:return "흔적";
    }
    return "미상";
}

const char* perceptionSubjectName(PerceptionSubject value)
{
    switch(value){
        case PerceptionSubject::Resource:return "resource";
        case PerceptionSubject::Animal:return "animal";
        case PerceptionSubject::Person:return "person";
        case PerceptionSubject::Fire:return "place";
        case PerceptionSubject::Residue:return "trace";
    }
    return "ordinary";
}

bool residentHasLineOfSight(const World& world,GridPos observer,GridPos target)
{
    // Bounded cells and 64-bit subtraction protect extreme Earth coordinates.
    if(perceptionGridDistance(observer,target)>ResidentLocalPerceptionMaxRadius)
        return false;
    if(observer.x==target.x &&observer.y==target.y) return true;
    for(const auto& chunk:world.generatedNaturalChunks){
        const auto near=chunkCoordForGrid(observer);
        if(std::abs(static_cast<std::int64_t>(chunk.coord.x)-near.x)>1
           ||std::abs(static_cast<std::int64_t>(chunk.coord.y)-near.y)>1) continue;
        for(const auto& obstacle:deriveNaturalPhysicalObstacles(chunk)){
            if((obstacle.grid.x==observer.x &&obstacle.grid.y==observer.y)
               ||(obstacle.grid.x==target.x &&obstacle.grid.y==target.y)) continue;
            if(obstacle.halfHeightCells>=0.35
               &&intersects(observer,target,
                   obstacle.grid.x+obstacle.offsetXCells,
                   obstacle.grid.y+obstacle.offsetYCells,
                   obstacle.halfExtentXCells,obstacle.halfExtentYCells)) return false;
        }
    }
    for(const auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::Shelter
           ||!facilityOperationalAndActive(facility)
           ||(facility.pos.x==observer.x &&facility.pos.y==observer.y)
           ||(facility.pos.x==target.x &&facility.pos.y==target.y)) continue;
        if(intersects(observer,target,facility.pos.x,facility.pos.y,0.45,0.45)) return false;
    }
    return true;
}

std::vector<PerceptionFact> perceiveResidentEnvironment(
    const World& world,CharacterId actor,GridPos position,
    const std::vector<ResidentSensoryPosition>& people)
{
    std::vector<PerceptionFact> result;
    const auto living=std::find_if(world.characters.begin(),world.characters.end(),
        [&](const auto& c){return c.id==actor &&c.alive;});
    if(actor==0 ||living==world.characters.end() ||world.minute<0) return result;
    const auto add=[&](PerceptionChannel channel,PerceptionSubject subject,
                       std::uint64_t id,GridPos point,int radius){
        const auto distance=perceptionGridDistance(position,point);
        if(distance>radius) return;
        const bool visual=channel!=PerceptionChannel::Hearing;
        if(visual &&!residentHasLineOfSight(world,position,point)) return;
        PerceptionFact fact{channel,subject,visual?id:0,world.minute,
            visual,visual?point:GridPos{},static_cast<int>(distance)};
        const auto less=[](const auto& a,const auto& b){
            return std::tie(a.distanceCells,a.channel,a.subject,a.subjectId)
                <std::tie(b.distanceCells,b.channel,b.subject,b.subjectId);
        };
        auto at=std::upper_bound(result.begin(),result.end(),fact,less);
        if(result.size()==MaxFacts &&at==result.end()) return;
        const auto index=at-result.begin();
        if(result.size()==MaxFacts) result.pop_back();
        result.insert(result.begin()+index,fact);
    };
    for(const auto& node:world.resourceNodes)
        if(node.id &&node.quantity>0 &&node.material!=MaterialKind::Unknown)
            add(PerceptionChannel::Vision,PerceptionSubject::Resource,node.id,node.pos,6);
    if(world.wildlifeEnabled) for(const auto& population:world.wildlifePopulations)
        for(const auto& animal:population.animals) if(animal.alive)
            add(PerceptionChannel::Vision,PerceptionSubject::Animal,animal.id,animal.position,6);
    for(const auto& person:people){
        if(person.actor==actor) continue;
        const auto other=std::find_if(world.characters.begin(),world.characters.end(),
            [&](const auto& c){return c.id==person.actor &&c.alive;});
        if(other!=world.characters.end())
            add(PerceptionChannel::Vision,PerceptionSubject::Person,person.actor,person.position,6);
    }
    for(const auto& residue:world.environmentalResidues.all())
        if(residue.createdMinute<=world.minute &&residue.amount>0.01 &&residue.intensity>0.01)
            add(PerceptionChannel::Trace,PerceptionSubject::Residue,residue.id,residue.pos,3);
    // Only a real active burning fire emits a sound. No invented animal calls,
    // unseen resident activity, resource identity or exact hidden coordinate.
    for(const auto& facility:world.facilities)
        if(facility.kind==FacilityKind::FirePit &&facility.lit
           &&facilityOperationalAndActive(facility))
            add(residentHasLineOfSight(world,position,facility.pos)
                    ?PerceptionChannel::Vision:PerceptionChannel::Hearing,
                PerceptionSubject::Fire,facility.id,facility.pos,8);
    return result;
}

void rememberResidentPerceptions(Character& actor,GridPos observer,
    int minute,const std::vector<PerceptionFact>& facts)
{
    if(!actor.alive ||actor.id==0 ||minute<0) return;
    for(const auto& fact:facts){
        if(fact.minute!=minute ||fact.distanceCells<0 ||fact.distanceCells>8
           ||fact.hasPosition!=(fact.channel!=PerceptionChannel::Hearing)
           ||(fact.hasPosition &&(fact.subjectId==0
               ||perceptionGridDistance(observer,fact.position)!=fact.distanceCells))) continue;
        MemoryRecord memory;
        memory.who=fact.subject==PerceptionSubject::Person?fact.subjectId:actor.id;
        memory.sourceCharacter=actor.id;
        memory.minute=minute;
        memory.source=fact.hasPosition?MemorySource::DirectWitness:MemorySource::Inferred;
        memory.witnessed=fact.hasPosition;
        memory.confidence=fact.hasPosition?0.9:0.45;
        memory.importance=0.30;
        memory.what=fact.hasPosition?"주변의 실제 대상을 관찰했다. 이후 상태는 다시 확인해야 한다."
            :"타는 불 소리를 들었다. 보이지 않는 대상의 정체와 정확한 위치는 알 수 없다.";
        memory.where=fact.hasPosition?"grid:"+std::to_string(fact.position.x)+","+std::to_string(fact.position.y)
            :"들린 소리·위치 미상";
        memory.tags={"mind_perception_v1",perceptionChannelName(fact.channel),
            perceptionSubjectName(fact.subject),"subject:"+std::to_string(fact.subjectId)};
        auto same=std::find_if(actor.memory.entries.begin(),actor.memory.entries.end(),
            [&](const auto& prior){return prior.sourceCharacter==actor.id &&prior.tags.size()>=4
                &&std::equal(memory.tags.begin(),memory.tags.end(),prior.tags.begin());});
        if(same!=actor.memory.entries.end()){
            if(same->minute<minute){
                unsigned visits=1;
                if(same->where==memory.where &&same->tags.size()==5
                   &&same->tags[4].rfind("observations:",0)==0){
                    const auto text=same->tags[4].substr(13);
                    if(!text.empty() &&text.size()<=4 &&std::all_of(text.begin(),text.end(),
                        [](char c){return c>='0' &&c<='9';}))
                        visits=std::min(4096U,static_cast<unsigned>(std::stoul(text))+1);
                }
                memory.tags.push_back("observations:"+std::to_string(visits));
                *same=std::move(memory);
            }
        }else{
            memory.tags.push_back("observations:1");
            actor.memory.add(std::move(memory));
        }
    }
    for(;;){
        std::size_t count=0,oldest=actor.memory.entries.size();
        for(std::size_t i=0;i<actor.memory.entries.size();++i){
            const auto& m=actor.memory.entries[i];
            if(m.tags.empty() ||m.tags.front()!="mind_perception_v1") continue;
            ++count;
            if(oldest==actor.memory.entries.size()
               ||std::tie(m.minute,m.tags)<std::tie(actor.memory.entries[oldest].minute,actor.memory.entries[oldest].tags)) oldest=i;
        }
        if(count<=MaxPerceptionMemories) break;
        actor.memory.entries.erase(actor.memory.entries.begin()+oldest);
    }
}

ResidentMindObservation observeResidentMind(const Character& actor,int minute)
{
    ResidentMindObservation out;
    out.curiosity=actor.personality.curiosity;
    out.riskTolerance=actor.personality.riskTolerance;
    if(actor.id==0 ||minute<0) return out;
    using Key=std::tuple<std::string,MemorySource,CharacterId>;
    std::map<Key,ResidentMemorySummary> groups;
    std::set<std::uint64_t> seen;
    std::size_t direct=0,success=0,failure=0;
    double social=0.0,socialWeight=0.0;
    for(const auto& m:actor.memory.entries){
        if(!validEpisode(m,actor.id,minute)) continue;
        const auto id=memoryRecordFingerprint(m);
        if(!seen.insert(id).second) continue;
        ++out.eligibleEpisodes;
        const auto kind=domain(m);
        const auto subject=m.who==actor.id?0:m.who;
        const Key key{kind,m.source,subject};
        bool include=true;
        if(groups.count(key)==0 &&groups.size()>=MaxSummaries){
            auto last=std::prev(groups.end());
            if(key<last->first){out.omittedSummaries+=last->second.episodes;groups.erase(last);}
            else {++out.omittedSummaries;include=false;}
        }
        // All saved episodes still inform the private disposition layer;
        // the bounded summary display cannot bias which events are learned.
        const bool firsthand=m.source==MemorySource::DirectWitness &&m.witnessed
            &&m.sourceCharacter==actor.id;
        const bool action=verifiedAction(actor,m,minute);
        if(firsthand &&action &&minute-m.minute<=30*1440){
            ++direct;
            if(m.tags[2]=="success") ++success;
            if(m.tags[2]=="failure") ++failure;
            if(subject &&m.tags[1].rfind("CoreContext:",0)==0){
                const double weight=m.effectiveConfidence(minute);
                social+=(m.tags[2]=="success"?1.0:-1.0)*weight;
                socialWeight+=weight;
            }
        }
        if(include){
        auto& summary=groups[key];
        summary.domain=kind;summary.source=m.source;summary.subject=subject;
        ++summary.episodes;
        summary.firstMinute=summary.firstMinute<0?m.minute:std::min(summary.firstMinute,m.minute);
        summary.lastMinute=std::max(summary.lastMinute,m.minute);
        summary.evidenceIds.push_back(id);
        std::sort(summary.evidenceIds.begin(),summary.evidenceIds.end());
        if(summary.evidenceIds.size()>MaxEvidencePerSummary){summary.evidenceIds.pop_back();++summary.omittedEvidence;}
        if(action &&m.tags[2]=="success") ++summary.successes;
        if(action &&m.tags[2]=="failure") ++summary.failures;
        summary.stance=static_cast<double>(summary.successes)-summary.failures;
        summary.stance/=summary.successes+summary.failures+3.0;
        // Only own recent actual outcomes adapt disposition; hearsay and
        // sensory sightings never become success, risk or trust rewards.
        }
        if(firsthand &&m.tags.size()==5 &&m.tags[0]=="mind_perception_v1"
           &&m.tags[4].rfind("observations:",0)==0){
            const auto text=m.tags[4].substr(13);
            if(!text.empty() &&text.size()<=4 &&std::all_of(text.begin(),text.end(),
                [](char c){return c>='0' &&c<='9';})){
                const double visits=std::min(4096UL,std::stoul(text));
                out.attachment=std::max(out.attachment,visits/(visits+6.0));
            }
        }
        if(m.where.rfind("grid:",0)==0){
            auto found=std::find_if(out.places.begin(),out.places.end(),[&](const auto& p){return p.place==m.where &&p.source==m.source;});
            if(found!=out.places.end()){
                if(found->minute<m.minute ||(found->minute==m.minute &&id<found->evidenceId))
                    *found={m.where,m.minute,m.effectiveConfidence(minute),m.source,id};
            }else out.places.push_back({m.where,m.minute,m.effectiveConfidence(minute),m.source,id});
            std::sort(out.places.begin(),out.places.end(),[](const auto& a,const auto& b){
                if(a.minute!=b.minute)return a.minute>b.minute;
                return std::tie(a.place,a.source,a.evidenceId)<std::tie(b.place,b.source,b.evidenceId);});
            if(out.places.size()>12) out.places.pop_back();
        }
    }
    for(auto& entry:groups) out.summaries.push_back(std::move(entry.second));
    const double experience=(static_cast<double>(success)-failure)/(direct+3.0);
    out.curiosity=std::clamp(out.curiosity+0.10*experience,0.0,1.0);
    out.riskTolerance=std::clamp(out.riskTolerance+0.10*experience,0.0,1.0);
    out.habit=static_cast<double>(success)/(direct+3.0);
    out.trust=std::clamp(0.5+0.15*social/(socialWeight+3.0),0.0,1.0);
    return out;
}

double residentOptionalCuriosityMultiplier(const Character& actor,int minute)
{
    // The executive needs only recent verified outcomes. Do not rebuild the
    // entire long-term hierarchy (hashing/sorting every episode) each time it
    // compares optional goals in a centuries-long world.
    std::map<std::tuple<std::string,CharacterId,int>,bool> attempts;
    for(const auto& memory:actor.memory.entries){
        if(memory.minute<0 ||memory.minute>minute ||minute-memory.minute>30*1440
           ||!validEpisode(memory,actor.id,minute)
           ||!verifiedAction(actor,memory,minute)) continue;
        auto& succeeded=attempts[{memory.tags[1],memory.who,memory.minute}];
        succeeded=succeeded ||memory.tags[2]=="success";
    }
    std::size_t successes=0;
    for(const auto& attempt:attempts)if(attempt.second)++successes;
    const double experience=(2.0*successes-attempts.size())/(attempts.size()+3.0);
    const double curiosity=std::clamp(actor.personality.curiosity+0.10*experience,0.0,1.0);
    const double risk=std::clamp(actor.personality.riskTolerance+0.10*experience,0.0,1.0);
    return std::clamp(1.0+0.10*(curiosity-actor.personality.curiosity)
        +0.04*(risk-actor.personality.riskTolerance),0.97,1.03);
}
} // namespace lifelens
