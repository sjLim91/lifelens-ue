"""Longitudinal read-only audit for Core verified resident behavior.

Multiple paired JSON reports are comparable only under the SAME seed and
scripted proxy policy. Never equate accepted proposals with finished actions,
or monotonic evidence counts with learned intelligence.
"""
import argparse
import json
from pathlib import Path

# Allow both direct "python Tools/script.py" and package/module invocation.
if __package__:
    from .analyze_resident_ai_residents import analyze_paired_residents
else:
    from analyze_resident_ai_residents import analyze_paired_residents

CORE_EVENTS = ("proposalsAttempted", "proposalsAccepted", "proposalsRejected",
               "civilizationPendingStarts", "socialPendingStarts", "otherPendingStarts",
               "civilizationSuccessEvidence", "civilizationFailureEvidence",
               "firsthandCivilizationMemories", "knownTechniqueRecords")


def analyze_series(reports):
    if not isinstance(reports, list) or len(reports) < 2:
        raise ValueError("need two or more independently observed checkpoints")
    checked = [(p, analyze_paired_residents(p)) for p in reports]
    seed = checked[0][1]["seed"]
    days = [p["days"] for _, p in checked]
    if (any(p["seed"] != seed for _, p in checked)
            or any(current <= prior for prior, current in zip(days, days[1:]))):
        raise ValueError("paired reports need one seed and strictly increasing days")
    for earlier, later in zip(days, days[1:]):
        if later > 3000:
            raise ValueError("unexpected longitudinal Core horizon")
    windows = []
    for (prev, previous), (curr, current) in zip(checked, checked[1:]):
        comparisons = {}
        for rid in sorted(set(previous["perResident"]) | set(current["perResident"]), key=int):
            past = previous["perResident"].get(rid)
            present = current["perResident"].get(rid)
            if past is None or present is None:
                comparisons[rid] = {"comparable": False,
                                    "reason": "resident_absent_from_checkpoint"}
                continue
            branches = {}
            for branch in ("baseline", "proxy"):
                older = past[branch]
                newer = present[branch]
                if older is None or newer is None:
                    branches[branch] = {"comparable": False}
                    continue
                # Core identities never resurrect. A dead resident present
                # at both checkpoints cannot silently restart the same GUID.
                if not older["alive"] and newer["alive"]:
                    raise ValueError(f"{rid}: impossible dead-to-alive resident transition")
                changes = {}
                for metric in CORE_EVENTS:
                    delta = newer[metric] - older[metric]
                    # Working memory and held technique records can shrink.
                    # Their counts are observations, NOT cumulative successes.
                    if (metric not in ("firsthandCivilizationMemories",
                                       "knownTechniqueRecords") and delta < 0):
                        raise ValueError(f"{rid}: nonmonotonic Core {branch}.{metric}")
                    changes[metric] = delta
                outcomes = {}
                past_kinds = older["verifiedCoreOutcomes"]
                current_kinds = newer["verifiedCoreOutcomes"]
                for kind in sorted(set(past_kinds) | set(current_kinds)):
                    if kind not in current_kinds:
                        raise ValueError(f"{rid}: disappeared Core outcome kind {kind}")
                    if kind not in past_kinds:
                        # A new report category may contain prior unreported
                        # outcomes. Do not invent a zero baseline or drop it.
                        outcomes[kind] = {
                            "comparable": False,
                            "reason": "outcome_kind_missing_from_previous_checkpoint",
                        }
                        continue
                    old = past_kinds[kind]
                    new = current_kinds[kind]
                    success = new["success"] - old["success"]
                    failure = new["failure"] - old["failure"]
                    if success < 0 or failure < 0:
                        raise ValueError(f"{rid}: nonmonotonic verified Core outcome {kind}")
                    outcomes[kind] = {
                        "comparable": True,
                        "successDelta": success,
                        "failureDelta": failure,
                    }
                trade_returns = newer["tradeJourneyReturns"] - older["tradeJourneyReturns"]
                if trade_returns < 0:
                    raise ValueError(f"{rid}: nonmonotonic verified Core trade return")
                branches[branch] = {
                    "comparable": True,
                    "observedMetricDelta": changes,
                    "residentAliveBefore": older["alive"],
                    "residentAliveAfter": newer["alive"],
                    "verifiedCoreOutcomeDelta": outcomes,
                    "tradeJourneyReturnsDelta": trade_returns,
                    "acceptedProposalsAreNotCompletedActions": True,
                }
            comparisons[rid] = branches
        # Never compare whole-population deltas when births, deaths, or
        # evidence schema changes make two cohorts unequal. Compare only
        # residents present in BOTH branches at BOTH checkpoints.
        paired_residents = [
            row for row in comparisons.values()
            if row.get("baseline", {}).get("comparable")
            and row.get("proxy", {}).get("comparable")
        ]
        kinds = sorted({
            kind
            for row in paired_residents
            for branch in ("baseline", "proxy")
            for kind in row[branch]["verifiedCoreOutcomeDelta"]
        })
        paired_outcomes = {}
        for kind in kinds:
            matched = [
                row for row in paired_residents
                if row["baseline"]["verifiedCoreOutcomeDelta"].get(kind, {}).get("comparable")
                and row["proxy"]["verifiedCoreOutcomeDelta"].get(kind, {}).get("comparable")
            ]
            baseline_success = sum(
                row["baseline"]["verifiedCoreOutcomeDelta"][kind]["successDelta"]
                for row in matched
            )
            proxy_success = sum(
                row["proxy"]["verifiedCoreOutcomeDelta"][kind]["successDelta"]
                for row in matched
            )
            baseline_failure = sum(
                row["baseline"]["verifiedCoreOutcomeDelta"][kind]["failureDelta"]
                for row in matched
            )
            proxy_failure = sum(
                row["proxy"]["verifiedCoreOutcomeDelta"][kind]["failureDelta"]
                for row in matched
            )
            paired_outcomes[kind] = {
                "comparableResidentCount": len(matched),
                "excludedResidentCount": len(comparisons) - len(matched),
                "baselineSuccessDelta": baseline_success,
                "proxySuccessDelta": proxy_success,
                "observedSuccessDifference": proxy_success - baseline_success,
                "baselineFailureDelta": baseline_failure,
                "proxyFailureDelta": proxy_failure,
                "observedFailureDifference": proxy_failure - baseline_failure,
            }
        windows.append({
            "fromDay": prev["days"], "toDay": curr["days"],
            "residents": comparisons,
            "pairedOutcomeComparison": {
                "comparableResidentCount": len(paired_residents),
                "excludedResidentCount": len(comparisons) - len(paired_residents),
                "outcomeKinds": paired_outcomes,
                "causalLearningEvidence": False,
                "note": ("Same-resident paired Core outcomes are observations, "
                         "not proof that the proxy or learning caused a change. "
                         "Unavailable/absent outcomes are excluded, not zero."),
            },
        })
    return {
        "seed": seed,
        "days": days,
        "windows": windows,
        "isLanguageModelExperiment": False,
        "causalImprovementDemonstrated": False,
        "longitudinalLearningProven": False,
        "note": ("Observed changes in Core-verified actions and firsthand memory "
                 "counts are NOT proof that learning caused different decisions. "
                 "Pending or accepted proposals are not successful outcomes."),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reports", type=Path, nargs="+")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = analyze_series([
        json.loads(path.read_text(encoding="utf-8")) for path in args.reports
    ])
    output = json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output)


if __name__ == "__main__":
    main()
