"""Observed Core behavior deltas must never invent model learning."""
import copy
import unittest

from Tools.analyze_resident_ai_longitudinal import analyze_series
from Tools.tests.test_analyze_resident_ai_residents import fixture


class TestCoreLongitudinalResidentAudit(unittest.TestCase):
    def test_two_checkpoints_zero_delta_is_valid(self):
        first = fixture()
        second = copy.deepcopy(first)
        first["days"], second["days"] = 1, 7
        result = analyze_series([first, second])
        self.assertEqual(result["days"], [1, 7])
        self.assertFalse(result["longitudinalLearningProven"])
        self.assertFalse(result["causalImprovementDemonstrated"])
        rid = result["windows"][0]["residents"]["1"]
        self.assertEqual(rid["proxy"]["verifiedCoreOutcomeDelta"]
                         ["KnowledgeTeaching"]["successDelta"], 0)
        self.assertTrue(rid["proxy"]["acceptedProposalsAreNotCompletedActions"])

    def test_later_real_core_actions_not_fake_proposals(self):
        first = fixture()
        second = copy.deepcopy(first)
        second["days"] = 100
        actor = second["proxy"]["residents"]["1"]
        actor["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"] += 1
        actor["civilizationSuccessEvidence"] += 1
        second["proxy"]["civilizationSuccessEvidence"] += 1
        second["proxy"]["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"] += 1
        row = analyze_series([first, second])["windows"][0]["residents"]["1"]["proxy"]
        self.assertEqual(row["verifiedCoreOutcomeDelta"]
                         ["KnowledgeTeaching"]["successDelta"], 1)
        self.assertEqual(row["observedMetricDelta"]["proposalsAccepted"], 0)

    def test_cross_seed_reordering_or_counter_reversal_rejected(self):
        first = fixture()
        second = copy.deepcopy(first)
        second["days"] = 365
        scenarios = []
        bad = copy.deepcopy(second); bad["seed"] += 1; scenarios.append(bad)
        bad = copy.deepcopy(second); bad["days"] = 1; scenarios.append(bad)
        bad = copy.deepcopy(second)
        bad["proxy"]["residents"]["1"]["proposalsAccepted"] = 1
        bad["proxy"]["residents"]["1"]["proposalsRejected"] = 2
        bad["proxy"]["proposalsAccepted"] -= 1
        bad["proxy"]["proposalsRejected"] += 1
        scenarios.append(bad)
        for item in scenarios:
            with self.subTest(item=item):
                with self.assertRaises(ValueError):
                    analyze_series([first, item])

    def test_new_outcome_kind_must_be_visible_without_invented_baseline(self):
        first = fixture()
        second = copy.deepcopy(first)
        second["days"] = 365
        # New evidence categories can emerge between report versions. A
        # missing earlier kind is UNKNOWN, not proof that it was zero.
        for branch_name in ("baseline", "proxy"):
            branch = second[branch_name]
            branch["verifiedCoreOutcomes"]["EmergencyShelter"] = {
                "success": 1, "failure": 0,
            }
            for rid, resident in branch["residents"].items():
                resident["verifiedCoreOutcomes"]["EmergencyShelter"] = {
                    "success": int(rid == "1"), "failure": 0,
                }
        result = analyze_series([first, second])
        row = result["windows"][0]["residents"]["1"]["proxy"]
        self.assertEqual(row["verifiedCoreOutcomeDelta"]["EmergencyShelter"], {
            "comparable": False,
            "reason": "outcome_kind_missing_from_previous_checkpoint",
        })
        self.assertEqual(row["verifiedCoreOutcomeDelta"]["KnowledgeTeaching"]
                         ["successDelta"], 0)

    def test_new_core_trade_return_and_counter_reversal(self):
        first = fixture()
        second = copy.deepcopy(first)
        second["days"] = 100
        second["proxy"]["residents"]["1"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = 1
        second["proxy"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = 1
        result = analyze_series([first, second])
        self.assertEqual(result["windows"][0]["residents"]["1"]["proxy"]
                         ["tradeJourneyReturnsDelta"], 1)
        reverse = copy.deepcopy(second)
        reverse["days"] = 365
        reverse["proxy"]["residents"]["1"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = 0
        reverse["proxy"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = 0
        with self.assertRaisesRegex(ValueError, "nonmonotonic verified Core trade"):
            analyze_series([first, second, reverse])

    def test_disappearing_core_outcome_kind_rejected(self):
        first = fixture()
        second = copy.deepcopy(first)
        second["days"] = 100
        for branch_name in ("baseline", "proxy"):
            branch = second[branch_name]
            branch["verifiedCoreOutcomes"].pop("KnowledgeTeaching")
            branch["verifiedCoreOutcomes"]["EmergencyShelter"] = {
                "success": 0, "failure": 0,
            }
            for resident in branch["residents"].values():
                resident["verifiedCoreOutcomes"].pop("KnowledgeTeaching")
                resident["verifiedCoreOutcomes"]["EmergencyShelter"] = {
                    "success": 0, "failure": 0,
                }
        with self.assertRaisesRegex(ValueError, "disappeared Core outcome"):
            analyze_series([first, second])

    def test_empty_singleton_and_invalid_policy(self):
        first = fixture()
        with self.assertRaises(ValueError):
            analyze_series([])
        with self.assertRaises(ValueError):
            analyze_series([first])
        later = copy.deepcopy(first)
        later["days"] = 100
        later["policy"] = "local-language-model"
        with self.assertRaises(ValueError):
            analyze_series([first, later])


if __name__ == "__main__":
    unittest.main()
