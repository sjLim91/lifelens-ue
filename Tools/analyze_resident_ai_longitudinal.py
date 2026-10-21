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
               "firsthandCivilizationMemories")


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
                changes = {}
                for metric in CORE_EVENTS:
                    delta = newer[metric] - older[metric]
                    if (metric != "firsthandCivilizationMemories" and delta < 0):
                        raise ValueError(f"{rid}: nonmonotonic Core {branch}.{metric}")
                    changes[metric] = delta
                outcomes = {}
                for kind in older["verifiedCoreOutcomes"]:
                    if kind not in newer["verifiedCoreOutcomes"]:
                        raise ValueError(f"{rid}: disappeared Core outcome kind {kind}")
                    old = older["verifiedCoreOutcomes"][kind]
                    new = newer["verifiedCoreOutcomes"][kind]
                    success = new["success"] - old["success"]
                    failure = new["failure"] - old["failure"]
                    if success < 0 or failure < 0:
                        raise ValueError(f"{rid}: nonmonotonic verified Core outcome {kind}")
                    outcomes[kind] = {"successDelta": success, "failureDelta": failure}
                branches[branch] = {
                    "comparable": True,
                    "observedMetricDelta": changes,
                    "verifiedCoreOutcomeDelta": outcomes,
                    "acceptedProposalsAreNotCompletedActions": True,
                }
            comparisons[rid] = branches
        windows.append({
            "fromDay": prev["days"], "toDay": curr["days"],
            "residents": comparisons,
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
