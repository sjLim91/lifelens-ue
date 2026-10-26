"""M0-04 closeout: reject missing, unverified or mixed resident snapshots."""
import json
import tempfile
import unittest
from pathlib import Path

from Tools.m0_memory_closeout import closeout


def record(day, seed, entries, snapshot):
    cp = {"type": "checkpoint", "seed": seed, "day": day,
          "total": 1, "memory_entries": entries, "snapshot_bytes": snapshot}
    resident = {
        "type": "resident", "seed": seed, "day": day, "id": "1",
        "memory_attribution": {
            "entries": entries, "utf8_text_bytes": entries * 9,
            "exact_duplicate_entries": max(0, entries - 5),
            "witnessed_direct_entries": entries,
            "direct_witness_entries": entries,
            "told_by_other_entries": 0, "inferred_entries": 0,
            "primary_tags": {"social_fact:direct": {
                "entries": entries, "utf8_text_bytes": entries * 9}},
            "future_entries": 0, "age_lt_1d_entries": entries,
            "age_1to7d_entries": 0, "age_7to30d_entries": 0,
            "age_30to365d_entries": 0, "age_365d_plus_entries": 0,
            "action_experiences_by_key": {"Civilization:Gather": {
                "entries": min(2, entries),
                "successful": min(2, entries),
                "failed": 0, "witnessed_direct": min(2, entries),
                "indirect_or_unverified": 0,
                "utf8_text_bytes": min(2, entries) * 9}},
        }}
    return [cp, resident]


class TestM0MemoryCloseout(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)

    def write(self, seed, days=(100, 365, 1000)):
        path = self.path / f"{seed}.jsonl"
        rows = []
        for index, day in enumerate(days):
            rows += record(day, str(seed), 2 + index * 3, 100 + index * 210)
        path.write_text("".join(json.dumps(row) + "\n" for row in rows),
                        encoding="utf-8")
        return path

    def test_two_actual_seeds_full_horizon(self):
        report = closeout([self.write(11), self.write(22)])
        self.assertEqual(report["verifiedSeedCount"], 2)
        self.assertEqual(report["verifiedSeeds"], ["11", "22"])
        self.assertEqual(len(report["analysis"][0]["growthIntervals"]), 2)
        self.assertEqual(report["analysis"][0]["growthIntervals"][0]
                         ["sourceEntriesDelta"]["direct_witness_entries"], 3)
        self.assertEqual(report["analysis"][0]["growthIntervals"][0]
                         ["otherMemoryEntriesDelta"], 3)
        self.assertEqual(report["analysis"][0]["growthIntervals"][0]
                         ["snapshotBytesDelta"], 210)

    def test_missing_day_rejected(self):
        with self.assertRaisesRegex(ValueError, "missing 100/365/1000"):
            closeout([self.write(11), self.write(22, (100, 1000))])

    def test_single_seed_or_duplicate_rejected(self):
        a = self.write(11)
        with self.assertRaisesRegex(ValueError, "at least two"):
            closeout([a])
        with self.assertRaisesRegex(ValueError, "duplicate independent seed"):
            closeout([a, a])

    def test_native_tag_bytes_and_age_cannot_be_fabricated(self):
        a, b = self.write(11), self.write(22)
        data = [json.loads(line) for line in b.read_text().splitlines()]
        group = data[1]["memory_attribution"]
        group["primary_tags"]["social_fact:direct"]["utf8_text_bytes"] += 1000
        b.write_text("".join(json.dumps(row) + "\n" for row in data))
        with self.assertRaisesRegex(ValueError, "primary tag text byte mismatch"):
            closeout([a, b])
        group["primary_tags"]["social_fact:direct"]["utf8_text_bytes"] -= 1000
        group["age_lt_1d_entries"] = 0
        b.write_text("".join(json.dumps(row) + "\n" for row in data))
        with self.assertRaisesRegex(ValueError, "age and future partition mismatch"):
            closeout([a, b])

    def test_unreconciled_summary_rejected(self):
        a, b = self.write(11), self.write(22)
        data = [json.loads(line) for line in b.read_text().splitlines()]
        del data[0]["total"]
        b.write_text("".join(json.dumps(row) + "\n" for row in data))
        with self.assertRaisesRegex(ValueError, "independently reconciled"):
            closeout([a, b])

    def test_unsafe_direct_attribution_or_truncated_snapshot_rejected(self):
        a, b = self.write(11), self.write(22)
        data = [json.loads(line) for line in b.read_text().splitlines()]
        data[1]["memory_attribution"]["inferred_entries"] = 1
        b.write_text("".join(json.dumps(row) + "\n" for row in data))
        with self.assertRaisesRegex(ValueError, "source partition mismatch"):
            closeout([a, b])
        data[1]["memory_attribution"]["inferred_entries"] = 0
        data[0]["total"] = 2
        b.write_text("".join(json.dumps(row) + "\n" for row in data))
        with self.assertRaisesRegex(ValueError, "checkpoint residents"):
            closeout([a, b])


if __name__ == "__main__":
    unittest.main()
