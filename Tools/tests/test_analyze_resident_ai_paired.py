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

    def test_per_intent_and_temporal_attribution_are_not_fake_successes(self):
        src = fixture()
        src["proxy"]["acceptedIntentToVerifiedOutcome"]["KnowledgeTeaching"][
            "temporallyMatchedSuccess"
        ] = 2
        out = analyze(src)
        lifecycle = out["intentLifecycle"]
        self.assertEqual(lifecycle["observedIntentKinds"], ["ImproveFoodSecurity"])
        self.assertEqual(lifecycle["intentKindCount"], 1)
        self.assertFalse(lifecycle["all14IntentsObserved"])
        self.assertTrue(lifecycle["notVerifiedActionCompletions"])
        rows = lifecycle["perIntent"]["ImproveFoodSecurity"]
        self.assertEqual(rows["accepted"]["deltaProxyMinusBaseline"], 5)
        self.assertEqual(rows["rejectedByValidator"]["proxy"], 2)
        self.assertEqual(rows["rejectedAfterValidator"]["proxy"], 0)
        self.assertEqual(
            out["temporalAttribution"]["KnowledgeTeaching"][
                "temporallyMatchedSuccess"]["deltaProxyMinusBaseline"], 2
        )
        self.assertEqual(
            out["verifiedCoreOutcomes"]["KnowledgeTeaching"]["successDelta"], 2
        )
        self.assertFalse(out["causalImprovementDemonstrated"])

    def test_verified_trade_returns_are_not_model_success(self):
        src = fixture()
        src["proxy"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = 2
        out = analyze(src)
        returns = out["verifiedTradeJourneyReturns"]
        self.assertTrue(returns["available"])
        self.assertEqual(returns["baseline"], 0)
        self.assertEqual(returns["proxy"], 2)
        self.assertEqual(returns["deltaProxyMinusBaseline"], 2)
        self.assertTrue(returns["notCausalProof"])
        self.assertFalse(out["causalImprovementDemonstrated"])

        # A malformed or false return count must never appear as a verified
        # Core trip; accepted proposals cannot substitute for journey returns.
        for malformed in (-1, True, 0.5, "3"):
            bad = copy.deepcopy(src)
            bad["proxy"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = malformed
            with self.subTest(malformed=malformed):
                with self.assertRaisesRegex(ValueError, "tradeJourneyReturns"):
                    analyze(bad)

    def test_legacy_trade_return_evidence_has_explicit_unknown_state(self):
        old = fixture()
        del old["baseline"]["verifiedCoreOutcomes"]["tradeJourneyReturns"]
        del old["proxy"]["verifiedCoreOutcomes"]["tradeJourneyReturns"]
        self.assertEqual(analyze(old)["verifiedTradeJourneyReturns"],
                         {"available": False})
        mismatched = copy.deepcopy(old)
        mismatched["proxy"]["verifiedCoreOutcomes"]["tradeJourneyReturns"] = 0
        with self.assertRaisesRegex(ValueError, "trade return evidence availability"):
            analyze(mismatched)

    def test_real_pending_token_lifecycle_is_not_completed_action(self):
        report = fixture()
        report["baseline"]["tokenLifecycle"] = {
            "KnowledgeTeaching": {
                "observedStarts": 4,
                "temporalIntentTargetMatches": 1,
                "noLongerPendingUnclassified": 2,
                "replacedByAnotherToken": 1,
            },
        }
        report["proxy"]["tokenLifecycle"] = {
            "KnowledgeTeaching": {
                "observedStarts": 6,
                "temporalIntentTargetMatches": 2,
                "noLongerPendingUnclassified": 3,
                "replacedByAnotherToken": 2,
            },
        }
        result = analyze(report)
        pending = result["pendingTokenLifecycle"]
        self.assertTrue(pending["available"])
        self.assertTrue(pending["notVerifiedGoalCompletion"])
        evidence = pending["perKind"]["KnowledgeTeaching"]
        self.assertEqual(evidence["observedStarts"]["deltaProxyMinusBaseline"], 2)
        self.assertEqual(evidence["replacedByAnotherToken"]["proxy"], 2)
        self.assertFalse(result["causalImprovementDemonstrated"])
        self.assertEqual(
            analyze(fixture())["pendingTokenLifecycle"], {"available": False})

        for bad_value in (-1, True, 3.2, "5", 7):
            bad = copy.deepcopy(report)
            bad["proxy"]["tokenLifecycle"]["KnowledgeTeaching"][
                "temporalIntentTargetMatches"] = bad_value
            with self.subTest(bad_value=bad_value):
                with self.assertRaises(ValueError):
                    analyze(bad)

        missing = copy.deepcopy(report)
        del missing["baseline"]["tokenLifecycle"]
        with self.assertRaisesRegex(ValueError, "lifecycle availability"):
            analyze(missing)

        extraneous = copy.deepcopy(report)
        extraneous["proxy"]["tokenLifecycle"]["InventedOutcome"] = {}
        with self.assertRaisesRegex(ValueError, "lifecycle kinds"):
            analyze(extraneous)

    def test_mismatched_catalog_intent_sets_must_not_compare(self):
        src = fixture()
        src["proxy"]["catalogIntentInputs"]["OtherIntent"] = (
            src["proxy"]["catalogIntentInputs"].pop("ImproveFoodSecurity")
        )
        with self.assertRaisesRegex(ValueError, "same intent catalog kinds"):
            analyze(src)

    def test_complete_14_kind_coverage_is_explicit_not_assumed(self):
        src = fixture()
        # All extra categories have zero counts. Neither a zero proposal
        # nor complete catalog coverage is evidence of action completion.
        zero = {key: 0 for key in src["baseline"]["catalogIntentInputs"][
            "ImproveFoodSecurity"]}
        from Tools.analyze_resident_ai_paired import CANONICAL_INTENTS
        for name in CANONICAL_INTENTS - {"ImproveFoodSecurity"}:
            src["baseline"]["catalogIntentInputs"][name] = dict(zero)
            src["proxy"]["catalogIntentInputs"][name] = dict(zero)
        result = analyze(src)
        self.assertTrue(result["intentLifecycle"]["all14IntentsObserved"])
        self.assertEqual(result["intentLifecycle"]["intentKindCount"], 14)
        self.assertEqual(
            result["intentLifecycle"]["perIntent"]["ImproveWaterSecurity"][
                "submitted"]["proxy"], 0
        )
        counterfeit = copy.deepcopy(src)
        counterfeit["baseline"]["catalogIntentInputs"]["InventedIntent"] = (
            counterfeit["baseline"]["catalogIntentInputs"].pop("ImproveShelter"))
        counterfeit["proxy"]["catalogIntentInputs"]["InventedIntent"] = (
            counterfeit["proxy"]["catalogIntentInputs"].pop("ImproveShelter"))
        self.assertFalse(analyze(counterfeit)["intentLifecycle"]["all14IntentsObserved"])

    def test_catalog_presence_does_not_claim_all_intents_were_selected(self):
        src = fixture()
        zero = {key: 0 for key in src["baseline"]["catalogIntentInputs"][
            "ImproveFoodSecurity"]}
        from Tools.analyze_resident_ai_paired import CANONICAL_INTENTS
        for name in CANONICAL_INTENTS - {"ImproveFoodSecurity"}:
            src["baseline"]["catalogIntentInputs"][name] = dict(zero)
            src["proxy"]["catalogIntentInputs"][name] = dict(zero)
        report = analyze(src)
        lifecycle = report["intentLifecycle"]
        self.assertTrue(lifecycle["all14IntentsObserved"])
        self.assertFalse(lifecycle["all14IntentsActuallySelected"])
        self.assertTrue(lifecycle["selectedIsNotCoreCompletion"])
        self.assertEqual(lifecycle["activelySelectedIntents"],
                         ["ImproveFoodSecurity"])
        self.assertEqual(set(lifecycle["neverSelectedIntents"]),
                         CANONICAL_INTENTS - {"ImproveFoodSecurity"})
        self.assertFalse(report["causalImprovementDemonstrated"])

        # A zero-row catalog must not become proof of 14 lived goals.
        src["baseline"]["catalogIntentInputs"]["ImproveWaterSecurity"] = {
            **zero, "selected": 1, "notOffered": 1,
        }
        out = analyze(src)["intentLifecycle"]
        self.assertIn("ImproveWaterSecurity", out["activelySelectedIntents"])
        self.assertNotIn("ImproveWaterSecurity", out["neverSelectedIntents"])
        self.assertFalse(out["all14IntentsActuallySelected"])

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
