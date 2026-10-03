"""Regression tests for acceptance: the old passing-but-regressed data must fail."""
import json
import unittest
from verify_sleep_longrun import BASELINE_PATH, compare_audit


class SleepLongrunAcceptanceTest(unittest.TestCase):
    def setUp(self):
        self.baseline = json.loads(BASELINE_PATH.read_text())

    def audit(self, item, values):
        return (f"RESIDENT seed={item['seed']} day=365 id=1 alive=1 "
                f"sleepLongestSat={values['longestSleepSaturationMinutes']} "
                f"sleepSatMin={values['totalSleepSaturationResidentMinutes']}\n"
                f"AUDIT_COMPLETE seed={item['seed']} days=365\n")

    def test_pinned_main_passes(self):
        for item in self.baseline['results']:
            self.assertFalse(compare_audit(self.audit(item, item['main']), self.baseline)[1])

    def test_old_cumulative_regression_fails_even_with_bounded_streak(self):
        for item in self.baseline['results'][:2]:
            # Keep the original failing evidence independent of refreshed
            # candidate entries in the report JSON.
            values = {'longestSleepSaturationMinutes': 671 if item['seed'] == 874213954 else 501,
                      'totalSleepSaturationResidentMinutes': 142910 if item['seed'] == 874213954 else 18999}
            failures = compare_audit(self.audit(item, values), self.baseline)[1]
            self.assertTrue(any('totalSleepSaturationResidentMinutes' in value for value in failures))

    def test_continuous_regression_fails_even_when_total_is_acceptable(self):
        item = self.baseline['results'][0]
        values = {**item['main'], 'longestSleepSaturationMinutes': 10000}
        failures = compare_audit(self.audit(item, values), self.baseline)[1]
        self.assertIn('continuous fatigue starvation', failures)

    def test_trajectory_tolerance_and_missing_completion(self):
        item = self.baseline['results'][1]
        values = {**item['main'], 'totalSleepSaturationResidentMinutes': 14900}
        self.assertFalse(compare_audit(self.audit(item, values), self.baseline)[1])
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            compare_audit('RESIDENT seed=2 day=365', self.baseline)
        with self.assertRaisesRegex(ValueError, 'no pinned'):
            compare_audit('AUDIT_COMPLETE seed=99 days=365', self.baseline)


if __name__ == '__main__':
    unittest.main()
