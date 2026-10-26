"""M0-03 native per-resident intent lifecycle and honest Core terminal limits."""
import copy
import json
import tempfile
import unittest
from pathlib import Path

from Tools.analyze_resident_ai_paired import CANONICAL_INTENTS, POLICY
from Tools.m0_intent_closeout import closeout


def branch(active):
    row = {
        "selected": int(active), "notOffered": 0, "noEligibleTarget": 0,
        "submitted": int(active), "accepted": int(active), "rejected": 0,
        "rejectedByValidator": 0, "rejectedAfterValidator": 0,
    }
    catalog = {intent: dict(row) for intent in CANONICAL_INTENTS}
    actual = {name: {"success": 0, "failure": 0}
              for name in ("KnowledgeTeaching", "Parenting", "Trade")}
    actual["tradeJourneyReturns"] = 0
    token = {name: {"observedStarts": 0, "temporalIntentTargetMatches": 0,
                    "noLongerPendingUnclassified": 0,
                    "replacedByAnotherToken": 0}
             for name in ("KnowledgeTeaching", "Parenting", "Trade")}
    matches = {name: {"temporallyMatchedSuccess": 0, "temporallyMatchedFailure": 0,
                      "observedWithoutAcceptedIntent": 0,
                      "noObservedPendingToken": 0}
               for name in ("KnowledgeTeaching", "Parenting", "Trade")}
    total = 14 if active else 0
    person = {"alive": True, "proposalsAttempted": total,
              "proposalsAccepted": total, "proposalsRejected": 0,
              "catalogIntentInputs": copy.deepcopy(catalog),
              "verifiedCoreOutcomes": copy.deepcopy(actual)}
    return {
        "proposalsAttempted": total, "proposalsAccepted": total,
        "proposalsRejected": 0, "observedCivilizationPendingStarts": 0,
        "observedSocialPendingStarts": 0, "civilizationSuccessEvidence": 0,
        "civilizationFailureEvidence": 0, "firsthandCivilizationMemories": 0,
        "livingResidents": 1, "catalogIntentInputs": catalog,
        "verifiedCoreOutcomes": actual,
        "acceptedIntentToVerifiedOutcome": matches, "tokenLifecycle": token,
        "residents": {"1": person},
    }


def fixture(seed, day):
    return {"seed": seed, "days": day, "policy": POLICY,
            "proxySelection": "all-fourteen-core-catalog",
            "matchedInitialResidents": True,
            "baseline": branch(False), "proxy": branch(True)}


class TestM0IntentCloseout(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)

    def write(self, seeds=(11, 22), days=(100, 365, 1000)):
        paths = []
        for seed in seeds:
            for day in days:
                path = self.directory / f"{seed}-{day}.json"
                path.write_text(json.dumps(fixture(seed, day)), encoding="utf-8")
                paths.append(path)
        return paths

    def test_all_14_per_resident_real_stage_receipts_are_reconciled(self):
        result = closeout(self.write())
        self.assertEqual(result["verifiedIndependentRunCount"], 6)
        self.assertEqual(result["intentCount"], 14)
        self.assertTrue(result["acceptedIsNotCompletion"])
        self.assertIsNone(result["reports"][0]["perIntent"]["TeachKnowledge"]
                          ["intentCausedCompletion"])
        self.assertEqual(result["reports"][0]["perIntent"]["CraftUsefulTools"]
                         ["terminalAttributionStatus"],
                         "no_intent_specific_Core_receipt")
        self.assertEqual(result["reports"][0]["perIntent"]["CareForDependent"]
                         ["proxy"]["submitted"], 1)

    def test_missing_day_or_same_seed_fails(self):
        with self.assertRaisesRegex(ValueError, "at least 2"):
            closeout(self.write(seeds=(11,)))
        paths = self.write()
        with self.assertRaisesRegex(ValueError, "missing days"):
            closeout(paths[:-1] + [paths[-2]])
        # Even with six files, duplicate day cannot count as another horizon.
        with self.assertRaisesRegex(ValueError, "duplicate seed/day"):
            closeout(paths[:5] + [paths[0]])

    def test_forged_person_or_mismatched_count_fails(self):
        paths = self.write()
        data = json.loads(paths[0].read_text())
        data["proxy"]["residents"]["1"]["catalogIntentInputs"][
            "CraftUsefulTools"]["accepted"] = 0
        paths[0].write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, "per-resident CraftUsefulTools.accepted"):
            closeout(paths)

    def test_core_terminal_event_count_must_reconcile_by_actor(self):
        paths = self.write()
        data = json.loads(paths[0].read_text())
        data["proxy"]["verifiedCoreOutcomes"]["Parenting"]["success"] = 3
        paths[0].write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, "resident Core Parenting mismatch"):
            closeout(paths)

    def test_catalog_only_is_not_a_language_model_or_completed_action(self):
        paths = self.write()
        data = json.loads(paths[0].read_text())
        data["proxySelection"] = "original-four"
        paths[0].write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, "fourteen-intent"):
            closeout(paths)


if __name__ == "__main__":
    unittest.main()
