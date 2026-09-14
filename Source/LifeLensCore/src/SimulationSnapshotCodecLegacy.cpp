#include "lifelens/SimulationSnapshotCodec.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <type_traits>

namespace lifelens {
namespace {

constexpr char Magic[]={'L','L','S','N','A','P','0','1'};
constexpr std::uint32_t MaxCollectionCount=1000000;
constexpr std::uint32_t MaxStringBytes=16*1024*1024;

void setError(std::string* error,const char* message)
{
    if(error) *error=message;
}

class Writer {
public:
    std::vector<std::uint8_t> bytes;

    void raw(const void* data,std::size_t size)
    {
        const auto* p=static_cast<const std::uint8_t*>(data);
        bytes.insert(bytes.end(),p,p+size);
    }

    void u8(std::uint8_t value){ bytes.push_back(value); }
    void boolean(bool value){ u8(value?1u:0u); }

    void u32(std::uint32_t value)
    {
        for(int i=0;i<4;++i) u8(static_cast<std::uint8_t>((value>>(i*8))&0xffu));
    }

    void u64(std::uint64_t value)
    {
        for(int i=0;i<8;++i) u8(static_cast<std::uint8_t>((value>>(i*8))&0xffu));
    }

    void i32(std::int32_t value){ u32(static_cast<std::uint32_t>(value)); }
    void i64(std::int64_t value){ u64(static_cast<std::uint64_t>(value)); }

    void real(double value)
    {
        static_assert(sizeof(double)==sizeof(std::uint64_t),"LifeLens snapshot requires 64-bit double");
        std::uint64_t bits=0;
        std::memcpy(&bits,&value,sizeof(bits));
        u64(bits);
    }

    void string(const std::string& value)
    {
        u32(static_cast<std::uint32_t>(value.size()));
        if(!value.empty()) raw(value.data(),value.size());
    }

    template<typename Enum>
    void enumeration(Enum value)
    {
        static_assert(std::is_enum<Enum>::value,"enum required");
        i32(static_cast<std::int32_t>(value));
    }
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& input):bytes(input){}

    bool raw(void* out,std::size_t size)
    {
        if(size>bytes.size()-offset) return false;
        if(size>0) std::memcpy(out,bytes.data()+offset,size);
        offset+=size;
        return true;
    }

    bool u8(std::uint8_t& value)
    {
        if(offset>=bytes.size()) return false;
        value=bytes[offset++];
        return true;
    }

    bool boolean(bool& value)
    {
        std::uint8_t rawValue=0;
        if(!u8(rawValue) || rawValue>1) return false;
        value=rawValue!=0;
        return true;
    }

    bool u32(std::uint32_t& value)
    {
        value=0;
        for(int i=0;i<4;++i){
            std::uint8_t part=0;
            if(!u8(part)) return false;
            value|=static_cast<std::uint32_t>(part)<<(i*8);
        }
        return true;
    }

    bool u64(std::uint64_t& value)
    {
        value=0;
        for(int i=0;i<8;++i){
            std::uint8_t part=0;
            if(!u8(part)) return false;
            value|=static_cast<std::uint64_t>(part)<<(i*8);
        }
        return true;
    }

    bool i32(std::int32_t& value)
    {
        std::uint32_t rawValue=0;
        if(!u32(rawValue)) return false;
        value=static_cast<std::int32_t>(rawValue);
        return true;
    }

    bool i64(std::int64_t& value)
    {
        std::uint64_t rawValue=0;
        if(!u64(rawValue)) return false;
        value=static_cast<std::int64_t>(rawValue);
        return true;
    }

    bool real(double& value)
    {
        std::uint64_t bits=0;
        if(!u64(bits)) return false;
        std::memcpy(&value,&bits,sizeof(bits));
        return true;
    }

    bool string(std::string& value)
    {
        std::uint32_t size=0;
        if(!u32(size) || size>MaxStringBytes || size>bytes.size()-offset) return false;
        value.assign(reinterpret_cast<const char*>(bytes.data()+offset),size);
        offset+=size;
        return true;
    }

    template<typename Enum>
    bool enumeration(Enum& value)
    {
        static_assert(std::is_enum<Enum>::value,"enum required");
        std::int32_t rawValue=0;
        if(!i32(rawValue)) return false;
        value=static_cast<Enum>(rawValue);
        return true;
    }

    bool count(std::uint32_t& value)
    {
        return u32(value) && value<=MaxCollectionCount;
    }

    bool done() const { return offset==bytes.size(); }

private:
    const std::vector<std::uint8_t>& bytes;
    std::size_t offset=0;
};

template<typename Container,typename WriteItem>
void writeCollection(Writer& w,const Container& items,WriteItem writeItem)
{
    w.u32(static_cast<std::uint32_t>(items.size()));
    for(const auto& item:items) writeItem(w,item);
}

template<typename T,typename ReadItem>
bool readVector(Reader& r,std::vector<T>& items,ReadItem readItem)
{
    std::uint32_t count=0;
    if(!r.count(count)) return false;
    std::vector<T> result;
    result.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        T item{};
        if(!readItem(r,item)) return false;
        result.push_back(std::move(item));
    }
    items=std::move(result);
    return true;
}

void writeStringValue(Writer& w,const std::string& value){ w.string(value); }
bool readStringValue(Reader& r,std::string& value){ return r.string(value); }
void writeCharacterId(Writer& w,CharacterId value){ w.u64(value); }
bool readCharacterId(Reader& r,CharacterId& value){ return r.u64(value); }
void writeObjectId(Writer& w,ObjectId value){ w.u64(value); }
bool readObjectId(Reader& r,ObjectId& value){ return r.u64(value); }

void writeNeedsDelta(Writer& w,const NeedsDelta& value)
{
    w.real(value.hunger); w.real(value.thirst); w.real(value.sleep); w.real(value.bladder); w.real(value.hygiene);
}
bool readNeedsDelta(Reader& r,NeedsDelta& value)
{
    return r.real(value.hunger)&&r.real(value.thirst)&&r.real(value.sleep)&&r.real(value.bladder)&&r.real(value.hygiene);
}
void writeNeeds(Writer& w,const Needs& value)
{
    w.real(value.hunger); w.real(value.thirst); w.real(value.sleep); w.real(value.bladder); w.real(value.hygiene);
}
bool readNeeds(Reader& r,Needs& value)
{
    return r.real(value.hunger)&&r.real(value.thirst)&&r.real(value.sleep)&&r.real(value.bladder)&&r.real(value.hygiene);
}
void writePersonality(Writer& w,const Personality& p)
{
    w.real(p.introversion); w.real(p.conscientiousness); w.real(p.openness); w.real(p.agreeableness);
    w.real(p.emotionalStability); w.real(p.empathy); w.real(p.impulsiveness); w.real(p.riskTolerance);
    w.real(p.ambition); w.real(p.patience); w.real(p.sociability); w.real(p.curiosity); w.real(p.orderliness); w.real(p.adaptability);
}
bool readPersonality(Reader& r,Personality& p)
{
    return r.real(p.introversion)&&r.real(p.conscientiousness)&&r.real(p.openness)&&r.real(p.agreeableness)&&
        r.real(p.emotionalStability)&&r.real(p.empathy)&&r.real(p.impulsiveness)&&r.real(p.riskTolerance)&&
        r.real(p.ambition)&&r.real(p.patience)&&r.real(p.sociability)&&r.real(p.curiosity)&&r.real(p.orderliness)&&r.real(p.adaptability);
}
void writeEmotion(Writer& w,const EmotionState& e)
{
    w.real(e.joy); w.real(e.sadness); w.real(e.anger); w.real(e.fear); w.real(e.embarrassment); w.real(e.pride);
    w.real(e.jealousy); w.real(e.affection); w.real(e.anxiety); w.real(e.relief); w.real(e.grief); w.real(e.valence); w.real(e.arousal);
}
bool readEmotion(Reader& r,EmotionState& e)
{
    return r.real(e.joy)&&r.real(e.sadness)&&r.real(e.anger)&&r.real(e.fear)&&r.real(e.embarrassment)&&r.real(e.pride)&&
        r.real(e.jealousy)&&r.real(e.affection)&&r.real(e.anxiety)&&r.real(e.relief)&&r.real(e.grief)&&r.real(e.valence)&&r.real(e.arousal);
}
void writeMemoryRecord(Writer& w,const MemoryRecord& m)
{
    w.u64(m.who); w.u64(m.sourceCharacter); w.string(m.what); w.string(m.where); w.i32(m.minute);
    w.real(m.emotionValence); w.real(m.emotionIntensity); w.real(m.importance); w.real(m.confidence);
    w.boolean(m.witnessed); w.enumeration(m.source); w.real(m.decayPerDay);
    writeCollection(w,m.tags,writeStringValue);
}
bool readMemoryRecord(Reader& r,MemoryRecord& m)
{
    return r.u64(m.who)&&r.u64(m.sourceCharacter)&&r.string(m.what)&&r.string(m.where)&&r.i32(m.minute)&&
        r.real(m.emotionValence)&&r.real(m.emotionIntensity)&&r.real(m.importance)&&r.real(m.confidence)&&
        r.boolean(m.witnessed)&&r.enumeration(m.source)&&r.real(m.decayPerDay)&&
        readVector(r,m.tags,readStringValue);
}
void writeMemoryState(Writer& w,const MemoryState& state){ writeCollection(w,state.entries,writeMemoryRecord); }
bool readMemoryState(Reader& r,MemoryState& state){ return readVector(r,state.entries,readMemoryRecord); }

void writeBeliefRecord(Writer& w,const BeliefRecord& b)
{
    w.u64(b.subject); w.string(b.proposition); w.real(b.stance); w.real(b.confidence); w.real(b.supportWeight); w.real(b.contradictionWeight);
    w.u64(static_cast<std::uint64_t>(b.supportCount)); w.u64(static_cast<std::uint64_t>(b.contradictionCount)); w.i32(b.lastUpdatedMinute);
}
bool readBeliefRecord(Reader& r,BeliefRecord& b)
{
    std::uint64_t support=0,contradiction=0;
    if(!r.u64(b.subject)||!r.string(b.proposition)||!r.real(b.stance)||!r.real(b.confidence)||!r.real(b.supportWeight)||!r.real(b.contradictionWeight)||
       !r.u64(support)||!r.u64(contradiction)||!r.i32(b.lastUpdatedMinute)) return false;
    if(support>std::numeric_limits<std::size_t>::max()||contradiction>std::numeric_limits<std::size_t>::max()) return false;
    b.supportCount=static_cast<std::size_t>(support); b.contradictionCount=static_cast<std::size_t>(contradiction); return true;
}
void writeBeliefState(Writer& w,const BeliefState& state){ writeCollection(w,state.beliefs,writeBeliefRecord); }
bool readBeliefState(Reader& r,BeliefState& state){ return readVector(r,state.beliefs,readBeliefRecord); }

void writeGenetics(Writer& w,const GeneticsProfile& g)
{
    w.real(g.faceShape); w.real(g.eyePigment); w.real(g.hairPigment); w.real(g.skinTone); w.real(g.heightPotential);
    w.real(g.buildPotential); w.real(g.healthPotential); w.real(g.learningPotential); w.real(g.temperamentSensitivity);
}
bool readGenetics(Reader& r,GeneticsProfile& g)
{
    return r.real(g.faceShape)&&r.real(g.eyePigment)&&r.real(g.hairPigment)&&r.real(g.skinTone)&&r.real(g.heightPotential)&&
        r.real(g.buildPotential)&&r.real(g.healthPotential)&&r.real(g.learningPotential)&&r.real(g.temperamentSensitivity);
}
void writeDevelopment(Writer& w,const ChildDevelopment& d)
{
    w.real(d.attachment); w.real(d.confidence); w.real(d.stress); w.real(d.socialSkill); w.real(d.emotionalSecurity);
    w.real(d.disciplineInternalization); w.real(d.learningSupport); w.real(d.health);
}
bool readDevelopment(Reader& r,ChildDevelopment& d)
{
    return r.real(d.attachment)&&r.real(d.confidence)&&r.real(d.stress)&&r.real(d.socialSkill)&&r.real(d.emotionalSecurity)&&
        r.real(d.disciplineInternalization)&&r.real(d.learningSupport)&&r.real(d.health);
}
void writeLifeCondition(Writer& w,const LifeCondition& c)
{
    w.real(c.physicalHealth); w.real(c.energyCapacity); w.real(c.movementCapacity); w.real(c.reproductivePotential);
    w.real(c.workCapacity); w.real(c.appearanceAgeFactor); w.real(c.lifeGoalFamilyFocus); w.real(c.familyRoleSalience);
}
bool readLifeCondition(Reader& r,LifeCondition& c)
{
    return r.real(c.physicalHealth)&&r.real(c.energyCapacity)&&r.real(c.movementCapacity)&&r.real(c.reproductivePotential)&&
        r.real(c.workCapacity)&&r.real(c.appearanceAgeFactor)&&r.real(c.lifeGoalFamilyFocus)&&r.real(c.familyRoleSalience);
}
void writeLifeHistoryEntry(Writer& w,const LifeHistoryEntry& e)
{
    w.enumeration(e.type); w.i32(e.minute); writeCollection(w,e.relatedCharacters,writeCharacterId); w.i32(e.value);
}
bool readLifeHistoryEntry(Reader& r,LifeHistoryEntry& e)
{
    return r.enumeration(e.type)&&r.i32(e.minute)&&readVector(r,e.relatedCharacters,readCharacterId)&&r.i32(e.value);
}
void writeCharacter(Writer& w,const Character& c)
{
    w.u64(c.id); w.string(c.name); w.enumeration(c.sex); writeNeeds(w,c.needs); writePersonality(w,c.personality); writeEmotion(w,c.emotion);
    writeMemoryState(w,c.memory); writeBeliefState(w,c.beliefs); writeGenetics(w,c.genetics); writeDevelopment(w,c.development); writeLifeCondition(w,c.lifeCondition);
    writeCollection(w,c.parentIds,writeCharacterId); writeCollection(w,c.childrenIds,writeCharacterId); writeCollection(w,c.lifeHistory,writeLifeHistoryEntry);
    w.boolean(c.hasBirthMinute); w.i32(c.birthMinute); w.enumeration(c.lifeStage); w.boolean(c.alive); w.i32(c.deathMinute);
    w.real(c.baseMetabolism); w.real(c.baseSleepTendency); w.real(c.metabolism); w.real(c.sleepTendency);
}
bool readCharacter(Reader& r,Character& c)
{
    return r.u64(c.id)&&r.string(c.name)&&r.enumeration(c.sex)&&readNeeds(r,c.needs)&&readPersonality(r,c.personality)&&readEmotion(r,c.emotion)&&
        readMemoryState(r,c.memory)&&readBeliefState(r,c.beliefs)&&readGenetics(r,c.genetics)&&readDevelopment(r,c.development)&&readLifeCondition(r,c.lifeCondition)&&
        readVector(r,c.parentIds,readCharacterId)&&readVector(r,c.childrenIds,readCharacterId)&&readVector(r,c.lifeHistory,readLifeHistoryEntry)&&
        r.boolean(c.hasBirthMinute)&&r.i32(c.birthMinute)&&r.enumeration(c.lifeStage)&&r.boolean(c.alive)&&r.i32(c.deathMinute)&&
        r.real(c.baseMetabolism)&&r.real(c.baseSleepTendency)&&r.real(c.metabolism)&&r.real(c.sleepTendency);
}

void writeSmartObject(Writer& w,const SmartObject& o)
{
    w.u64(o.id); w.enumeration(o.kind); w.i32(o.pos.x); w.i32(o.pos.y); w.boolean(o.reservedBy.has_value());
    if(o.reservedBy) w.u64(*o.reservedBy); writeNeedsDelta(w,o.effectPerTick); w.i32(o.useDurationTicks);
}
bool readSmartObject(Reader& r,SmartObject& o)
{
    bool hasReservation=false;
    if(!r.u64(o.id)||!r.enumeration(o.kind)||!r.i32(o.pos.x)||!r.i32(o.pos.y)||!r.boolean(hasReservation)) return false;
    if(hasReservation){ CharacterId id=0; if(!r.u64(id)) return false; o.reservedBy=id; } else o.reservedBy.reset();
    return readNeedsDelta(r,o.effectPerTick)&&r.i32(o.useDurationTicks);
}

void writeWorld(Writer& w,const World& world)
{
    w.i32(world.minute); w.u64(world.seed);
    std::ostringstream rngState; rngState<<world.rng; w.string(rngState.str());
    writeCollection(w,world.characters,writeCharacter); writeCollection(w,world.objects,writeSmartObject);
}
bool readWorld(Reader& r,World& world)
{
    std::int32_t minute=0; std::uint64_t seed=0; std::string rngState; std::vector<Character> characters; std::vector<SmartObject> objects;
    if(!r.i32(minute)||!r.u64(seed)||seed==0||!r.string(rngState)||!readVector(r,characters,readCharacter)||!readVector(r,objects,readSmartObject)) return false;
    World result(seed); result.minute=minute; std::istringstream rngInput(rngState); rngInput>>result.rng; if(!rngInput) return false;
    result.characters=std::move(characters); result.objects=std::move(objects); world=std::move(result); return true;
}

void writeRelationship(Writer& w,const Relationship& x)
{
    w.u64(x.from); w.u64(x.to); w.real(x.affection); w.real(x.trust); w.real(x.respect); w.real(x.comfort); w.real(x.familiarity);
    w.real(x.attraction); w.real(x.romanticInterest); w.real(x.sexualAttraction); w.real(x.commitment); w.real(x.conflict); w.real(x.jealousy); w.real(x.fear); w.real(x.grudge);
}
bool readRelationship(Reader& r,Relationship& x)
{
    return r.u64(x.from)&&r.u64(x.to)&&r.real(x.affection)&&r.real(x.trust)&&r.real(x.respect)&&r.real(x.comfort)&&r.real(x.familiarity)&&
        r.real(x.attraction)&&r.real(x.romanticInterest)&&r.real(x.sexualAttraction)&&r.real(x.commitment)&&r.real(x.conflict)&&r.real(x.jealousy)&&r.real(x.fear)&&r.real(x.grudge);
}
void writeGenealogyNode(Writer& w,const GenealogyNode& n)
{
    w.u64(n.characterId); writeCollection(w,n.parents,writeCharacterId); writeCollection(w,n.children,writeCharacterId); writeCollection(w,n.spouses,writeCharacterId);
}
bool readGenealogyNode(Reader& r,GenealogyNode& n)
{
    return r.u64(n.characterId)&&readVector(r,n.parents,readCharacterId)&&readVector(r,n.children,readCharacterId)&&readVector(r,n.spouses,readCharacterId);
}
void writeRomancePair(Writer& w,const RomancePair& p)
{
    w.u64(p.first); w.u64(p.second); w.u64(p.initiator); w.enumeration(p.stage); w.i32(p.startedMinute); w.i32(p.engagedMinute); w.i32(p.marriedMinute); w.i32(p.endedMinute);
}
bool readRomancePair(Reader& r,RomancePair& p)
{
    return r.u64(p.first)&&r.u64(p.second)&&r.u64(p.initiator)&&r.enumeration(p.stage)&&r.i32(p.startedMinute)&&r.i32(p.engagedMinute)&&r.i32(p.marriedMinute)&&r.i32(p.endedMinute);
}
void writeResponsibilities(Writer& w,const HouseholdResponsibilities& x)
{
    w.real(x.cooking); w.real(x.cleaning); w.real(x.shopping); w.real(x.maintenance); w.real(x.caregiving);
}
bool readResponsibilities(Reader& r,HouseholdResponsibilities& x)
{
    return r.real(x.cooking)&&r.real(x.cleaning)&&r.real(x.shopping)&&r.real(x.maintenance)&&r.real(x.caregiving);
}
void writeHouseholdMember(Writer& w,const HouseholdMember& m)
{
    w.u64(m.characterId); writeResponsibilities(w,m.responsibilities); w.real(m.contributionWeight);
}
bool readHouseholdMember(Reader& r,HouseholdMember& m)
{
    return r.u64(m.characterId)&&readResponsibilities(r,m.responsibilities)&&r.real(m.contributionWeight);
}
void writeHousehold(Writer& w,const Household& h)
{
    w.u64(h.id); writeCollection(w,h.members,writeHouseholdMember); w.u64(h.home); w.real(h.resources); w.real(h.sharedMoney); writeCollection(w,h.sharedObjects,writeObjectId);
}
bool readHousehold(Reader& r,Household& h)
{
    return r.u64(h.id)&&readVector(r,h.members,readHouseholdMember)&&r.u64(h.home)&&r.real(h.resources)&&r.real(h.sharedMoney)&&readVector(r,h.sharedObjects,readObjectId);
}
void writePregnancy(Writer& w,const PregnancyState& p)
{
    w.u64(p.gestationalParent); w.u64(p.geneticPartner); w.i32(p.conceptionMinute); w.i32(p.dueMinute); w.i32(p.lastUpdateMinute); w.enumeration(p.stage);
    w.real(p.health); w.real(p.fatigue); w.real(p.stress); w.real(p.nutrition);
}
bool readPregnancy(Reader& r,PregnancyState& p)
{
    return r.u64(p.gestationalParent)&&r.u64(p.geneticPartner)&&r.i32(p.conceptionMinute)&&r.i32(p.dueMinute)&&r.i32(p.lastUpdateMinute)&&r.enumeration(p.stage)&&
        r.real(p.health)&&r.real(p.fatigue)&&r.real(p.stress)&&r.real(p.nutrition);
}
void writeBirth(Writer& w,const BirthRecord& b)
{
    w.u64(b.childId); w.u64(b.parentA); w.u64(b.parentB); w.i32(b.birthMinute); w.u64(b.householdId);
}
bool readBirth(Reader& r,BirthRecord& b)
{
    return r.u64(b.childId)&&r.u64(b.parentA)&&r.u64(b.parentB)&&r.i32(b.birthMinute)&&r.u64(b.householdId);
}
void writeAction(Writer& w,const Action& a)
{
    w.enumeration(a.type); w.u64(a.objectId); w.i32(a.remainingTicks);
}
bool readAction(Reader& r,Action& a)
{
    return r.enumeration(a.type)&&r.u64(a.objectId)&&r.i32(a.remainingTicks);
}
void writeRuntime(Writer& w,const SimulationRuntimeSnapshot& x)
{
    w.enumeration(x.goal); writeCollection(w,x.plan,writeAction); w.u64(static_cast<std::uint64_t>(x.actionIndex));
    w.i32(x.pos.x); w.i32(x.pos.y); w.boolean(x.announced); w.enumeration(x.lastGoal); w.i32(x.repeatCount); w.i32(x.consecutiveFailures);
    w.i32(x.penaltyUntilMinute); w.i32(x.socialCooldownUntilMinute); w.boolean(x.socialActive); w.enumeration(x.socialIntent); w.u64(x.socialTarget);
}
bool readRuntime(Reader& r,SimulationRuntimeSnapshot& x)
{
    std::uint64_t actionIndex=0;
    if(!r.enumeration(x.goal)||!readVector(r,x.plan,readAction)||!r.u64(actionIndex)||actionIndex>std::numeric_limits<std::size_t>::max()||
       !r.i32(x.pos.x)||!r.i32(x.pos.y)||!r.boolean(x.announced)||!r.enumeration(x.lastGoal)||!r.i32(x.repeatCount)||!r.i32(x.consecutiveFailures)||
       !r.i32(x.penaltyUntilMinute)||!r.i32(x.socialCooldownUntilMinute)||!r.boolean(x.socialActive)||!r.enumeration(x.socialIntent)||!r.u64(x.socialTarget)) return false;
    x.actionIndex=static_cast<std::size_t>(actionIndex); return true;
}

bool rebuildRelationshipBook(const std::vector<Relationship>& items,RelationshipBook& book)
{
    RelationshipBook result;
    for(const auto& item:items) result.getOrCreate(item.from,item.to)=item;
    book=std::move(result); return true;
}
bool rebuildGenealogyBook(const std::vector<GenealogyNode>& items,GenealogyBook& book)
{
    GenealogyBook result;
    for(const auto& item:items) result.getOrCreate(item.characterId)=item;
    book=std::move(result); return true;
}
bool rebuildRomanceBook(const std::vector<RomancePair>& items,RomanceBook& book)
{
    RomanceBook result;
    for(const auto& item:items){
        if(!result.startDating(item.first,item.second,item.initiator,item.startedMinute)) return false;
        RomancePair* created=result.findActivePair(item.first,item.second); if(!created) return false; *created=item;
    }
    book=std::move(result); return true;
}
bool rebuildHouseholdBook(const std::vector<Household>& items,HouseholdBook& book)
{
    HouseholdBook result;
    for(const auto& item:items){
        if(!result.create(item.id,{})) return false;
        Household* created=result.find(item.id); if(!created) return false; *created=item;
    }
    book=std::move(result); return true;
}
bool rebuildPregnancyBook(const std::vector<PregnancyState>& items,PregnancyBook& book)
{
    PregnancyBook result;
    for(const auto& item:items){
        PregnancyState* created=result.start(item.gestationalParent,item.geneticPartner,item.conceptionMinute); if(!created) return false; *created=item;
    }
    book=std::move(result); return true;
}
bool rebuildBirthBook(const std::vector<BirthRecord>& items,BirthBook& book)
{
    BirthBook result;
    for(const auto& item:items) if(!result.add(item)) return false;
    book=std::move(result); return true;
}

} // namespace

bool encodeSimulationSnapshot(const SimulationStateSnapshot& snapshot,std::vector<std::uint8_t>& outBytes,std::string* error)
{
    if(snapshot.version!=SimulationSnapshotVersion){ setError(error,"unsupported Core snapshot version"); return false; }
    Writer w; w.raw(Magic,sizeof(Magic)); w.u32(SimulationSnapshotBinaryFormatVersion); w.u32(snapshot.version); writeWorld(w,snapshot.world);
    writeCollection(w,snapshot.relationships.all(),writeRelationship);
    writeCollection(w,snapshot.genealogy.all(),writeGenealogyNode);
    writeCollection(w,snapshot.romances.all(),writeRomancePair);
    writeCollection(w,snapshot.households.all(),writeHousehold);
    writeCollection(w,snapshot.pregnancies.all(),writePregnancy);
    writeCollection(w,snapshot.births.all(),writeBirth);

    std::vector<CharacterId> runtimeIds; runtimeIds.reserve(snapshot.runtime.size());
    for(const auto& item:snapshot.runtime) runtimeIds.push_back(item.first);
    std::sort(runtimeIds.begin(),runtimeIds.end());
    w.u32(static_cast<std::uint32_t>(runtimeIds.size()));
    for(CharacterId id:runtimeIds){ w.u64(id); writeRuntime(w,snapshot.runtime.at(id)); }
    writeCollection(w,snapshot.logs,writeStringValue);
    outBytes=std::move(w.bytes); if(error) error->clear(); return true;
}

bool decodeSimulationSnapshot(const std::vector<std::uint8_t>& bytes,SimulationStateSnapshot& outSnapshot,std::string* error)
{
    Reader r(bytes); char magic[sizeof(Magic)]{}; std::uint32_t binaryVersion=0,snapshotVersion=0;
    if(!r.raw(magic,sizeof(magic))||std::memcmp(magic,Magic,sizeof(Magic))!=0){ setError(error,"invalid snapshot magic"); return false; }
    if(!r.u32(binaryVersion)||binaryVersion!=SimulationSnapshotBinaryFormatVersion){ setError(error,"unsupported snapshot binary format"); return false; }
    if(!r.u32(snapshotVersion)||snapshotVersion!=SimulationSnapshotVersion){ setError(error,"unsupported Core snapshot version"); return false; }

    SimulationStateSnapshot decoded; decoded.version=snapshotVersion;
    if(!readWorld(r,decoded.world)){ setError(error,"invalid world payload"); return false; }

    std::vector<Relationship> relationships; if(!readVector(r,relationships,readRelationship)||!rebuildRelationshipBook(relationships,decoded.relationships)){ setError(error,"invalid relationship payload"); return false; }
    std::vector<GenealogyNode> genealogy; if(!readVector(r,genealogy,readGenealogyNode)||!rebuildGenealogyBook(genealogy,decoded.genealogy)){ setError(error,"invalid genealogy payload"); return false; }
    std::vector<RomancePair> romances; if(!readVector(r,romances,readRomancePair)||!rebuildRomanceBook(romances,decoded.romances)){ setError(error,"invalid romance payload"); return false; }
    std::vector<Household> households; if(!readVector(r,households,readHousehold)||!rebuildHouseholdBook(households,decoded.households)){ setError(error,"invalid household payload"); return false; }
    std::vector<PregnancyState> pregnancies; if(!readVector(r,pregnancies,readPregnancy)||!rebuildPregnancyBook(pregnancies,decoded.pregnancies)){ setError(error,"invalid pregnancy payload"); return false; }
    std::vector<BirthRecord> births; if(!readVector(r,births,readBirth)||!rebuildBirthBook(births,decoded.births)){ setError(error,"invalid birth payload"); return false; }

    std::uint32_t runtimeCount=0; if(!r.count(runtimeCount)){ setError(error,"invalid runtime count"); return false; }
    for(std::uint32_t i=0;i<runtimeCount;++i){
        CharacterId id=0; SimulationRuntimeSnapshot runtime;
        if(!r.u64(id)||!readRuntime(r,runtime)||!decoded.runtime.emplace(id,std::move(runtime)).second){ setError(error,"invalid runtime payload"); return false; }
    }
    if(!readVector(r,decoded.logs,readStringValue)){ setError(error,"invalid log payload"); return false; }
    if(!r.done()){ setError(error,"snapshot contains trailing bytes"); return false; }

    outSnapshot=std::move(decoded); if(error) error->clear(); return true;
}

} // namespace lifelens
