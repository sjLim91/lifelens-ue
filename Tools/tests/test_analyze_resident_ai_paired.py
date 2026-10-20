"""Paired Core evidence must never be presented as model success."""
import copy
import unittest

from Tools.analyze_resident_ai_paired import analyze, POLICY


def branch(accepted, rejected, verified_success):
    return {
        "proposalsAttempted": accepted + rejected,
        "proposalsAccepted": accepted,
        "proposalsRejected": rejected,
        "observedCivilizationPendingStarts": 3,
        "observedSocialPendingStarts": 2,
        "civilizationSuccessEvidence": 1,
        "civilizationFailureEvidence": 2,
        "firsthandCivilizationMemories": 1,
        "livingResidents": 4,
        "catalogIntentInputs": {
            "ImproveFoodSecurity": {
                "selected": accepted + rejected,
                "notOffered": 0, "noEligibleTarget": 0,
                "submitted": accepted + rejected,
                "accepted": accepted, "rejected": rejected,
                "rejectedByValidator": rejected,
                "rejectedAfterValidator": 0,
            },
        },
        "verifiedCoreOutcomes": {
            "KnowledgeTeaching": {"success": verified_success, "failure": 1},
            "tradeJourneyReturns": 0,
        },
        "acceptedIntentToVerifiedOutcome": {
            "KnowledgeTeaching": {
                "temporallyMatchedSuccess": 0,
                "temporallyMatchedFailure": 0,
                "observedWithoutAcceptedIntent": 0,
                "noObservedPendingToken": 0,
            },
        },
    }


def fixture():
    return {
        "seed": 4242001, "days": 1, "policy": POLICY,
        "matchedInitialResidents": True,
        "baseline": branch(0, 0, 1),
        "proxy": branch(5, 2, 3),
    }


class TestPairedAuditAnalysis(unittest.TestCase):
    def test_separates_proposals_and_actual_core_results(self):
        out = analyze(fixture())
        self.assertFalse(out["languageModelEvaluated"])
        self.assertFalse(out["causalImprovementDemonstrated"])
        self.assertEqual(out["comparison"]["proposalsAccepted"]
                         ["deltaProxyMinusBaseline"], 5)
        self.assertEqual(out["verifiedCoreOutcomes"]["KnowledgeTeaching"]
                         ["successDelta"], 2)
        self.assertIn("not proof", out["interpretation"])

    def test_invalid_evidence_never_silently_passes(self):
        variants = []
        bad = fixture()
        bad["policy"] = "local-language-model"
        variants.append(bad)
        bad = fixture()
        bad["matchedInitialResidents"] = False
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["proposalsAttempted"] = 99
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["catalogIntentInputs"]["ImproveFoodSecurity"]["accepted"] = 50
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["acceptedIntentToVerifiedOutcome"]["KnowledgeTeaching"]["temporallyMatchedSuccess"] = 4
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["verifiedCoreOutcomes"]["KnowledgeTeaching"]["success"] = -1
        variants.append(bad)
        bad = fixture()
        bad["proxy"]["proposalsAccepted"] = True
        variants.append(bad)
        for malformed in variants:
            with self.subTest(malformed=malformed):
                with self.assertRaises(ValueError):
                    analyze(malformed)

    def test_input_immutable_repeated_report_deterministic(self):
        src = fixture()
        original = copy.deepcopy(src)
        self.assertEqual(analyze(src), analyze(src))
        self.assertEqual(src, original)


if __name__ == "__main__":
    unittest.main()
