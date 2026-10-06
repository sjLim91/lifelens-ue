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


if __name__ == "__main__":
    unittest.main()
