#!/usr/bin/env python3
"""Run/resume the existing ll_balance_audit and export evidence, never tune Core.

Resume skips only completed, matching invocations. Simulation-only loaded saves
cannot reconstruct historical audit accumulators, so interrupted runs restart.
"""
import argparse
import csv
import hashlib
import json
import pathlib
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

CHECKPOINTS = "1,7,30,100,365,1000"
PERFORMANCE_FIELDS = {"elapsed_wall_ms", "peak_rss_kib"}


def read_jsonl(path):
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            try:
                yield json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"{path}:{line_number}: {error}") from error


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def atomic_json(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def flatten(value, prefix=""):
    for name, child in sorted(value.items()):
        key = f"{prefix}.{name}" if prefix else name
        if isinstance(child, dict):
            yield from flatten(child, key)
        else:
            yield key, child


def normalized_record(record):
    return {key: value for key, value in record.items() if key not in PERFORMANCE_FIELDS}


def compare(first, second):
    """Full streamed metric/event comparison plus byte-identical save files."""
    from itertools import zip_longest
    issues = []
    for filename in ("metrics.jsonl", "events.jsonl"):
        for index, (a, b) in enumerate(zip_longest(read_jsonl(first / filename), read_jsonl(second / filename)), 1):
            if a is None or b is None or normalized_record(a) != normalized_record(b):
                issues.append({"file": filename, "record": index, "first": a, "second": b})
                break
    saves = {}
    a_files = {p.name: p for p in (first / "snapshots").glob("*.llsave")}
    b_files = {p.name: p for p in (second / "snapshots").glob("*.llsave")}
    if not a_files or a_files.keys() != b_files.keys():
        issues.append({"error": "snapshot checkpoint set missing or unequal"})
    for name in sorted(a_files.keys() & b_files.keys()):
        a, b = digest(a_files[name]), digest(b_files[name])
        saves[name] = {"sha256": a, "identical": a == b}
        if a != b:
            issues.append({"file": name, "first_sha256": a, "second_sha256": b})
    result = {"passed": not issues, "excluded_nondeterministic_fields": sorted(PERFORMANCE_FIELDS), "snapshots": saves, "issues": issues}
    atomic_json(first.parent / "determinism.json", result)
    return result


def percentages(checkpoint):
    budget = checkpoint["activity_minutes"]
    total = sum(budget.values())
    def percent(names):
        return round(100 * sum(budget.get(name, 0) for name in names) / total, 4) if total else None
    return {
        "survival": percent(["survival"]), "social": percent(["social"]),
        "family": percent(["family", "parenting", "health_care"]),
        "civilization": percent(["gather", "storage_logistics", "construction", "repair", "production", "cultivation", "learning", "teaching", "trade", "exploration"]),
        "idle": percent(["idle"]), "failed_retry": percent(["failed_retry"]),
    }


def summarize(directory):
    checkpoints, first_signals, invariants, completions = [], [], [], []
    latest = {}
    with (directory / "metrics.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["seed", "day", "minute", "scope", "entity", "metric", "value"])
        for record in read_jsonl(directory / "metrics.jsonl"):
            entity = record.get("id", record.get("resident", record.get("material", "world")))
            for metric, value in flatten(record):
                writer.writerow([record.get("seed"), record.get("day"), record.get("minute"), record["type"], entity, metric,
                                 "unavailable" if value is None else value])
            if record["type"] == "checkpoint":
                checkpoints.append({**record, "activity_percent": percentages(record)})
            if record["type"] in ("first_need_pressure", "invariant_failure"):
                first_signals.append(record)
            if record["type"] == "invariant_failure":
                invariants.append(record)
            if record["type"] == "complete":
                completions.append(record)
            if "day" in record:
                key = (record["type"], str(entity), record.get("technique", record.get("need", "")))
                latest[key] = record
    summary = {
        "schema_version": 1, "checkpoints": checkpoints, "first_signals": first_signals,
        "invariants": invariants, "complete": bool(completions), "latest_entities": list(latest.values()),
        "measurement_limits": {
            "critical_band": "0.90 is Core critical provision threshold; other Needs use same observational band, not an AI threshold change",
            "activity": "exclusive post-tick Core presentation category; independent romance/family/institution/economy actions are not separable in this read model",
            "failed_retry": "inactive minutes inside Core penaltyUntilMinute; excludes failed work that still has active presentation",
            "zero_stock": "living carried + global stored raw material; natural direct water, containers and dead inventory excluded; zero does not prove shortage",
            "resource_flows": "gross production/consumption/spoilage/trade flows unavailable; inventory deltas cannot identify all causes",
            "natural": "all materialized natural quantity; accessibility/reachability not asserted",
            "knowledge_use": "subsequent successfulUses increments; Core initial discovery also increments the counter; not proof of a distinct product outcome",
            "survival_days": "right-censored observation duration; founders counted from invocation start, newborns from birth",
            "generations": "Adult requires 18*365=6570 days after birth; 1000 days cannot validate adult descendant reproduction",
            "settlement_storage": "service-area membership can overlap; do not sum across settlements",
            "causality": "timeline order is evidence; causal attribution also requires the relevant Core implementation",
        },
    }
    atomic_json(directory / "summary.json", summary)
    lines = ["# 전 시스템 장기 계측 요약", "", "Core 수치 변경 없이 관찰한 결과. 인과 판정에는 events.jsonl과 Core 코드가 함께 필요하다.", "",
             "| day | living/total | births | generation depth | chunks | facilities | survival % | social % | family % | civilization % | idle % | retry % |",
             "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for c in checkpoints:
        p = c["activity_percent"]
        lines.append(f"| {c['day']} | {c['living']}/{c['total']} | {c['births']} | {c['generation_depth']} | {c['chunks']} | {c['facilities']} | " +
                     " | ".join(str(p[key]) for key in ("survival", "social", "family", "civilization", "idle", "failed_retry")) + " |")
    lines += ["", "## 관측 한계", ""]
    lines += [f"- `{name}`: {value}" for name, value in summary["measurement_limits"].items()]
    (directory / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return summary


def run_one(executable, directory, seed, days, checkpoints, resume):
    directory.mkdir(parents=True, exist_ok=True)
    manifest_path = directory / "run.json"
    identity = {"binary_sha256": digest(executable), "seed": str(seed), "days": days, "checkpoints": checkpoints}
    if resume and manifest_path.exists():
        prior = json.loads(manifest_path.read_text())
        if prior.get("identity") == identity and prior.get("status") == "complete":
            summary = summarize(directory)
            if summary["complete"]:
                print(f"RESUME_SKIP {seed} {directory}", flush=True)
                return prior
    state = {"identity": identity, "status": "running", "started_utc_epoch": time.time()}
    atomic_json(manifest_path, state)
    command = [str(executable), "--seed", str(seed), "--days", str(days), "--checkpoints", checkpoints,
               "--audit-directory", str(directory), "--snapshot-directory", str(directory / "snapshots")]
    print(f"START {seed} {directory}", flush=True)
    try:
        with (directory / "core.log").open("w", encoding="utf-8") as log:
            child = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
            state["pid"] = child.pid
            atomic_json(manifest_path, state)
            code = child.wait()
        state.update(returncode=code, runtime_seconds=time.time() - state["started_utc_epoch"])
        summary = summarize(directory)
        expected = sorted({int(c) for c in checkpoints.split(",") if 0 < int(c) <= days} | {days})
        actual = [c["day"] for c in summary["checkpoints"]]
        state["status"] = "complete" if code == 0 and summary["complete"] and actual == expected else "failed"
        state["snapshot_sha256"] = {p.name: digest(p) for p in (directory / "snapshots").glob("*.llsave")}
    except BaseException:
        state["status"] = "interrupted"
        atomic_json(manifest_path, state)
        raise
    atomic_json(manifest_path, state)
    print(f"FINISH {seed} {state['status']} {state['runtime_seconds']:.2f}s", flush=True)
    return state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    run = sub.add_parser("run")
    run.add_argument("--executable", type=pathlib.Path, required=True)
    run.add_argument("--output", type=pathlib.Path, required=True)
    run.add_argument("--seeds", default="874213954,4242001")
    run.add_argument("--days", type=int, default=1000)
    run.add_argument("--checkpoints", default=CHECKPOINTS)
    run.add_argument("--workers", type=int, default=2)
    run.add_argument("--resume", action="store_true")
    run.add_argument("--determinism-seed", default="874213954")
    summary = sub.add_parser("summarize")
    summary.add_argument("directory", type=pathlib.Path)
    comparison = sub.add_parser("compare")
    comparison.add_argument("first", type=pathlib.Path)
    comparison.add_argument("second", type=pathlib.Path)
    args = parser.parse_args()
    if args.command == "summarize":
        summarize(args.directory)
        return 0
    if args.command == "compare":
        return 0 if compare(args.first, args.second)["passed"] else 2
    if args.days <= 0 or args.workers <= 0:
        parser.error("days and workers must be positive")
    args.executable = args.executable.resolve()
    args.output = args.output.resolve()
    seeds = list(dict.fromkeys(args.seeds.split(",")))
    if args.determinism_seed and args.determinism_seed not in seeds:
        parser.error("determinism seed must be one of the run seeds; use an empty string to disable")
    requests = [(seed, args.output / seed) for seed in seeds]
    if args.determinism_seed:
        requests.append((args.determinism_seed, args.output / f"{args.determinism_seed}-repeat"))
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        states = list(pool.map(lambda request: run_one(args.executable, request[1], request[0], args.days, args.checkpoints, args.resume), requests))
    passed = all(state["status"] == "complete" for state in states)
    if passed and args.determinism_seed:
        passed = compare(args.output / args.determinism_seed, args.output / f"{args.determinism_seed}-repeat")["passed"]
    atomic_json(args.output / "run_manifest.json", {"passed": passed, "runs": states})
    return 0 if passed else 2


if __name__ == "__main__":
    sys.exit(main())
