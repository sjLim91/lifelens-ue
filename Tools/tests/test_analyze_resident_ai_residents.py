"""Per-person evidence must match the Core-verified branch totals."""
import copy
import unittest
from Tools.analyze_resident_ai_residents import analyze_paired_residents


def resident(attempts=0, accepted=0, actual=0, alive=True):
    return {
        "proposalsAttempted": attempts, "proposalsAccepted": accepted,
        "proposalsRejected": attempts-accepted, "civilizationPendingStarts": 0,
        "socialPendingStarts": 0, "otherPendingStarts": 0,
        "civilizationSuccessEvidence": actual,
        "civilizationFailureEvidence": 0,
        "firsthandCivilizationMemories": 0,
        "knownTechniqueRecords": 0,
        "alive": alive,
        "verifiedCoreOutcomes": {
            "KnowledgeTeaching": {"success": actual, "failure": 0},
            "tradeJourneyReturns": 0,
        },
    }


def branch(a, b):
    rows = {"1": a, "18446744073709551615": b}
    result = {"residents": rows, "verifiedCoreOutcomes": {
        "KnowledgeTeaching": {"success": 0, "failure": 0},
        "tradeJourneyReturns": 0,
    }}
    for _, global_key in (
        ("proposalsAttempted", "proposalsAttempted"),
        ("proposalsAccepted", "proposalsAccepted"),
        ("proposalsRejected", "proposalsRejected"),
        ("civilizationPendingStarts", "observedCivilizationPendingStarts"),
        ("socialPendingStarts", "observedSocialPendingStarts"),
        ("otherPendingStarts", "observedOtherPendingStarts"),
        ("civilizationSuccessEvidence", "civilizationSuccessEvidence"),
        ("civilizationFailureEvidence", "civilizationFailureEvidence"),
        ("firsthandCivilizationMemories", "firsthandCivilizationMemories"),
        ("knownTechniqueRecords", "knownTechniqueRecords"),
    ):
        result[global_key] = 0
    for local_key, global_key in (
        ("proposalsAttempted", "proposalsAttempted"),
        ("proposalsAccepted", "proposalsAccepted"),
        ("proposalsRejected", "proposalsRejected"),
        ("civilizationPendingStarts", "observedCivilizationPendingStarts"),
        ("socialPendingStarts", "observedSocialPendingStarts"),
        ("otherPendingStarts", "observedOtherPendingStarts"),
        ("civilizationSuccessEvidence", "civilizationSuccessEvidence"),
        ("civilizationFailureEvidence", "civilizationFailureEvidence"),
        ("firsthandCivilizationMemories", "firsthandCivilizationMemories"),
        ("knownTechniqueRecords", "knownTechniqueRecords"),
    ):
        result[global_key] = sum(row[local_key] for row in rows.values())
    result["livingResidents"] = sum(int(r["alive"]) for r in rows.values())
    result["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"] = sum(
        r["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"]
        for r in rows.values()
    )
    return result


def fixture():
    return {
        "policy": "fixed-deterministic-proxy-not-llm",
        "seed": 4242001, "days": 1,
        "matchedInitialResidents": True,
        "baseline": branch(resident(), resident()),
        "proxy": branch(resident(3, 2, 1), resident(1, 1, 0)),
    }


class TestPersonalPairedAudit(unittest.TestCase):
    def test_lossless_ids_and_real_outcomes(self):
        payload = fixture()
        output = analyze_paired_residents(payload)
        self.assertEqual(list(output["perResident"]),
                         ["1", "18446744073709551615"])
        self.assertFalse(output["causalImprovementDemonstrated"])
        self.assertFalse(output["isLanguageModelExperiment"])
        self.assertEqual(output["perResident"]["1"]["proxy"]
                         ["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"], 1)
        self.assertTrue(output["perResident"]["1"]["comparable"])
        self.assertEqual(payload, fixture())

    def test_tampering_rejected(self):
        variants = []
        bad = fixture()
        bad["proxy"]["proposalsAccepted"] += 1
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["residents"]["1"]["proposalsAccepted"] = 4
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["residents"]["1"]["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"] = 2
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["residents"]["18446744073709551615"]["alive"] = False
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["residents"]["0001"] = bad["proxy"]["residents"].pop("1")
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["residents"]["1"]["proposalsAttempted"] = True
        variants.append(bad)
        for item in variants:
            with self.subTest(item=item):
                with self.assertRaises(ValueError):
                    analyze_paired_residents(item)

    def test_real_core_nested_trade_return_field_contract(self):
        value = fixture()
        result = analyze_paired_residents(value)
        self.assertEqual(result["perResident"]["1"]["proxy"]["tradeJourneyReturns"], 0)
        bad = fixture()
        bad["proxy"]["residents"]["1"]["verifiedCoreOutcomes"].pop("tradeJourneyReturns")
        with self.assertRaises(ValueError):
            analyze_paired_residents(bad)
        bad = fixture()
        bad["proxy"]["residents"]["1"]["tradeJourneyReturns"] = 1
        result = analyze_paired_residents(bad)
        self.assertEqual(result["perResident"]["1"]["proxy"]["tradeJourneyReturns"], 0)
        # Core source of truth is nested; sibling cannot invent a return.

    def test_absent_new_birth_never_fabricated(self):
        value = fixture()
        new = resident()
        value["proxy"]["residents"]["3"] = new
        value["proxy"]["livingResidents"] += 1
        output = analyze_paired_residents(value)
        self.assertIsNone(output["perResident"]["3"]["baseline"])
        self.assertFalse(output["perResident"]["3"]["comparable"])


if __name__ == "__main__":
    unittest.main()
