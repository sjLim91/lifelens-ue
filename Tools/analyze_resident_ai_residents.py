"""Audit every resident's REAL Core outcomes against paired aggregate totals.

This is a read-only report. The Core comparison uses a scripted deterministic
proposal proxy, not a language model, and delta is not causal evidence.
"""
import argparse
import json
import re
from pathlib import Path


FIELDS = (
    ("proposalsAttempted", "proposalsAttempted"),
    ("proposalsAccepted", "proposalsAccepted"),
    ("proposalsRejected", "proposalsRejected"),
    ("civilizationPendingStarts", "observedCivilizationPendingStarts"),
    ("socialPendingStarts", "observedSocialPendingStarts"),
    ("otherPendingStarts", "observedOtherPendingStarts"),
    ("civilizationSuccessEvidence", "civilizationSuccessEvidence"),
    ("civilizationFailureEvidence", "civilizationFailureEvidence"),
    ("firsthandCivilizationMemories", "firsthandCivilizationMemories"),
    ("knownTechniqueRecords", "knownTechniqueRecords"),
)
POLICY = "fixed-deterministic-proxy-not-llm"


def nonnegative(value, field):
    if type(value) is not int or value < 0:
        raise ValueError(f"{field}: expected nonnegative integer")
    return value


def inspect_branch(data, label):
    if not isinstance(data, dict) or not isinstance(data.get("residents"), dict):
        raise ValueError(f"{label}: missing resident projection")
    residents = data["residents"]
    if not residents:
        raise ValueError(f"{label}: empty resident projection")
    sums = {global_key: 0 for _, global_key in FIELDS}
    living = 0
    kinds = data.get("verifiedCoreOutcomes")
    if not isinstance(kinds, dict) or not kinds:
        raise ValueError(f"{label}: missing authoritative outcomes")
    names = set(kinds) - {"tradeJourneyReturns"}
    if not names or "tradeJourneyReturns" not in kinds:
        raise ValueError(f"{label}: incomplete Core outcome kinds")
    succeeded = {name: 0 for name in names}
    failed = {name: 0 for name in names}
    returns = 0
    normalized = {}
    for resident_id, entry in residents.items():
        if not isinstance(resident_id, str) or not re.fullmatch(r"[1-9][0-9]*", resident_id):
            raise ValueError(f"{label}: invalid lossless resident ID {resident_id}")
        if int(resident_id) > 2**64-1 or not isinstance(entry, dict):
            raise ValueError(f"{label}: invalid resident {resident_id}")
        if type(entry.get("alive")) is not bool:
            raise ValueError(f"{label}: missing resident alive truth {resident_id}")
        living += int(entry["alive"])
        metrics = {"alive": entry["alive"]}
        for local_key, global_key in FIELDS:
            value = nonnegative(entry.get(local_key), f"{label}.{resident_id}.{local_key}")
            metrics[local_key] = value
            sums[global_key] += value
        if metrics["proposalsAttempted"] != (
            metrics["proposalsAccepted"] + metrics["proposalsRejected"]
        ):
            raise ValueError(f"{label}.{resident_id}: proposal lifecycle mismatch")
        observations = entry.get("verifiedCoreOutcomes")
        if (not isinstance(observations, dict)
                or set(observations) != (names | {"tradeJourneyReturns"})):
            raise ValueError(f"{label}.{resident_id}: missing/extra outcome kinds")
        metrics["verifiedCoreOutcomes"] = {}
        for name in sorted(names):
            source = observations[name]
            if not isinstance(source, dict):
                raise ValueError(f"{label}.{resident_id}.{name}: malformed outcome")
            success = nonnegative(source.get("success"), f"{label}.{resident_id}.{name}.success")
            failure = nonnegative(source.get("failure"), f"{label}.{resident_id}.{name}.failure")
            succeeded[name] += success
            failed[name] += failure
            metrics["verifiedCoreOutcomes"][name] = {
                "success": success, "failure": failure,
            }
        returned = nonnegative(observations.get("tradeJourneyReturns"),
                               f"{label}.{resident_id}.verifiedCoreOutcomes.tradeJourneyReturns")
        returns += returned
        metrics["tradeJourneyReturns"] = returned
        normalized[resident_id] = metrics

    for key, value in sums.items():
        if value != nonnegative(data.get(key), f"{label}.{key}"):
            raise ValueError(f"{label}: resident/summary mismatch {key}")
    if living != nonnegative(data.get("livingResidents"), f"{label}.livingResidents"):
        raise ValueError(f"{label}: living resident count mismatch")
    if returns != nonnegative(kinds.get("tradeJourneyReturns"),
                              f"{label}.tradeJourneyReturns"):
        raise ValueError(f"{label}: trade return mismatch")
    for name in names:
        global_value = kinds[name]
        if not isinstance(global_value, dict):
            raise ValueError(f"{label}.{name}: malformed Core outcome total")
        if (succeeded[name] != nonnegative(global_value.get("success"),
                                           f"{label}.{name}.success")
            or failed[name] != nonnegative(global_value.get("failure"),
                                          f"{label}.{name}.failure")):
            raise ValueError(f"{label}.{name}: resident/verified Core outcome mismatch")
    return normalized


def analyze_paired_residents(report):
    if not isinstance(report, dict) or report.get("policy") != POLICY:
        raise ValueError("only paired deterministic proxy truth is supported")
    if report.get("matchedInitialResidents") is not True:
        raise ValueError("initial resident identity mismatch")
    seed = nonnegative(report.get("seed"), "seed")
    days = nonnegative(report.get("days"), "days")
    if seed == 0 or days == 0:
        raise ValueError("invalid paired simulation identity")
    baseline = inspect_branch(report.get("baseline"), "baseline")
    proxy = inspect_branch(report.get("proxy"), "proxy")
    return {
        "seed": seed,
        "days": days,
        "policy": POLICY,
        "isLanguageModelExperiment": False,
        "causalImprovementDemonstrated": False,
        "perResident": {
            rid: {
                "baseline": baseline.get(rid),
                "proxy": proxy.get(rid),
                # Birth/death/branch differences cannot be silently converted
                # to zero: only compare an actor visible in BOTH branches.
                "comparable": rid in baseline and rid in proxy,
            }
            for rid in sorted(set(baseline) | set(proxy), key=int)
        },
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("report", type=Path)
    parser.add_argument("--output", type=Path)
    options = parser.parse_args()
    data = analyze_paired_residents(
        json.loads(options.report.read_text(encoding="utf-8")))
    result = json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if options.output:
        options.output.write_text(result, encoding="utf-8")
    else:
        print(result)


if __name__ == "__main__":
    main()
