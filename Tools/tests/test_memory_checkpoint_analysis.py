"""Offline read-only 100/365/1000-day memory attribution report regression."""
import json
import tempfile
import unittest
from pathlib import Path

from Tools.analyze_memory_checkpoints import analyze_file


def checkpoint(day, size, seed="99"):
    return {"type": "checkpoint", "seed": seed, "day": day, "snapshot_bytes": size}


def resident(day, entries, action_count=0, seed="99"):
    return {
        "type": "resident", "seed": seed, "day": day, "id": "1",
        "memory_attribution": {
            "entries": entries, "utf8_text_bytes": entries * 10,
            "exact_duplicate_entries": 0, "witnessed_direct_entries": entries,
            "direct_witness_entries": entries, "told_by_other_entries": 0,
            "inferred_entries": 0, "action_experiences_by_key": {
                "CoreContext:Trade": {
                    "entries": action_count, "successful": action_count,
                    "failed": 0, "witnessed_direct": action_count,
                    "indirect_or_unverified": 0,
                    "utf8_text_bytes": action_count * 10,
                },
            },
        },
    }


def write_rows(path, rows):
    path.write_text("\n".join(json.dumps(x) for x in rows) + "\n",
                    encoding="utf-8")


class TestMemoryCheckpointAnalysis(unittest.TestCase):
    def test_three_real_checkpoints_produce_two_intervals(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "audit.jsonl"
            write_rows(path, [
                checkpoint(100, 1200), resident(100, 3, 1),
                checkpoint(365, 2300), resident(365, 6, 3),
                checkpoint(1000, 4000), resident(1000, 10, 4),
            ])
            report = analyze_file(path)
        self.assertEqual(report["seed"], "99")
        self.assertEqual(report["verified_days"], [100, 365, 1000])
        self.assertEqual(report["missing_days"], [])
        self.assertTrue(report["is_complete"])
        self.assertEqual(len(report["intervals"]), 2)
        self.assertEqual(report["intervals"][0]["memory_entries_delta"], 3)
        self.assertEqual(report["intervals"][0]["snapshot_bytes_delta"], 1100)
        self.assertEqual(report["intervals"][0]["action_entries_delta"]
                         ["CoreContext:Trade"], 2)
        self.assertEqual(report["intervals"][1]["snapshot_bytes_delta"], 1700)

    def test_no_invented_middle_checkpoint(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "audit.jsonl"
            write_rows(path, [
                checkpoint(100, 1200), resident(100, 1),
                checkpoint(1000, 4000), resident(1000, 3),
            ])
            report = analyze_file(path)
        self.assertEqual(report["missing_days"], [365])
        self.assertEqual(report["intervals"], [])
        self.assertFalse(report["is_complete"])

    def test_mixed_seed_or_duplicate_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "audit.jsonl"
            write_rows(path, [checkpoint(100, 1200), resident(100, 1, seed="other")])
            with self.assertRaisesRegex(ValueError, "mixed simulation seeds"):
                analyze_file(path)
            write_rows(path, [
                checkpoint(100, 1200), checkpoint(100, 1200),
                resident(100, 1),
            ])
            with self.assertRaisesRegex(ValueError, "duplicate checkpoint"):
                analyze_file(path)

    def test_missing_real_snapshot_never_accepted(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "audit.jsonl"
            write_rows(path, [
                {"type": "checkpoint", "seed": "99", "day": 365},
                resident(365, 1),
            ])
            with self.assertRaisesRegex(ValueError, "snapshot bytes"):
                analyze_file(path)


if __name__ == "__main__":
    unittest.main()
