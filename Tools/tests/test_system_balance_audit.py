"""Factual audit invariants, with no balance-quality thresholds."""
import importlib.util
import json
import pathlib
import tempfile
import unittest

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



class MemoryProvenanceSummaryTest(unittest.TestCase):
    @staticmethod
    def memory_row(identity, entries):
        return {"id": identity, "memory_attribution": {
            "entries": entries, "utf8_text_bytes": entries * 9,
            "exact_duplicate_entries": 0, "direct_witness_entries": entries,
            "told_by_other_entries": 0, "inferred_entries": 0,
            "primary_tags": {"social_fact:direct": {
                "entries": entries, "utf8_text_bytes": entries * 9}},
        }}

    def test_last_row_wins_without_summing_same_resident_twice(self):
        data = AUDIT.aggregate_checkpoint_memory([
            self.memory_row("1", 5), self.memory_row("2", 4),
            self.memory_row("1", 7),
        ])
        self.assertTrue(data["complete_coverage"])
        self.assertEqual(data["totals"]["entries"], 11)
        self.assertEqual(data["covered_residents"], 2)
        self.assertEqual(data["dominant_categories"][0]["entries"], 11)

    def test_missing_is_never_zero_or_complete(self):
        unavailable = AUDIT.aggregate_checkpoint_memory([{"id": "1"}])
        self.assertIsNone(unavailable["totals"])
        self.assertFalse(unavailable["complete_coverage"])
        partial = AUDIT.aggregate_checkpoint_memory([
            self.memory_row("1", 0), {"id": "2"}])
        self.assertFalse(partial["complete_coverage"])
        self.assertEqual(partial["covered_residents"], 1)
        self.assertEqual(partial["reported_residents"], 2)

    def test_untrusted_counters_rejected(self):
        bad = self.memory_row("1", 3)
        bad["memory_attribution"]["entries"] = True
        with self.assertRaises(ValueError):
            AUDIT.aggregate_checkpoint_memory([bad])
        bad = self.memory_row("1", 3)
        bad["memory_attribution"]["primary_tags"]["social_fact:direct"]["utf8_text_bytes"] = -1
        with self.assertRaises(ValueError):
            AUDIT.aggregate_checkpoint_memory([bad])


if __name__ == "__main__":
    unittest.main()
