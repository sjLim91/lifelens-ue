import json
from pathlib import Path
import sys
import tempfile
import unittest
from zipfile import ZipFile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from audit_health_cohort import analyze_cohort


class HealthCohortTests(unittest.TestCase):
    def write_archive(self, directory, complete=True):
        path = Path(directory) / "health.zip"
        events = [
            {"resident": "1", "message": "A became ill after accumulated pathogen exposure"},
            {"resident": "1", "message": "A became ill after accumulated pathogen exposure"},
            {"resident": "1", "message": "A recovered from illness and gained resilience"},
            {"resident": "1", "message": "A became ill after accumulated pathogen exposure"},
            {"resident": "1", "message": "A recovered from illness and gained resilience"},
            {"resident": "2", "message": "B died from illness"},
            {"resident": "0", "message": "birth: C child of A and B"},
        ]
        metrics = [
            {"type": "health_transition", "resident": "1",
             "after_pathogen": 0.7, "after_illness": 0.5,
             "local_residue_exposure_after_tick": 0.8,
             "pending_water_dose_before_tick": 0.2},
            {"type": "health_transition", "resident": "1",
             "after_pathogen": 0.1, "after_illness": 0.0,
             "local_residue_exposure_after_tick": 0.0,
             "pending_water_dose_before_tick": 0.0},
            {"type": "health_transition", "resident": "2",
             "after_pathogen": 1.0, "after_illness": 1.0,
             "local_residue_exposure_after_tick": 0.9,
             "pending_water_dose_before_tick": 0.0},
            {"type": "health_transition", "resident": "3",
             "after_pathogen": 0.0, "after_illness": 0.0,
             "local_residue_exposure_after_tick": 0.0,
             "pending_water_dose_before_tick": 0.0},
        ]
        with ZipFile(path, "w") as archive:
            root = "4242001/"
            archive.writestr(root + "summary.json", json.dumps({
                "complete": complete, "checkpoints": [
                    {"elapsed_day": 1000, "living": 3, "births": 1}]}))
            archive.writestr(root + "evidence_validation.json",
                             json.dumps({"passed": True}))
            archive.writestr(root + "events.jsonl",
                             "\n".join(json.dumps(x) for x in events))
            archive.writestr(root + "metrics.jsonl",
                             "\n".join(json.dumps(x) for x in metrics))
        return path

    def test_every_resident_including_healthy_and_recovered(self):
        with tempfile.TemporaryDirectory() as directory:
            result = analyze_cohort(self.write_archive(directory))
        count = result["cohort"]
        self.assertEqual(count["residents_with_health_or_events"], 3)
        self.assertEqual(count["residents_with_recorded_illness_onset"], 1)
        self.assertEqual(count["residents_with_recorded_recovery"], 1)
        self.assertEqual(count["residents_with_fatal_event"], 1)
        self.assertEqual(count["residents_without_recorded_illness_onset"], 2)
        self.assertEqual(count["duplicated_onset_before_recovery"], 1)
        by_id = {x["resident"]: x for x in result["residents"]}
        self.assertEqual(by_id["1"]["illness_onsets"], 3)
        self.assertEqual(by_id["1"]["recoveries"], 2)
        self.assertEqual(by_id["1"]["mean_local_exposure"], 0.4)
        self.assertEqual(by_id["1"]["max_water_dose"], 0.2)
        self.assertEqual(by_id["3"]["illness_onsets"], 0)
        self.assertEqual(by_id["2"]["fatal_cause"], "illness")

    def test_invalid_archive_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                analyze_cohort(self.write_archive(directory, complete=False))


if __name__ == "__main__":
    unittest.main()
