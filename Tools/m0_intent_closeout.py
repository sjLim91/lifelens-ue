#!/usr/bin/env python3
"""M0-03: reconcile real 14-intent Core-input evidence by resident and seed.

The fixed deterministic proxy is NOT a language model. Core terminal outcomes
exist for three social context kinds; an accepted input, pending token or a
temporal correlation must NEVER be reported as an intent-caused completion.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from Tools.analyze_resident_ai_paired import CANONICAL_INTENTS, analyze

DAYS = (100, 365, 1000)
STAGES = ("selected", "notOffered", "noEligibleTarget", "submitted",
          "accepted", "rejected", "rejectedByValidator", "rejectedAfterValidator")
CORE_CONTEXTS = {
    "TeachKnowledge": "KnowledgeTeaching",
    "CareForDependent": "Parenting",
    "TradeWithResident": "Trade",
}


def _uint(value, name):
    if type(value) is not int or value < 0:
        raise ValueError(f"{name}: expected nonnegative integer")
    return value


def _catalog(branch, label):
    if not isinstance(branch, dict):
        raise ValueError(f"{label}: missing cohort")
    catalog = branch.get("catalogIntentInputs")
    if not isinstance(catalog, dict) or set(catalog) != CANONICAL_INTENTS:
        raise ValueError(f"{label}: missing canonical 14-intent audit")
    residents = branch.get("residents")
    if not isinstance(residents, dict) or not residents:
        raise ValueError(f"{label}: missing actual Core resident observations")
    for key, person in residents.items():
        if not isinstance(key, str) or not key.isdigit() or int(key) == 0:
            raise ValueError(f"{label}: invalid Core resident id")
        if not isinstance(person, dict) or set(person.get("catalogIntentInputs", {})) != CANONICAL_INTENTS:
            raise ValueError(f"{label}: missing per-resident 14-intent evidence")
    for intent in sorted(CANONICAL_INTENTS):
        for stage in STAGES:
            expected = _uint(catalog[intent].get(stage), f"{label}.{intent}.{stage}")
            actual = sum(
                _uint(person["catalogIntentInputs"][intent].get(stage),
                      f"{label}.{key}.{intent}.{stage}")
                for key, person in residents.items()
            )
            if actual != expected:
                raise ValueError(
                    f"{label}: per-resident {intent}.{stage} does not reconcile "
                    f"with global Core audit ({actual}!={expected})")
    for metric in ("proposalsAttempted", "proposalsAccepted", "proposalsRejected"):
        observed = sum(_uint(p.get(metric), f"{label}.{key}.{metric}")
                       for key, p in residents.items())
        if observed != _uint(branch.get(metric), f"{label}.{metric}"):
            raise ValueError(f"{label}: per-resident {metric} mismatch")
    # Core outcome facts have actor identity, unlike time-matched preference
    # events. Validate per-resident sums against actual native outcome ledger.
    for kind in ("KnowledgeTeaching", "Parenting", "Trade"):
        global_metrics = branch.get("verifiedCoreOutcomes", {}).get(kind)
        if not isinstance(global_metrics, dict):
            raise ValueError(f"{label}: missing verified Core {kind}")
        for state in ("success", "failure"):
            observed = sum(_uint(
                p.get("verifiedCoreOutcomes", {}).get(kind, {}).get(state),
                f"{label}.{key}.{kind}.{state}")
                for key, p in residents.items())
            if observed != _uint(global_metrics.get(state),
                                 f"{label}.{kind}.{state}"):
                raise ValueError(f"{label}: resident Core {kind} mismatch")
    return residents


def closeout(paths: list[Path]) -> dict:
    if len(paths) < 6:
        raise ValueError("M0-03 requires at least 2 seeds * 100/365/1000 days")
    datasets = {}
    for path in paths:
        raw = json.loads(Path(path).read_text(encoding="utf-8"))
        if raw.get("proxySelection") != "all-fourteen-core-catalog":
            raise ValueError("M0-03 requires the real fourteen-intent Core audit policy")
        examined = analyze(raw)
        seed, day = str(examined["seed"]), examined["days"]
        if day not in DAYS:
            raise ValueError(f"M0-03 unsupported checkpoint day {day}")
        key = (seed, day)
        if key in datasets:
            raise ValueError(f"M0-03 duplicate seed/day: {key}")
        people = {}
        for cohort in ("baseline", "proxy"):
            people[cohort] = _catalog(raw.get(cohort), f"{seed}/{day}/{cohort}")
        datasets[key] = (raw, people)
    seeds = sorted({seed for seed, _ in datasets})
    if len(seeds) < 2:
        raise ValueError("M0-03 requires two independent simulation seeds")
    for seed in seeds:
        if [d for d in DAYS if (seed, d) in datasets] != list(DAYS):
            raise ValueError(f"M0-03 missing days 100/365/1000 for seed {seed}")
    reports = []
    for seed in seeds:
        for day in DAYS:
            raw, people = datasets[seed, day]
            kinds = {}
            for intent in sorted(CANONICAL_INTENTS):
                baseline, proxy = (
                    raw[cohort]["catalogIntentInputs"][intent]
                    for cohort in ("baseline", "proxy")
                )
                for cohort, row in (("baseline", baseline), ("proxy", proxy)):
                    if (_uint(row["selected"], f"{intent}.selected")
                        != row["notOffered"] + row["noEligibleTarget"] + row["submitted"]
                        or row["submitted"] != row["accepted"] + row["rejected"]):
                        raise ValueError(f"{cohort}.{intent}: impossible lifecycle")
                verified_kind = CORE_CONTEXTS.get(intent)
                outcome = {
                    cohort: raw[cohort]["verifiedCoreOutcomes"][verified_kind]
                    for cohort in ("baseline", "proxy")
                } if verified_kind else None
                kinds[intent] = {
                    "baseline": {stage: baseline[stage] for stage in STAGES},
                    "proxy": {stage: proxy[stage] for stage in STAGES},
                    "coreContextEvidenceKind": verified_kind,
                    "coreObservedTerminalForKind": outcome,
                    "intentCausedCompletion": None,
                    "terminalAttributionStatus": (
                        "core_kind_observed_not_causal_link"
                        if verified_kind else "no_intent_specific_Core_receipt"),
                    "unofferedOrIneligible": (
                        proxy["notOffered"] + proxy["noEligibleTarget"]),
                    "validatorOrCommitRejections": (
                        proxy["rejectedByValidator"] + proxy["rejectedAfterValidator"]),
                }
            reports.append({
                "seed": seed, "day": day,
                "cohortsStartFromMatchingWorld": raw["matchedInitialResidents"],
                "residentCountBaseline": len(people["baseline"]),
                "residentCountProxy": len(people["proxy"]),
                "perResident": {
                    cohort: {key: {
                        "alive": person["alive"],
                        "intents": person["catalogIntentInputs"],
                        "verifiedCoreOutcomes": person["verifiedCoreOutcomes"],
                    } for key, person in members.items()}
                    for cohort, members in people.items()
                },
                "perIntent": kinds,
                "coreReturnEvidence": {
                    cohort: raw[cohort]["verifiedCoreOutcomes"].get("tradeJourneyReturns")
                    for cohort in ("baseline", "proxy")
                },
                "notVerifiedCognitionCausation": True,
            })
    return {
        "schema": "lifelens-m0-intent-closeout-v1",
        "verifiedSeeds": seeds, "verifiedDays": list(DAYS),
        "verifiedIndependentRunCount": len(datasets),
        "intentCount": len(CANONICAL_INTENTS),
        "coreEvidenceKinds": sorted(set(CORE_CONTEXTS.values())),
        "reports": reports,
        "acceptedIsNotCompletion": True,
        "missingIntentSpecificTerminalReceipts": sorted(
            set(CANONICAL_INTENTS) - set(CORE_CONTEXTS)),
        "limitations": [
            "Fixed deterministic proxy, not GPT or a resident's actual local model.",
            "Core kind-level outcomes are genuine but not intent-linked causation.",
            "Missing intent-specific terminal receipts remain explicitly unverified.",
            "No blank accepted proposal is silently marked as an executed action.",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paired_json", nargs="+", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = closeout(args.paired_json)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print("M0-03: verified 14 input intents, residents and 6 Core timelines; "
          "causal terminal gaps remain explicitly unverified")


if __name__ == "__main__":
    main()
