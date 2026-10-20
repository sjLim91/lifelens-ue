#!/usr/bin/env python3
"""Read-only per-seed 100/365/1000-day LifeLens memory and snapshot audit.

Consumes the JSONL records produced by ll_balance_audit. This does not run a
simulation, change its rules, or mistake action-memory labels for execution.
Missing checkpoints are reported as missing: never interpolate fake numbers.
"""
from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

DAYS = (100, 365, 1000)
SOURCE_KEYS = ("direct_witness_entries", "told_by_other_entries", "inferred_entries")


def _nonnegative(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(f"{field} must be a nonnegative integer")
    return value


def analyze_file(path: Path) -> dict[str, Any]:
    checkpoints: dict[int, dict[str, Any]] = {}
    residents: dict[int, dict[str, dict[str, Any]]] = defaultdict(dict)
    seed: str | None = None
    with path.open("r", encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            if not line.strip():
                continue
            data = json.loads(line)
            if not isinstance(data, dict):
                raise ValueError(f"{path}:{number} expected a JSON object")
            current_seed = data.get("seed")
            if not isinstance(current_seed, str) or not current_seed:
                raise ValueError(f"{path}:{number} missing seed")
            if seed is None:
                seed = current_seed
            elif seed != current_seed:
                raise ValueError(f"{path}:{number} mixed simulation seeds")
            day = data.get("day")
            if type(day) is not int or day not in DAYS:
                continue
            kind = data.get("type")
            if kind == "checkpoint":
                if day in checkpoints:
                    raise ValueError(f"{path}:{number} duplicate checkpoint day {day}")
                checkpoints[day] = data
            elif kind == "resident":
                resident = data.get("id")
                if not isinstance(resident, str) or not resident:
                    raise ValueError(f"{path}:{number} resident id missing")
                if resident in residents[day]:
                    raise ValueError(f"{path}:{number} duplicate resident {resident}")
                residents[day][resident] = data

    if seed is None:
        raise ValueError(f"{path}: empty input")
    points = []
    for day in DAYS:
        if day not in checkpoints:
            continue
        cp = checkpoints[day]
        if "snapshot_bytes" not in cp:
            raise ValueError(f"{path}: checkpoint {day} has no verified snapshot bytes")
        total_bytes = _nonnegative(cp["snapshot_bytes"], "snapshot_bytes")
        per_action: dict[str, dict[str, int]] = defaultdict(
            lambda: dict(entries=0, successful=0, failed=0,
                         witnessed_direct=0, indirect_or_unverified=0,
                         utf8_text_bytes=0))
        total = dict(entries=0, utf8_text_bytes=0,
                     exact_duplicate_entries=0, witnessed_direct_entries=0)
        sources = {key: 0 for key in SOURCE_KEYS}
        for resident in residents[day].values():
            attribution = resident.get("memory_attribution")
            if not isinstance(attribution, dict):
                raise ValueError(f"{path}: day {day} lacks resident memory audit")
            for key in total:
                total[key] += _nonnegative(attribution.get(key), key)
            for key in SOURCE_KEYS:
                sources[key] += _nonnegative(attribution.get(key), key)
            for key, value in attribution.get("action_experiences_by_key", {}).items():
                if not isinstance(value, dict):
                    raise ValueError(f"{path}: bad action memory entry")
                for metric in per_action[key]:
                    per_action[key][metric] += _nonnegative(value.get(metric), metric)
        points.append({
            "day": day, "resident_count": len(residents[day]),
            "snapshot_bytes": total_bytes, "memory": total,
            "sources": sources,
            "action_experiences_by_key": dict(sorted(per_action.items())),
        })

    intervals = []
    for earlier, later in zip(points, points[1:]):
        if (earlier["day"], later["day"]) not in ((100, 365), (365, 1000)):
            # Missing intermediate checkpoints must NOT be concealed.
            continue
        intervals.append({
            "from_day": earlier["day"], "to_day": later["day"],
            "memory_entries_delta": later["memory"]["entries"] - earlier["memory"]["entries"],
            "memory_bytes_delta": later["memory"]["utf8_text_bytes"] - earlier["memory"]["utf8_text_bytes"],
            "snapshot_bytes_delta": later["snapshot_bytes"] - earlier["snapshot_bytes"],
            "resident_count_delta": later["resident_count"] - earlier["resident_count"],
            "action_entries_delta": {
                action: later["action_experiences_by_key"].get(action, {}).get("entries", 0)
                        - earlier["action_experiences_by_key"].get(action, {}).get("entries", 0)
                for action in sorted(
                    earlier["action_experiences_by_key"].keys()
                    | later["action_experiences_by_key"].keys()
                )
            },
        })
    return {
        "file": str(path), "seed": seed,
        "verified_days": [point["day"] for point in points],
        "missing_days": [day for day in DAYS if day not in checkpoints],
        "checkpoints": points,
        "intervals": intervals,
        "is_complete": len(points) == len(DAYS),
        "note": "Action-memory tags and presence are NOT verified Core completions.",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audit_jsonl", nargs="+", type=Path)
    parser.add_argument("--output", type=Path,
                        help="Write JSON report, otherwise print it")
    args = parser.parse_args()
    report = {
        "schema": "lifelens-memory-checkpoint-audit-v1",
        "runs": [analyze_file(path) for path in args.audit_jsonl],
    }
    data = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(data, encoding="utf-8")
    else:
        print(data, end="")


if __name__ == "__main__":
    main()
