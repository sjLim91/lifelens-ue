#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "World.h"

namespace lifelens {

enum class PerceptionChannel { Vision, Hearing, Trace };
enum class PerceptionSubject { Resource, Animal, Person, Fire, Residue };

// A dated sensory receipt, never a promise of stock, access or an outcome.
// Unseen sounds intentionally have neither an entity identity nor a position.
struct PerceptionFact {
    PerceptionChannel channel=PerceptionChannel::Vision;
    PerceptionSubject subject=PerceptionSubject::Resource;
    std::uint64_t subjectId=0;
    int minute=-1;
    bool hasPosition=false;
    GridPos position{};
    int distanceCells=0;
};

struct ResidentSensoryPosition {
    CharacterId actor=0;
    GridPos position{};
};

bool residentHasLineOfSight(const World& world,GridPos observer,GridPos target);
std::vector<PerceptionFact> perceiveResidentEnvironment(
    const World& world,CharacterId actor,GridPos position,
    const std::vector<ResidentSensoryPosition>& people={});
void rememberResidentPerceptions(Character& actor,GridPos observer,
    int minute,const std::vector<PerceptionFact>& facts);

struct ResidentMemorySummary {
    std::string domain;
    MemorySource source=MemorySource::Inferred;
    CharacterId subject=0;
    std::size_t episodes=0;
    std::size_t successes=0;
    std::size_t failures=0;
    int firstMinute=-1;
    int lastMinute=-1;
    // IDs refer to exact snapshot-backed episodes, not generated prose.
    std::vector<std::uint64_t> evidenceIds;
    std::size_t omittedEvidence=0;
    double stance=0.0;
};

struct ResidentExperiencePlace {
    std::string place;
    int minute=-1;
    double confidence=0.0;
    MemorySource source=MemorySource::Inferred;
    std::uint64_t evidenceId=0;
};

struct ResidentMindObservation {
    double curiosity=0.5;
    double habit=0.0;
    double attachment=0.0;
    double riskTolerance=0.5;
    double trust=0.5;
    std::size_t eligibleEpisodes=0;
    std::size_t omittedSummaries=0;
    std::vector<ResidentMemorySummary> summaries;
    std::vector<ResidentExperiencePlace> places;
};

// One source of saved evidence: the existing personal episode/belief store.
// Summary and disposition layers are deterministic materialized read views.
// Rebuilding them on restore avoids a second, divergent memory authority.
ResidentMindObservation observeResidentMind(const Character& actor,int minute);
double residentOptionalCuriosityMultiplier(const Character& actor,int minute);
const char* perceptionChannelName(PerceptionChannel value);
const char* perceptionSubjectName(PerceptionSubject value);

} // namespace lifelens
