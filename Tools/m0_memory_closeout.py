#!/usr/bin/env python3
"""M0-04: verify real 100/365/1000-day, multi-seed Core memory evidence.

Read-only: never count missing reports, invented bytes, or action tags as
physical execution. Exit nonzero if ANY mandatory evidence is absent.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from Tools.analyze_memory_checkpoints import (
    DAYS, SOURCE_KEYS, analyze_file, compare_independent_seeds,
)


def closeout(paths: list[Path]) -> dict:
    if len(paths) < 2:
        raise ValueError("M0-04 requires at least two independent seeds")
    runs = [analyze_file(Path(path)) for path in paths]
    comparison = compare_independent_seeds(runs)
    if comparison["complete_independent_seeds"] != len(runs):
        raise ValueError("M0-04: missing 100/365/1000 day checkpoint")
    seeds = sorted(run["seed"] for run in runs)
    details = []
    for run in runs:
        points = run["checkpoints"]
        if [p["day"] for p in points] != list(DAYS):
            raise ValueError("M0-04: required days out of order or missing")
        for point in points:
            if point["checkpoint_reconciliation"] != "verified":
                raise ValueError("M0-04: missing independently reconciled Core totals")
            if sum(point["sources"].values()) != point["memory"]["entries"]:
                raise ValueError("M0-04: memory source partition mismatch")
        intervals = []
        for previous, current in zip(points, points[1:]):
            current_actions = current["action_experiences_by_key"]
            previous_actions = previous["action_experiences_by_key"]
            counts = {k: current_actions.get(k, {}).get("entries", 0)
                      - previous_actions.get(k, {}).get("entries", 0)
                      for k in sorted(current_actions.keys() | previous_actions.keys())}
            source_delta = {
                source: current["sources"][source] - previous["sources"][source]
                for source in SOURCE_KEYS
            }
            entries_delta = current["memory"]["entries"] - previous["memory"]["entries"]
            if sum(source_delta.values()) != entries_delta:
                raise ValueError("M0-04: unexplained source count delta")
            # Action tags are a SUBSET of all memory. Their delta need not be
            # positive, and the residual is not assumed to be leakage.
            intervals.append({
                "fromDay": previous["day"], "toDay": current["day"],
                "snapshotBytesDelta": current["snapshot_bytes"] - previous["snapshot_bytes"],
                "textBytesDelta": current["memory"]["utf8_text_bytes"] - previous["memory"]["utf8_text_bytes"],
                "memoryEntriesDelta": entries_delta,
                "residentCountDelta": current["resident_count"] - previous["resident_count"],
                "sourceEntriesDelta": source_delta,
                "actionEntriesDelta": counts,
                "otherMemoryEntriesDelta": entries_delta - sum(counts.values()),
                "exactDuplicatesDelta": (
                    current["memory"]["exact_duplicate_entries"]
                    - previous["memory"]["exact_duplicate_entries"]),
                "note": "Storage snapshot bytes are NOT the sum of UTF-8 payloads; action tags are NOT Core action receipts.",
            })
        details.append({
            "seed": run["seed"], "verifiedDays": run["verified_days"],
            "checkpoints": points, "growthIntervals": intervals,
        })
    return {
        "schema": "lifelens-m0-memory-closeout-v1",
        "acceptance": "measured_multi_seed_100_365_1000",
        "verifiedSeedCount": len(seeds),
        "verifiedSeeds": seeds,
        "sourceTruth": "native ll_balance_audit checkpoint and per-resident memory_attribution",
        "analysis": details,
        "comparison": comparison,
        "limitations": [
            "A memory record's success tag is not proof a Core action occurred.",
            "Snapshot includes world state beyond character memory.",
            "Text bytes exclude vector capacities and object overhead.",
            "Growth, even positive growth, does not independently prove a memory leak.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("metrics_jsonl", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    options = parser.parse_args()
    result = closeout(options.metrics_jsonl)
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(
        json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print("M0-04 verified actual multi-seed days 100/365/1000: "
          + ", ".join(result["verifiedSeeds"]))


if __name__ == "__main__":
    main()
