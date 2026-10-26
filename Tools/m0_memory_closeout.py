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



AGE_KEYS = ("age_lt_1d_entries", "age_1to7d_entries",
            "age_7to30d_entries", "age_30to365d_entries",
            "age_365d_plus_entries")


def _native_tag_breakdown(path: Path, seed: str) -> dict[int, dict]:
    """Native per-record primary tags + age partitions, NEVER guessed from text."""
    by_day: dict[int, dict] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        item = json.loads(line)
        if item.get("type") != "resident" or item.get("day") not in DAYS:
            continue
        if item.get("seed") != seed:
            raise ValueError(f"{path}:{number} wrong seed in resident attribution")
        day = item["day"]
        attr = item.get("memory_attribution")
        if not isinstance(attr, dict):
            raise ValueError(f"{path}:{number} missing native attribution")
        tags = attr.get("primary_tags")
        if not isinstance(tags, dict):
            raise ValueError(f"{path}:{number} missing native primary tags")
        per_resident = {}
        for label, bucket in tags.items():
            if not isinstance(label, str) or not label or not isinstance(bucket, dict):
                raise ValueError(f"{path}:{number} malformed native primary tag")
            per_resident[label] = {
                "entries": _count(bucket.get("entries"), "primary_tags.entries"),
                "utf8_text_bytes": _count(
                    bucket.get("utf8_text_bytes"), "primary_tags.utf8_text_bytes"),
            }
        entries = _count(attr.get("entries"), "native.entries")
        bytes_ = _count(attr.get("utf8_text_bytes"), "native.utf8_text_bytes")
        if sum(x["entries"] for x in per_resident.values()) != entries:
            raise ValueError(f"{path}:{number} primary tag count partition mismatch")
        if sum(x["utf8_text_bytes"] for x in per_resident.values()) != bytes_:
            raise ValueError(f"{path}:{number} primary tag text byte mismatch")
        ages = {k: _count(attr.get(k), k) for k in AGE_KEYS}
        future = _count(attr.get("future_entries"), "future_entries")
        if sum(ages.values()) + future != entries:
            raise ValueError(f"{path}:{number} age and future partition mismatch")
        agg = by_day.setdefault(day, {"primary_tags": {},
                                      "age_entries": {k: 0 for k in AGE_KEYS},
                                      "future_entries": 0})
        agg["future_entries"] += future
        for key in AGE_KEYS:
            agg["age_entries"][key] += ages[key]
        for label, bucket in per_resident.items():
            dest = agg["primary_tags"].setdefault(
                label, {"entries": 0, "utf8_text_bytes": 0})
            for field in ("entries", "utf8_text_bytes"):
                dest[field] += bucket[field]
    if set(by_day) != set(DAYS):
        raise ValueError(f"{path}: missing primary tag evidence on 100/365/1000")
    return by_day


def _count(value, field: str) -> int:
    if type(value) is not int or value < 0:
        raise ValueError(f"{field} must be a nonnegative integer")
    return value



def closeout(paths: list[Path]) -> dict:
    if len(paths) < 2:
        raise ValueError("M0-04 requires at least two independent seeds")
    runs = [analyze_file(Path(path)) for path in paths]
    comparison = compare_independent_seeds(runs)
    if comparison["complete_independent_seeds"] != len(runs):
        raise ValueError("M0-04: missing 100/365/1000 day checkpoint")
    seeds = sorted(run["seed"] for run in runs)
    details = []
    for run, path in zip(runs, paths):
        points = run["checkpoints"]
        groups = _native_tag_breakdown(Path(path), run["seed"])
        if [p["day"] for p in points] != list(DAYS):
            raise ValueError("M0-04: required days out of order or missing")
        for point in points:
            group = groups[point["day"]]
            point["primary_tags"] = group["primary_tags"]
            point["age_entries"] = group["age_entries"]
            point["future_entries"] = group["future_entries"]
            if (sum(x["entries"] for x in group["primary_tags"].values())
                    != point["memory"]["entries"]):
                raise ValueError("M0-04: native primary tag reconciliation mismatch")
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
            categories = sorted(
                current["primary_tags"].keys() | previous["primary_tags"].keys()
            )
            tag_delta = {
                tag: {
                    field: current["primary_tags"].get(tag, {}).get(field, 0)
                    - previous["primary_tags"].get(tag, {}).get(field, 0)
                    for field in ("entries", "utf8_text_bytes")
                } for tag in categories
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
                "primaryTagDelta": tag_delta,
                "largestGrowingTags": [
                    {"tag": key, **change}
                    for key, change in sorted(tag_delta.items(),
                        key=lambda pair: (-pair[1]["utf8_text_bytes"], pair[0]))[:8]
                ],
                "futureEntriesDelta": current["future_entries"] - previous["future_entries"],
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
