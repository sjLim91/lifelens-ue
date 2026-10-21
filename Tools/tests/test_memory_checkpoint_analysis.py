"""Offline read-only 100/365/1000-day memory attribution report regression."""
import json
import tempfile
import unittest
from pathlib import Path

from Tools.analyze_memory_checkpoints import analyze_file, compare_independent_seeds


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

    def test_cross_seed_comparison_counts_missing_evidence(self):
        with tempfile.TemporaryDirectory() as temp:
            first = Path(temp) / "first.jsonl"
            second = Path(temp) / "second.jsonl"
            write_rows(first, [
                checkpoint(100, 1000), resident(100, 2, 1),
                checkpoint(365, 1200), resident(365, 5, 1),
                checkpoint(1000, 1800), resident(1000, 8, 2),
            ])
            write_rows(second, [
                checkpoint(100, 3000, seed="200"),
                resident(100, 6, 3, seed="200"),
                checkpoint(1000, 4000, seed="200"),
                resident(1000, 10, 5, seed="200"),
            ])
            runs = [analyze_file(first), analyze_file(second)]
            summary = compare_independent_seeds(runs)
        self.assertEqual(summary["total_independent_seeds"], 2)
        self.assertEqual(summary["complete_independent_seeds"], 1)
        self.assertEqual(summary["by_day"]["100"]["verified_seed_count"], 2)
        self.assertEqual(summary["by_day"]["100"]["memory_entries"]
                         ["mean_per_seed"], 4.0)
        self.assertEqual(summary["by_day"]["365"]["verified_seed_count"], 1)
        self.assertEqual(summary["by_day"]["365"]["missing_seeds"], ["200"])
        self.assertEqual(summary["by_day"]["365"]["snapshot_bytes"]
                         ["max_per_seed"], 1200)
        self.assertEqual(summary["by_day"]["1000"]["memory_entries"]
                         ["mean_per_seed"], 9.0)
        with self.assertRaisesRegex(ValueError, "duplicate independent seed"):
            compare_independent_seeds([runs[0], runs[0]])

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

    def test_core_summary_reconciles_all_residents_and_memory(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "audit.jsonl"
            correct = {**checkpoint(100, 3000), "total": 1,
                       "memory_entries": 4}
            write_rows(path, [correct, resident(100, 4)])
            point = analyze_file(path)["checkpoints"][0]
            self.assertEqual(point["checkpoint_reconciliation"], "verified")
            self.assertEqual(point["memory"]["entries"], 4)
            write_rows(path, [{**correct, "total": 2}, resident(100, 4)])
            with self.assertRaisesRegex(ValueError, "checkpoint residents"):
                analyze_file(path)
            write_rows(path, [{**correct, "memory_entries": 5}, resident(100, 4)])
            with self.assertRaisesRegex(ValueError, "checkpoint memories"):
                analyze_file(path)
            write_rows(path, [{**correct, "memory_entries": -1}, resident(100, 4)])
            with self.assertRaisesRegex(ValueError, "checkpoint.memory_entries"):
                analyze_file(path)
            # A legacy fixture lacking summary fields is identifiable as
            # unverifiable instead of being falsely marked reconciled.
            write_rows(path, [checkpoint(100, 3000), resident(100, 4)])
            self.assertEqual(analyze_file(path)["checkpoints"][0]
                             ["checkpoint_reconciliation"],
                             "summary_fields_missing")

    def test_resident_memory_partitions_fail_closed(self):
        # The Core snapshot total alone cannot detect corrupt attribution
        # buckets: the three sources and two action partitions must agree.
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "audit.jsonl"
            base = resident(100, 4, 2)

            def altered(change):
                item = json.loads(json.dumps(base))
                change(item["memory_attribution"])
                return item

            cases = [
                ("source partition", lambda a: a.update(direct_witness_entries=3),
                 "memory source partition mismatch"),
                ("witness claim", lambda a: a.update(witnessed_direct_entries=5),
                 "witnessed direct exceeds source count"),
                ("duplicate overflow", lambda a: a.update(exact_duplicate_entries=5),
                 "exact duplicates exceed memory entries"),
                ("action outcome", lambda a: a["action_experiences_by_key"]
                 ["CoreContext:Trade"].update(successful=0),
                 "action status partition mismatch"),
                ("action provenance", lambda a: a["action_experiences_by_key"]
                 ["CoreContext:Trade"].update(indirect_or_unverified=2),
                 "action provenance partition mismatch"),
                ("bad action map", lambda a: a.update(action_experiences_by_key=[]),
                 "malformed action memory map"),
                ("action overflow", lambda a: a["action_experiences_by_key"].update({
                     "Other:Work": {"entries": 3, "successful": 3, "failed": 0,
                                    "witnessed_direct": 3,
                                    "indirect_or_unverified": 0,
                                    "utf8_text_bytes": 30}}),
                 "action entries exceed memory total"),
            ]
            for name, mutate, message in cases:
                with self.subTest(name=name):
                    write_rows(path, [checkpoint(100, 1000), altered(mutate)])
                    with self.assertRaisesRegex(ValueError, message):
                        analyze_file(path)
            # Legitimate read-only reports still preserve measured totals.
            write_rows(path, [checkpoint(100, 1000), base])
            point = analyze_file(path)["checkpoints"][0]
            self.assertEqual(point["sources"]["direct_witness_entries"], 4)
            self.assertEqual(point["action_experiences_by_key"]
                             ["CoreContext:Trade"]["entries"], 2)

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
