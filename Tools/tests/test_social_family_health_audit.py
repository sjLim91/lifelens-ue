import json
from pathlib import Path
import sys
import tempfile
import unittest
from zipfile import ZipFile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from audit_social_family_health import analyze_artifact


class SocialFamilyHealthArtifactTests(unittest.TestCase):
    def test_birth_death_care_and_exposure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "audit.zip"
            events = [
                {"message": "[Day 5 12:00] birth: Baby child of A and B",
                 "elapsed_day": 4.17, "resident": "0"},
                {"message": "[Day 6 12:00] Baby left sanitation residue id=9 during assisted toilet care via sanitation site",
                 "elapsed_day": 5.17, "resident": "5"},
                {"message": "[Day 7 12:00] Baby died from illness",
                 "elapsed_day": 6.17, "resident": "5"},
            ]
            health = [
                {"type": "health_transition", "resident": "5", "elapsed_day": 5.17,
                 "after_pathogen": 0.8, "after_illness": 0.5,
                 "local_residue_exposure_after_tick": 0.9,
                 "pending_water_dose_before_tick": 0.0},
                {"type": "health_transition", "resident": "5", "elapsed_day": 6.17,
                 "after_pathogen": 1.0, "after_illness": 1.0,
                 "local_residue_exposure_after_tick": 1.0,
                 "pending_water_dose_before_tick": 0.0},
            ]
            with ZipFile(path, "w") as archive:
                prefix = "42/"
                archive.writestr(prefix+"summary.json", json.dumps({
                    "complete": True, "checkpoints": [{
                        "elapsed_day": 10, "total": 5, "living": 4,
                        "deceased": 1, "births": 1}]}))
                archive.writestr(prefix+"evidence_validation.json", json.dumps({"passed": True}))
                archive.writestr(prefix+"events.jsonl",
                                 "\n".join(json.dumps(row) for row in events))
                archive.writestr(prefix+"metrics.jsonl",
                                 "\n".join(json.dumps(row) for row in health))
            result = analyze_artifact(path)
            self.assertEqual(result["final"]["living"], 4)
            self.assertEqual(result["surviving_births_in_run"], 0)
            self.assertEqual(result["toilet_assist_site_count"], 1)
            self.assertEqual(result["distinct_assist_residue_ids"], 1)
            baby = result["deaths_of_births_in_run"][0]
            self.assertEqual(baby["age_at_death_days"], 2.0)
            self.assertEqual(baby["mean_local_exposure"], 0.95)

    def test_incomplete_run_is_not_accepted(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "audit.zip"
            with ZipFile(path, "w") as archive:
                archive.writestr("42/summary.json", json.dumps({"complete": False}))
                archive.writestr("42/evidence_validation.json", json.dumps({"passed": True}))
            with self.assertRaises(ValueError):
                analyze_artifact(path)


if __name__ == "__main__":
    unittest.main()
