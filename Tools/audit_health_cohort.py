#!/usr/bin/env python3
"""Read-only per-resident illness/cohort analysis of completed long-run ZIPs.

A zero illness-event count is "no recorded onset", not proof of zero pathogen
exposure. A successful workflow is not a guarantee of healthy population.
"""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import re
from zipfile import ZipFile

ONSET = re.compile(r"\bbecame ill after accumulated pathogen exposure\b")
RECOVERY = re.compile(r"\brecovered from illness and gained resilience\b")
DEATH = re.compile(r"\bdied from (illness|severe deprivation|deprivation|accident|exposure)\b", re.I)


def analyze_cohort(archive_path):
    with ZipFile(archive_path) as archive:
        roots = sorted({
            name.split("/")[0] for name in archive.namelist()
            if name.endswith("/summary.json") and "-repeat/" not in name
        })
        if len(roots) != 1:
            raise ValueError(f"Expected a single completed seed, got {roots}")
        root = roots[0]
        summary = json.loads(archive.read(f"{root}/summary.json"))
        validation = json.loads(archive.read(f"{root}/evidence_validation.json"))
        if not summary.get("complete") or not validation.get("passed"):
            raise ValueError(f"Incomplete or invalid audit artifact for {root}")

        residents = defaultdict(lambda: {
            "illness_onsets": 0, "recoveries": 0, "fatal_cause": None,
            "onset_while_episode_open": 0, "recovery_without_open_episode": 0,
            "health_samples": 0, "max_pathogen": None, "max_illness": None,
            "max_local_exposure": None, "sum_local_exposure": 0.0,
            "max_water_dose": None, "sum_water_dose": 0.0,
        })
        open_episode = defaultdict(bool)
        for line in archive.open(f"{root}/events.jsonl"):
            event = json.loads(line)
            resident = str(event.get("resident", ""))
            if not resident or resident == "0":
                continue
            message = event.get("message", "")
            if ONSET.search(message):
                residents[resident]["illness_onsets"] += 1
                if open_episode[resident]:
                    residents[resident]["onset_while_episode_open"] += 1
                open_episode[resident] = True
            if RECOVERY.search(message):
                residents[resident]["recoveries"] += 1
                if not open_episode[resident]:
                    residents[resident]["recovery_without_open_episode"] += 1
                open_episode[resident] = False
            fatal = DEATH.search(message)
            if fatal:
                residents[resident]["fatal_cause"] = fatal.group(1).lower()

        for line in archive.open(f"{root}/metrics.jsonl"):
            metric = json.loads(line)
            if metric.get("type") != "health_transition":
                continue
            resident = str(metric.get("resident", ""))
            if not resident or resident == "0":
                continue
            data = residents[resident]
            data["health_samples"] += 1
            # New audit artifacts carry sampled physical coordinates and the
            # largest nearby waste contribution; old ZIPs omit these keys.
            if "position_x" in metric and "position_y" in metric:
                data["last_position"] = {
                    "x": int(metric["position_x"]),
                    "y": int(metric["position_y"]),
                }
            if "top_residue_id" in metric:
                data["last_top_residue"] = {
                    "id": str(metric["top_residue_id"]),
                    "source_character": str(metric.get("top_residue_source", "0")),
                    "distance_tiles": metric.get("top_residue_distance"),
                    "contribution": metric.get("top_residue_contribution"),
                    "amount": metric.get("top_residue_amount"),
                }
            for metric_name, destination in [
                ("after_pathogen", "max_pathogen"),
                ("after_illness", "max_illness"),
                ("local_residue_exposure_after_tick", "max_local_exposure"),
                ("pending_water_dose_before_tick", "max_water_dose"),
            ]:
                value = metric.get(metric_name)
                if value is not None:
                    number = float(value)
                    old = data[destination]
                    data[destination] = number if old is None else max(old, number)
            data["sum_local_exposure"] += float(
                metric.get("local_residue_exposure_after_tick") or 0.0)
            data["sum_water_dose"] += float(
                metric.get("pending_water_dose_before_tick") or 0.0)

        def sort_key(identity):
            return (0, int(identity)) if identity.isdecimal() else (1, identity)

        observations = []
        for resident in sorted(residents, key=sort_key):
            row = dict(residents[resident])
            count = row.pop("health_samples")
            row["resident"] = resident
            row["daily_health_samples"] = count
            row["mean_local_exposure"] = (
                round(row.pop("sum_local_exposure") / count, 6)
                if count else None)
            row["total_sampled_water_dose"] = round(
                row.pop("sum_water_dose"), 6)
            row["episode_open_at_end"] = open_episode[resident]
            observations.append(row)

        with_onset = sum(x["illness_onsets"] > 0 for x in observations)
        with_recovery = sum(x["recoveries"] > 0 for x in observations)
        with_death = sum(x["fatal_cause"] is not None for x in observations)
        return {
            "seed": root,
            "artifact_verified": True,
            "final": summary["checkpoints"][-1],
            "cohort": {
                "residents_with_health_or_events": len(observations),
                "residents_with_recorded_illness_onset": with_onset,
                "residents_with_recorded_recovery": with_recovery,
                "residents_with_fatal_event": with_death,
                "residents_without_recorded_illness_onset":
                    len(observations) - with_onset,
                "duplicated_onset_before_recovery": sum(
                    x["onset_while_episode_open"] for x in observations),
                "recovery_without_recorded_onset": sum(
                    x["recovery_without_open_episode"] for x in observations),
            },
            "residents": observations,
            "limitations": [
                "No recorded onset does not prove a resident was never exposed.",
                "Daily sampled contamination is not a per-source causal attribution.",
                "Birth event resident=0 cannot identify a child without additional linkage.",
                "Simulation quality cannot be inferred from CI success alone.",
            ],
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifacts", nargs="+", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    runs = [analyze_cohort(path) for path in args.artifacts]
    content = json.dumps({"schema_version": 1, "read_only": True, "runs": runs},
                         ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(content, encoding="utf-8")
    else:
        print(content, end="")


if __name__ == "__main__":
    main()
