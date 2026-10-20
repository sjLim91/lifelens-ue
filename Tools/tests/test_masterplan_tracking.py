"""M0-05: guard masterplan completion evidence against optimistic bookkeeping."""
import unittest
from Tools.validate_masterplan_tracking import verify_masterplan_ledger

PLAN = """| 단위 | 개발 산출물 | 완료 판정 |
| M0-01 | Real trade | Live results |
| M1-01 | Perception | Real facts |
"""
GOOD = """- 완료: **1/2** (M0-01).
- 검증 완료·병합 검토: **0**.
- 진행: **1**.
- 미완 또는 미검증: **0**.

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
| [x] | **M0-01** | Real trade | ✅ 완료 | [PR](https://github.com/sjLim91/lifelens-ue/pull/816) main e6061967 |
| [ ] | **M1-01** | Perception | 🚧 진행 | Core test still pending |
"""


class TestMasterplanTracking(unittest.TestCase):
    def test_valid_partial_implementation(self):
        self.assertEqual(verify_masterplan_ledger(PLAN, GOOD),
                         dict(complete=1, review=0, in_progress=1,
                              unfinished=0, total=2))

    def test_unknown_or_duplicate_unit_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate tracked"):
            verify_masterplan_ledger(PLAN, GOOD + GOOD.splitlines()[-1] + "\n")
        with self.assertRaisesRegex(ValueError, "drift"):
            verify_masterplan_ledger(PLAN, GOOD.replace("M1-01", "M9-99"))

    def test_false_checked_completion_rejected(self):
        with self.assertRaisesRegex(ValueError, "checkbox"):
            verify_masterplan_ledger(PLAN, GOOD.replace("[ ] | **M1-01**", "[x] | **M1-01**"))
        with self.assertRaisesRegex(ValueError, "main/PR"):
            verify_masterplan_ledger(PLAN, GOOD.replace(
                "[PR](https://github.com/sjLim91/lifelens-ue/pull/816) main e6061967",
                "local tests passed"))

    def test_stale_aggregates_rejected(self):
        with self.assertRaisesRegex(ValueError, "summary mismatch"):
            verify_masterplan_ledger(PLAN, GOOD.replace("- 진행: **1**", "- 진행: **2**"))
        with self.assertRaisesRegex(ValueError, "summary mismatch"):
            verify_masterplan_ledger(PLAN, GOOD.replace("**1/2**", "**2/2**"))

    def test_empty_and_duplicate_plan_rejected(self):
        with self.assertRaisesRegex(ValueError, "empty or duplicated"):
            verify_masterplan_ledger("", GOOD)
        with self.assertRaisesRegex(ValueError, "empty or duplicated"):
            verify_masterplan_ledger(PLAN + "| M0-01 | Duplicate | No |\n", GOOD)


if __name__ == "__main__":
    unittest.main()
