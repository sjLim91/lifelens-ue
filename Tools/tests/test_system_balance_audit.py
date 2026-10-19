"""Factual audit invariants, with no balance-quality thresholds."""
import importlib.util
import json
import pathlib
import tempfile
import unittest
import contextlib
import io
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location("system_balance", pathlib.Path(__file__).parents[1] / "audit_system_balance.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class DeterministicEvidenceTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        root = pathlib.Path(self.temporary.name)
        self.first, self.second = root / "first", root / "second"
        for directory in (self.first, self.second):
            (directory / "snapshots").mkdir(parents=True)
            (directory / "snapshots/day-1.llsave").write_bytes(b"same authoritative bytes")
            (directory / "events.jsonl").write_text(json.dumps({"type": "core_event", "minute": 100, "message": "same event"}) + "\n")
            self.write_metrics(directory)

    def tearDown(self):
        self.temporary.cleanup()

    def write_metrics(self, directory, complete=True, value=4, wall=10):
        records = [{"type": "checkpoint", "living": value, "elapsed_wall_ms": wall, "peak_rss_kib": wall}]
        if complete:
            records.append({"type": "complete", "invariant_failed": 0})
        (directory / "metrics.jsonl").write_text("".join(json.dumps(record) + "\n" for record in records))

    def test_wall_clock_does_not_invalidate_truth_identity(self):
        self.write_metrics(self.second, wall=300)
        self.assertTrue(AUDIT.compare(self.first, self.second)["passed"])

    def test_checkpoint_truth_difference_is_a_failure(self):
        self.write_metrics(self.second, value=3)
        self.assertFalse(AUDIT.compare(self.first, self.second)["passed"])

    def test_snapshot_byte_difference_is_a_failure(self):
        (self.second / "snapshots/day-1.llsave").write_bytes(b"different")
        self.assertFalse(AUDIT.compare(self.first, self.second)["passed"])

    def test_event_difference_is_a_failure(self):
        (self.second / "events.jsonl").write_text(json.dumps({"type": "core_event", "minute": 101, "message": "same event"}) + "\n")
        self.assertFalse(AUDIT.compare(self.first, self.second)["passed"])

    def test_identical_incomplete_runs_are_not_success(self):
        for directory in (self.first, self.second):
            self.write_metrics(directory, complete=False)
        self.assertFalse(AUDIT.compare(self.first, self.second)["passed"])

    def test_calendar_prefixed_actor_attribution_is_validated(self):
        resident = {"type": "resident", "name": "Hayun", "id": "4", "activity_minutes": {"survival": 8, "idle": 2}, "observed_minutes": 10}
        (self.first / "metrics.jsonl").write_text(json.dumps(resident) + "\n")
        event = {"type": "core_event", "sequence": "1", "message": "[Day 58 12:00] Hayun became ill", "resident": "0"}
        (self.first / "events.jsonl").write_text(json.dumps(event) + "\n")
        self.assertFalse(AUDIT.validate_evidence(self.first))
        event["resident"] = "4"
        (self.first / "events.jsonl").write_text(json.dumps(event) + "\n")
        self.assertTrue(AUDIT.validate_evidence(self.first))

    def test_zero_token_exploration_is_not_a_live_attempt(self):
        (self.first / "metrics.jsonl").write_text(json.dumps({"type": "exploration_start", "token": "0", "minute": 10}) + "\n")
        self.assertFalse(AUDIT.validate_evidence(self.first))


class MemoryAttributionReportTest(unittest.TestCase):
    def row(self, identity, entries, text_bytes):
        return {"type": "resident", "id": identity, "memory_attribution": {
            **{key: 0 for key in AUDIT.MEMORY_COUNT_FIELDS},
            "entries": entries, "utf8_text_bytes": text_bytes,
            "direct_witness_entries": entries, "exact_duplicate_entries": 1,
            "primary_tags": {"social_event": {"entries": entries, "utf8_text_bytes": text_bytes}},
        }}

    def test_repeated_entity_rows_use_last_record_not_double_count(self):
        result = AUDIT.aggregate_memory_attribution([
            self.row("1", 3, 20), self.row("2", 4, 30), self.row("1", 5, 40)])
        self.assertTrue(result["complete_coverage"])
        self.assertEqual(result["covered_residents"], 2)
        self.assertEqual(result["totals"]["entries"], 9)
        self.assertEqual(result["totals"]["utf8_text_bytes"], 70)
        self.assertEqual(result["totals"]["exact_duplicate_entries"], 2)
        self.assertEqual(result["primary_tags"]["social_event"]["entries"], 9)

    def test_missing_old_evidence_is_unavailable_and_partial_is_marked(self):
        empty = AUDIT.aggregate_memory_attribution([{"id": "1"}])
        self.assertFalse(empty["available"])
        self.assertIsNone(empty["totals"])
        partial = AUDIT.aggregate_memory_attribution([self.row("1", 0, 0), {"id": "2"}])
        self.assertTrue(partial["available"])
        self.assertFalse(partial["complete_coverage"])
        self.assertEqual(partial["totals"]["entries"], 0)
        self.assertEqual(partial["reported_residents"], 2)

    def test_summary_separates_checkpoint_days_and_emits_readable_report(self):
        with tempfile.TemporaryDirectory() as name:
            directory = pathlib.Path(name)
            records = []
            for day, count in ((1, 4), (7, 10)):
                row = {**self.row("1", count, count * 12), "day": day, "needs": {}}
                records += [row, row, {"type": "checkpoint", "day": day, "minute": day * 1440,
                    "activity_minutes": {"survival": day * 1440}, "living": 1, "total": 1,
                    "births": 0, "generation_depth": 0, "chunks": 1, "facilities": 0,
                    "snapshot_bytes": 100 + day, "memory_entries": count, "elapsed_wall_ms": day,
                    "needs": {}}]
            records.append({"type": "complete"})
            (directory / "metrics.jsonl").write_text("".join(json.dumps(row) + "\n" for row in records))
            summary = AUDIT.summarize(directory)
            self.assertEqual([row["memory_attribution"]["totals"]["entries"]
                              for row in summary["checkpoints"]], [4, 10])
            self.assertIn("UTF-8", (directory / "summary.md").read_text())
            output = directory / "job-summary.md"
            with patch.dict("os.environ", {"GITHUB_STEP_SUMMARY": str(output)}):
                with contextlib.redirect_stdout(io.StringIO()) as log:
                    AUDIT.publish_report(directory)
            self.assertIn("\n# 전 시스템", log.getvalue())
            self.assertTrue(output.read_text().startswith("\n## Audit "))


if __name__ == "__main__":
    unittest.main()
