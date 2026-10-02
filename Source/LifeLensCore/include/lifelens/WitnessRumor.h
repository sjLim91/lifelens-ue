#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include "Belief.h"
#include "Ids.h"
#include "Memory.h"

namespace lifelens {

using SocialFactId = std::uint64_t;

struct SocialFact {
    SocialFactId id = 0;
    CharacterId subject = 0;
    std::string proposition;
    std::string where;
    int eventMinute = 0;
    bool supports = true;
    double importance = 0.5;
    double confidence = 1.0;
    double emotionValence = 0.0;
    double emotionIntensity = 0.0;

    void normalize() {
        importance = MemoryRecord::clamp01(importance);
        confidence = MemoryRecord::clamp01(confidence);
        emotionValence = MemoryRecord::clampSigned(emotionValence);
        emotionIntensity = MemoryRecord::clamp01(emotionIntensity);
    }

    bool valid() const {
        return id != 0 && subject != 0 && !proposition.empty();
    }
};

struct KnowledgeReceipt {
    SocialFactId factId = 0;
    CharacterId holder = 0;
    CharacterId originWitness = 0;
    CharacterId immediateSource = 0;
    CharacterId subject = 0;
    bool supports = true;
    double confidence = 0.0;
    int learnedMinute = 0;
    MemorySource source = MemorySource::ToldByOther;
    std::vector<CharacterId> transmissionPath;

    std::size_t hopCount() const {
        return transmissionPath.empty() ? 0 : transmissionPath.size() - 1;
    }
};

struct SocialStatement {
    SocialFactId factId = 0;
    CharacterId speaker = 0;
    CharacterId originWitness = 0;
    CharacterId subject = 0;
    std::string proposition;
    std::string where;
    int eventMinute = 0;
    int spokenMinute = 0;
    bool supports = true;
    double confidence = 0.0;
    std::vector<CharacterId> transmissionPath;

    bool valid() const {
        return factId != 0 && speaker != 0 && subject != 0 &&
               !proposition.empty() && !transmissionPath.empty() &&
               transmissionPath.back() == speaker;
    }
};

enum class StatementReceptionResult {
    Accepted,
    Duplicate,
    LoopSuppressed,
    TooWeak,
    Invalid
};

struct StatementReceptionOutcome {
    StatementReceptionResult result = StatementReceptionResult::Invalid;
    double acceptedConfidence = 0.0;
    double beliefStance = 0.0;
    double beliefConfidence = 0.0;
};

inline std::uint64_t mixKnowledge64(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31);
}

inline double deterministicKnowledgeUnit(
    std::uint64_t worldSeed,
    SocialFactId factId,
    CharacterId first,
    CharacterId second,
    std::uint64_t epoch) {

    std::uint64_t value = mixKnowledge64(worldSeed ^ 0x4C4C4B4E4F574C44ull);
    value = mixKnowledge64(value ^ factId);
    value = mixKnowledge64(value ^ first);
    value = mixKnowledge64(value ^ (second * 0x9E3779B97F4A7C15ull));
    value = mixKnowledge64(value ^ epoch);
    const std::uint64_t mantissa = value >> 11;
    return static_cast<double>(mantissa) / static_cast<double>(1ull << 53);
}

class SocialKnowledgeBook {
public:
    bool registerFact(SocialFact fact) {
        fact.normalize();
        if (!fact.valid()) return false;

        const auto existing=factIndex_.find(fact.id);
        if(existing!=factIndex_.end()){
            return sameFact(facts_[existing->second],fact);
        }

        const std::size_t index=facts_.size();
        const SocialFactId id=fact.id;
        facts_.push_back(std::move(fact));
        factIndex_[id]=index;
        return true;
    }

    const SocialFact* findFact(SocialFactId factId) const {
        const auto it=factIndex_.find(factId);
        return it==factIndex_.end()
            ? nullptr
            : &facts_[it->second];
    }

    const KnowledgeReceipt* findReceipt(CharacterId holder, SocialFactId factId) const {
        const auto holderIt=receiptIndex_.find(holder);
        if(holderIt==receiptIndex_.end()) return nullptr;
        const auto factIt=holderIt->second.find(factId);
        return factIt==holderIt->second.end()
            ? nullptr
            : &receipts_[factIt->second];
    }

    bool hasReceipt(CharacterId holder, SocialFactId factId) const {
        return findReceipt(holder, factId) != nullptr;
    }

    const std::vector<SocialFact>& facts() const { return facts_; }
    const std::vector<KnowledgeReceipt>& receipts() const { return receipts_; }

    void clear() {
        facts_.clear();
        receipts_.clear();
        factIndex_.clear();
        receiptIndex_.clear();
    }

    bool restoreState(
        std::vector<SocialFact> facts,
        std::vector<KnowledgeReceipt> receipts) {

        SocialKnowledgeBook rebuilt;
        for (auto& fact : facts) {
            if (!rebuilt.registerFact(std::move(fact))) return false;
        }

        for (auto& receipt : receipts) {
            const SocialFact* fact = rebuilt.findFact(receipt.factId);
            if (fact == nullptr || receipt.holder == 0 || receipt.originWitness == 0 ||
                receipt.immediateSource == 0 || receipt.subject != fact->subject ||
                receipt.learnedMinute < 0 || !std::isfinite(receipt.confidence) ||
                receipt.confidence < 0.0 || receipt.confidence > 1.0 ||
                receipt.transmissionPath.empty() ||
                receipt.transmissionPath.back() != receipt.holder ||
                receipt.transmissionPath.front() != receipt.originWitness) {
                return false;
            }

            if(rebuilt.findReceipt(receipt.holder,receipt.factId)!=nullptr){
                return false;
            }
            for (std::size_t i = 0; i < receipt.transmissionPath.size(); ++i) {
                if (receipt.transmissionPath[i] == 0) return false;
                for (std::size_t j = i + 1; j < receipt.transmissionPath.size(); ++j) {
                    if (receipt.transmissionPath[i] == receipt.transmissionPath[j]) return false;
                }
            }

            if (receipt.source == MemorySource::DirectWitness) {
                if (receipt.transmissionPath.size() != 1 ||
                    receipt.originWitness != receipt.holder ||
                    receipt.immediateSource != receipt.holder) return false;
            } else if (receipt.source == MemorySource::ToldByOther) {
                if (receipt.transmissionPath.size() < 2 ||
                    receipt.immediateSource != receipt.transmissionPath[receipt.transmissionPath.size() - 2]) return false;
            } else {
                return false;
            }

            const std::size_t receiptIndex=rebuilt.receipts_.size();
            const CharacterId holder=receipt.holder;
            const SocialFactId factId=receipt.factId;
            rebuilt.receipts_.push_back(std::move(receipt));
            rebuilt.receiptIndex_[holder][factId]=receiptIndex;
        }

        *this = std::move(rebuilt);
        return true;
    }

    const KnowledgeReceipt* recordDirectWitness(
        SocialFactId factId,
        CharacterId witness,
        MemoryState& memory,
        BeliefState& beliefs,
        int currentMinute) {

        const SocialFact* fact = findFact(factId);
        if (fact == nullptr || witness == 0) return nullptr;
        if (const KnowledgeReceipt* existing = findReceipt(witness, factId)) return existing;

        MemoryRecord record;
        record.who = fact->subject;
        record.sourceCharacter = witness;
        record.what = fact->proposition;
        record.where = fact->where;
        record.minute = fact->eventMinute;
        record.emotionValence = fact->emotionValence;
        record.emotionIntensity = fact->emotionIntensity;
        record.importance = fact->importance;
        record.confidence = fact->confidence;
        record.witnessed = true;
        record.source = MemorySource::DirectWitness;
        record.tags = {
            std::string("fact:") + std::to_string(fact->id),
            "direct-witness"
        };
        memory.add(std::move(record));
        beliefs.ingestMemory(memory.entries.back(), fact->proposition, fact->supports, currentMinute);

        KnowledgeReceipt receipt;
        receipt.factId = fact->id;
        receipt.holder = witness;
        receipt.originWitness = witness;
        receipt.immediateSource = witness;
        receipt.subject = fact->subject;
        receipt.supports = fact->supports;
        receipt.confidence = fact->confidence;
        receipt.learnedMinute = currentMinute;
        receipt.source = MemorySource::DirectWitness;
        receipt.transmissionPath = {witness};
        const std::size_t receiptIndex=receipts_.size();
        const CharacterId holder=receipt.holder;
        const SocialFactId receiptFactId=receipt.factId;
        receipts_.push_back(std::move(receipt));
        receiptIndex_[holder][receiptFactId]=receiptIndex;
        return &receipts_.back();
    }

    std::optional<SocialStatement> makeStatement(
        SocialFactId factId,
        CharacterId speaker,
        int spokenMinute,
        std::uint64_t worldSeed) const {

        const SocialFact* fact = findFact(factId);
        const KnowledgeReceipt* receipt = findReceipt(speaker, factId);
        if (fact == nullptr || receipt == nullptr || receipt->transmissionPath.empty()) {
            return std::nullopt;
        }

        SocialStatement statement;
        statement.factId = factId;
        statement.speaker = speaker;
        statement.originWitness = receipt->originWitness;
        statement.subject = fact->subject;
        statement.proposition = fact->proposition;
        statement.where = fact->where;
        statement.eventMinute = fact->eventMinute;
        statement.spokenMinute = spokenMinute;
        statement.supports = receipt->supports;
        statement.transmissionPath = receipt->transmissionPath;

        const double daysHeld = std::max(0, spokenMinute - receipt->learnedMinute) / 1440.0;
        const double ageRetention = std::exp(-0.015 * daysHeld);
        const double fidelityRoll = deterministicKnowledgeUnit(
            worldSeed,
            factId,
            speaker,
            receipt->originWitness,
            static_cast<std::uint64_t>(receipt->hopCount() + 1));
        const double retellRetention = 0.86 + 0.10 * fidelityRoll;
        statement.confidence = MemoryRecord::clamp01(
            receipt->confidence * ageRetention * retellRetention);

        if (statement.confidence < 0.35) {
            const double distortionRoll = deterministicKnowledgeUnit(
                worldSeed ^ 0x44535452544E5255ull,
                factId,
                speaker,
                receipt->originWitness,
                static_cast<std::uint64_t>(receipt->hopCount() + 17));
            const double flipChance = (0.35 - statement.confidence) * 0.20;
            if (distortionRoll < flipChance) statement.supports = !statement.supports;
        }

        return statement;
    }

    StatementReceptionOutcome receiveStatement(
        const SocialStatement& statement,
        CharacterId receiver,
        double receiverTrustInSpeaker,
        MemoryState& memory,
        BeliefState& beliefs,
        int currentMinute,
        std::uint64_t worldSeed) {

        StatementReceptionOutcome outcome;
        if (!statement.valid() || receiver == 0 || receiver == statement.speaker) {
            outcome.result = StatementReceptionResult::Invalid;
            return outcome;
        }

        const SocialFact* fact = findFact(statement.factId);
        if (fact == nullptr || fact->subject != statement.subject ||
            fact->proposition != statement.proposition) {
            outcome.result = StatementReceptionResult::Invalid;
            return outcome;
        }

        if (hasReceipt(receiver, statement.factId)) {
            outcome.result = StatementReceptionResult::Duplicate;
            return outcome;
        }

        if (std::find(
                statement.transmissionPath.begin(),
                statement.transmissionPath.end(),
                receiver) != statement.transmissionPath.end()) {
            outcome.result = StatementReceptionResult::LoopSuppressed;
            return outcome;
        }

        const double trust = MemoryRecord::clamp01(receiverTrustInSpeaker);
        const double trustWeighted = statement.confidence * (0.15 + 0.85 * trust);
        const double comprehensionRoll = deterministicKnowledgeUnit(
            worldSeed ^ 0x524350544E434F4Dull,
            statement.factId,
            statement.speaker,
            receiver,
            static_cast<std::uint64_t>(statement.transmissionPath.size()));
        const double acceptedConfidence = MemoryRecord::clamp01(
            trustWeighted * (0.92 + 0.08 * comprehensionRoll));

        if (acceptedConfidence < 0.25) {
            outcome.result = StatementReceptionResult::TooWeak;
            return outcome;
        }

        MemoryRecord record;
        record.who = statement.subject;
        record.sourceCharacter = statement.speaker;
        record.what = statement.proposition;
        record.where = statement.where;
        record.minute = currentMinute;
        record.importance = fact->importance;
        record.confidence = acceptedConfidence;
        record.witnessed = false;
        record.source = MemorySource::ToldByOther;
        record.tags = {
            std::string("fact:") + std::to_string(statement.factId),
            "rumor",
            std::string("hop:") + std::to_string(statement.transmissionPath.size())
        };
        memory.add(std::move(record));
        BeliefRecord& belief = beliefs.ingestMemory(
            memory.entries.back(),
            statement.proposition,
            statement.supports,
            currentMinute);

        KnowledgeReceipt receipt;
        receipt.factId = statement.factId;
        receipt.holder = receiver;
        receipt.originWitness = statement.originWitness;
        receipt.immediateSource = statement.speaker;
        receipt.subject = statement.subject;
        receipt.supports = statement.supports;
        receipt.confidence = acceptedConfidence;
        receipt.learnedMinute = currentMinute;
        receipt.source = MemorySource::ToldByOther;
        receipt.transmissionPath = statement.transmissionPath;
        receipt.transmissionPath.push_back(receiver);
        const std::size_t receiptIndex=receipts_.size();
        const CharacterId holder=receipt.holder;
        const SocialFactId receiptFactId=receipt.factId;
        receipts_.push_back(std::move(receipt));
        receiptIndex_[holder][receiptFactId]=receiptIndex;

        outcome.result = StatementReceptionResult::Accepted;
        outcome.acceptedConfidence = acceptedConfidence;
        outcome.beliefStance = belief.stance;
        outcome.beliefConfidence = belief.confidence;
        return outcome;
    }

private:
    static bool sameFact(const SocialFact& a, const SocialFact& b) {
        return a.id == b.id &&
               a.subject == b.subject &&
               a.proposition == b.proposition &&
               a.where == b.where &&
               a.eventMinute == b.eventMinute &&
               a.supports == b.supports;
    }

    // Derived runtime indexes only. Authoritative/persisted order stays in
    // facts_/receipts_; snapshot format therefore remains unchanged.
    std::vector<SocialFact> facts_;
    std::vector<KnowledgeReceipt> receipts_;
    std::unordered_map<SocialFactId,std::size_t> factIndex_;
    std::unordered_map<
        CharacterId,
        std::unordered_map<SocialFactId,std::size_t>
    > receiptIndex_;
};

} // namespace lifelens
