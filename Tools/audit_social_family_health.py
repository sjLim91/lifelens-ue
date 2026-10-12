#!/usr/bin/env python3
"""Read-only summary of birth, death, care and pathogen signals in long-run ZIPs.

No simulation logic, save file, thresholds or simulation time is modified.
GitHub Actions green means execution success, not survival-quality approval.
"""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import re
from zipfile import ZipFile

BIRTH = re.compile(r"\bbirth: (.+?) child of (.+)$")
DEATH = re.compile(r"\b(.+?) died from (illness|severe deprivation|deprivation|accident|exposure)\b", re.I)
ASSIST = re.compile(r"\b(.+?) left sanitation residue id=(\d+) during assisted toilet care")


def analyze_artifact(archive_path, seed=None):
    with ZipFile(archive_path) as archive:
        roots = sorted({
            name.split("/")[0] for name in archive.namelist()
            if name.endswith("/summary.json") and "-repeat/" not in name
        })
        if seed is not None:
            roots = [root for root in roots if root == str(seed)]
        if len(roots) != 1:
            raise ValueError(f"Expected one seed in artifact, found {roots}")
        root = roots[0]

        def load(name):
            return json.loads(archive.read(f"{root}/{name}"))

        summary = load("summary.json")
        validation = load("evidence_validation.json")
        if not summary.get("complete") or not validation.get("passed"):
            raise ValueError(f"Audit artifact is incomplete or invalid for {root}")

        births = []
        deaths = []
        assist = []
        for line in archive.open(f"{root}/events.jsonl"):
            event = json.loads(line)
            message = event.get("message", "")
            match = BIRTH.search(message)
            if match:
                births.append({"name": match.group(1), "parents": match.group(2),
                               "elapsed_day": event["elapsed_day"]})
            match = DEATH.search(message)
            if match:
                deaths.append({"name": match.group(1).split("] ")[-1],
                               "cause": match.group(2).lower(),
                               "resident": str(event.get("resident", "")),
                               "elapsed_day": event["elapsed_day"]})
            match = ASSIST.search(message)
            if match:
                assist.append({"name": match.group(1).split("] ")[-1],
                               "residue_id": match.group(2),
                               "site": "via sanitation site" in message})

        health = defaultdict(list)
        death_ids = {row["resident"] for row in deaths}
        for line in archive.open(f"{root}/metrics.jsonl"):
            row = json.loads(line)
            if row.get("type") != "health_transition":
                continue
            resident = str(row.get("resident", ""))
            if resident not in death_ids:
                continue
            health[resident].append({
                "elapsed_day": row["elapsed_day"],
                "pathogen": row.get("after_pathogen"),
                "illness": row.get("after_illness"),
                "local_exposure": row.get("local_residue_exposure_after_tick"),
                "water_dose": row.get("pending_water_dose_before_tick"),
            })

    birth_by_name = {row["name"]: row for row in births}
    newborn_deaths, other_deaths = [], []
    for death in deaths:
        matching_birth = birth_by_name.get(death["name"])
        daily = health[death["resident"]]
        exposures = [float(v["local_exposure"] or 0) for v in daily]
        entry = dict(death)
        entry.update({
            "age_at_death_days": round(death["elapsed_day"] - matching_birth["elapsed_day"], 3)
                                 if matching_birth else None,
            "health_daily_samples": len(daily),
            "first_health": daily[0] if daily else None,
            "last_health": daily[-1] if daily else None,
            "mean_local_exposure": round(sum(exposures) / len(exposures), 6)
                                   if exposures else None,
            "max_local_exposure": max(exposures) if exposures else None,
            "toilet_assist_at_site": sum(
                row["name"] == death["name"] and row["site"] for row in assist),
        })
        (newborn_deaths if matching_birth else other_deaths).append(entry)

    latest = summary["checkpoints"][-1]
    keys = ("elapsed_day", "total", "living", "deceased", "births", "pregnancies",
            "married", "households", "facilities", "chunks", "active_settlements")
    return {
        "seed": root,
        "artifact_verified": True,
        "final": {key: latest.get(key) for key in keys},
        "checkpoints": [{key: row.get(key) for key in keys}
                        for row in summary["checkpoints"]],
        "births": births,
        "deaths_of_births_in_run": newborn_deaths,
        "other_deaths": other_deaths,
        "surviving_births_in_run": len(births) - len(newborn_deaths),
        "toilet_assist_count": len(assist),
        "toilet_assist_site_count": sum(row["site"] for row in assist),
        "distinct_assist_residue_ids": len({row["residue_id"] for row in assist}),
        "limits": ("Disease exposure is observed, but causal source attribution "
                   "requires physical position, route, and residue provenance. "
                   "CI success does not mean that residents survived."),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifacts", type=Path, nargs="+")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = {"schema_version": 1, "read_only": True,
              "runs": [analyze_artifact(path) for path in args.artifacts]}
    contents = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(contents, encoding="utf-8")
    else:
        print(contents, end="")


if __name__ == "__main__":
    main()
