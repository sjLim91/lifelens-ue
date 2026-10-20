"""Compare observed resident-AI paired Core evidence without claiming causality.

Input is the JSON printed by ll_resident_ai_paired_audit. Its proxy is a
fixed deterministic proposal policy, NOT a language model.
"""
import argparse
import json
from pathlib import Path

POLICY = "fixed-deterministic-proxy-not-llm"
COUNT_FIELDS = (
    "proposalsAttempted", "proposalsAccepted", "proposalsRejected",
    "observedCivilizationPendingStarts", "observedSocialPendingStarts",
    "civilizationSuccessEvidence", "civilizationFailureEvidence",
    "firsthandCivilizationMemories", "livingResidents",
)


def _count(value, path):
    if type(value) is not int or value < 0:
        raise ValueError(f"{path} must be a nonnegative integer")
    return value


def _metric(value, path):
    if not isinstance(value, dict):
        raise ValueError(f"{path} must be an object")
    counts = {key: _count(value.get(key), f"{path}.{key}")
              for key in COUNT_FIELDS}
    if counts["proposalsAttempted"] != (
        counts["proposalsAccepted"] + counts["proposalsRejected"]
    ):
        raise ValueError(f"{path}: attempted != accepted + rejected")
    catalog = value.get("catalogIntentInputs")
    if not isinstance(catalog, dict) or not catalog:
        raise ValueError(f"{path}: missing 14-intent evidence")
    for intent, row in catalog.items():
        if not isinstance(row, dict):
            raise ValueError(f"{path}: invalid intent {intent}")
        keys = ("selected", "notOffered", "noEligibleTarget",
                "submitted", "accepted", "rejected",
                "rejectedByValidator", "rejectedAfterValidator")
        m = {key: _count(row.get(key), f"{path}.{intent}.{key}")
             for key in keys}
        if (m["selected"] != m["notOffered"] + m["noEligibleTarget"]
                + m["submitted"]
            or m["submitted"] != m["accepted"] + m["rejected"]
            or m["rejected"] != m["rejectedByValidator"]
                + m["rejectedAfterValidator"]):
            raise ValueError(f"{path}: inconsistent intent lifecycle {intent}")
    verified = value.get("verifiedCoreOutcomes")
    attributed = value.get("acceptedIntentToVerifiedOutcome")
    if not isinstance(verified, dict) or not isinstance(attributed, dict):
        raise ValueError(f"{path}: missing verified/attribution evidence")
    labels = set(verified) - {"tradeJourneyReturns"}
    if not labels or labels != set(attributed):
        raise ValueError(f"{path}: verified and attributed kinds differ")
    real = {}
    temporal = {}
    for label in sorted(labels):
        item = verified[label]
        match = attributed[label]
        if not isinstance(item, dict) or not isinstance(match, dict):
            raise ValueError(f"{path}: malformed {label} outcome")
        real[label] = {key: _count(item.get(key), f"{path}.{label}.{key}")
                       for key in ("success", "failure")}
        temporal[label] = {key: _count(match.get(key), f"{path}.{label}.{key}")
                           for key in ("temporallyMatchedSuccess",
                                       "temporallyMatchedFailure",
                                       "observedWithoutAcceptedIntent",
                                       "noObservedPendingToken")}
        if (temporal[label]["temporallyMatchedSuccess"]
                > real[label]["success"]
            or temporal[label]["temporallyMatchedFailure"]
                > real[label]["failure"]):
            raise ValueError(f"{path}: matched outcomes exceed Core truth {label}")
    return counts, real, temporal


def analyze(report):
    if not isinstance(report, dict) or report.get("policy") != POLICY:
        raise ValueError("only the fixed deterministic Core proxy is supported")
    seed = _count(report.get("seed"), "seed")
    days = _count(report.get("days"), "days")
    if seed == 0 or days == 0 or type(report.get("matchedInitialResidents")) is not bool:
        raise ValueError("missing simulation identity or paired baseline evidence")
    if not report["matchedInitialResidents"]:
        raise ValueError("paired cohorts did not start from the same residents")
    baseline, base_real, base_temporal = _metric(report.get("baseline"), "baseline")
    proxy, proxy_real, proxy_temporal = _metric(report.get("proxy"), "proxy")
    del base_temporal, proxy_temporal
    keys = ("proposalsAttempted", "proposalsAccepted", "proposalsRejected",
            "observedCivilizationPendingStarts", "observedSocialPendingStarts",
            "civilizationSuccessEvidence", "civilizationFailureEvidence",
            "firsthandCivilizationMemories", "livingResidents")
    return {
        "seed": seed,
        "days": days,
        "policy": POLICY,
        "languageModelEvaluated": False,
        "causalImprovementDemonstrated": False,
        "comparison": {
            key: {"baseline": baseline[key], "proxy": proxy[key],
                  "deltaProxyMinusBaseline": proxy[key] - baseline[key]}
            for key in keys
        },
        "verifiedCoreOutcomes": {
            kind: {
                "baseline": base_real[kind],
                "proxy": proxy_real[kind],
                "successDelta": (
                    proxy_real[kind]["success"] - base_real[kind]["success"]),
                "failureDelta": (
                    proxy_real[kind]["failure"] - base_real[kind]["failure"]),
            }
            for kind in sorted(base_real)
        },
        "interpretation": (
            "Proposals, accepted inputs, and pending starts are not successful "
            "actions. Verified Core outcomes are observed, not proof that the "
            "proxy caused them. Temporal matches alone are not causal proof."
        ),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("report", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = analyze(json.loads(args.report.read_text(encoding="utf-8")))
    serialized = json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(serialized + "\n", encoding="utf-8")
    else:
        print(serialized)


if __name__ == "__main__":
    main()
