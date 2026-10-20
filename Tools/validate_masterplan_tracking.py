"""Fail-closed structural checks for the canonical 86-unit LifeLens roadmap ledger.

This checks document consistency only, not actual GitHub CI/merge evidence.
"""
from collections import Counter
from pathlib import Path
import re

PLAN_ROW = re.compile(r"^\|\s*(M\d+-\d+)\s*\|")
TRACK_ROW = re.compile(r"^\|\s*\[([ x])\]\s*\|\s*\*\*(M\d+-\d+)\*\*\s*\|")
DONE = "✅ 완료"
IN_REVIEW = "🔎 병합 검토"
IN_PROGRESS = "🚧 진행"
NOT_STARTED = "⬜ 미완"
BLOCKED = "⚠️ 차단"
VALID_STATES = {DONE, IN_REVIEW, IN_PROGRESS, NOT_STARTED, BLOCKED}
PR_BASE = "https://github.com/sjLim91/lifelens-ue/pull/"


def _single_count(text, pattern, label):
    found = re.findall(pattern, text, flags=re.MULTILINE)
    if len(found) != 1:
        raise ValueError(f"{label}: expected one summary, got {len(found)}")
    return int(found[0])


def verify_masterplan_ledger(masterplan: str, progress: str):
    plan_ids = [match.group(1) for line in masterplan.splitlines()
                if (match := PLAN_ROW.match(line))]
    if not plan_ids or len(plan_ids) != len(set(plan_ids)):
        raise ValueError("masterplan units empty or duplicated")
    rows = {}
    for line in progress.splitlines():
        match = TRACK_ROW.match(line)
        if not match:
            continue
        checked, unit = match.groups()
        if unit in rows:
            raise ValueError(f"duplicate tracked unit: {unit}")
        columns = line.strip().strip("|").split("|")
        if len(columns) < 5:
            raise ValueError(f"malformed tracked unit: {unit}")
        state, evidence = columns[-2].strip(), columns[-1].strip()
        if state not in VALID_STATES:
            raise ValueError(f"unrecognized state for {unit}: {state}")
        if (checked == "x") != (state == DONE):
            raise ValueError(f"completion checkbox and status disagree: {unit}")
        if checked == "x" and (PR_BASE not in evidence or "main" not in evidence):
            raise ValueError(f"completed unit has no main/PR citation: {unit}")
        rows[unit] = state

    missing = sorted(set(plan_ids) - set(rows))
    extra = sorted(set(rows) - set(plan_ids))
    if missing or extra:
        raise ValueError(f"roadmap/tracker drift: missing={missing} extra={extra}")

    totals = Counter(rows.values())
    actual = {
        "complete": totals[DONE],
        "review": totals[IN_REVIEW],
        "in_progress": totals[IN_PROGRESS],
        "unfinished": totals[NOT_STARTED] + totals[BLOCKED],
        "total": len(plan_ids),
    }
    expected = {
        "complete": _single_count(progress, r"^- 완료:\s*\*\*(\d+)/\d+\*\*", "complete"),
        "review": _single_count(progress, r"^- 검증 완료·병합 검토:\s*\*\*(\d+)\*\*", "review"),
        "in_progress": _single_count(progress, r"^- 진행:\s*\*\*(\d+)\*\*", "in progress"),
        "unfinished": _single_count(progress, r"^- 미완 또는 미검증:\s*\*\*(\d+)\*\*", "unfinished"),
        "total": _single_count(progress, r"^- 완료:\s*\*\*\d+/(\d+)\*\*", "total"),
    }
    if actual != expected:
        raise ValueError(f"roadmap summary mismatch: actual={actual} declared={expected}")
    return actual


def main():
    root = Path(__file__).resolve().parents[1]
    plan = (root / "docs/AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md").read_text(encoding="utf-8")
    progress = (root / "docs/AUTONOMOUS_ARTIFICIAL_LIFE_PROGRESS.md").read_text(encoding="utf-8")
    counts = verify_masterplan_ledger(plan, progress)
    print(f"LifeLens roadmap tracking PASS: {counts}")


if __name__ == "__main__":
    main()
