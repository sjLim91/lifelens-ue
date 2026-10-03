#!/usr/bin/env python3
"""Compare completed 365-day sleep audits with the pinned main baseline.

Tolerances are regression acceptance settings only, never gameplay rules.
"""
import json
from pathlib import Path
import re
import sys

BASELINE_PATH = Path(__file__).resolve().parents[1] / 'tasks/WEATHER_SLEEP_ERA_LONGRUN.json'
MAX_CONTINUOUS_SLEEP_SATURATION_MINUTES = 10_000
SATURATION_MULTIPLIER_TOLERANCE = 1.5
TOTAL_ADDITIONAL_MINUTES_TOLERANCE = 10_000
CONTINUOUS_ADDITIONAL_MINUTES_TOLERANCE = 300


def compare_audit(content, baseline):
    completion = re.search(r'AUDIT_COMPLETE seed=(\d+) days=(\d+)', content)
    if not completion:
        raise ValueError('incomplete audit')
    seed, days = map(int, completion.groups())
    reference = next((item['main'] for item in baseline['results']
                      if item['seed'] == seed and item['days'] == days), None)
    if reference is None:
        raise ValueError(f'no pinned main baseline for seed={seed}, days={days}')
    rows = [dict(re.findall(r'(\w+)=([^\s]+)', line))
            for line in content.splitlines()
            if line.startswith('RESIDENT ') and f'day={days} ' in line]
    if not rows:
        raise ValueError('missing final resident metrics')
    result = {
        'seed': seed, 'days': days,
        'longestSleepSaturationMinutes': max(int(row['sleepLongestSat']) for row in rows),
        'totalSleepSaturationResidentMinutes': sum(int(row['sleepSatMin']) for row in rows),
        'livingResidents': sum(int(row['alive']) for row in rows),
    }
    failures = []
    if result['longestSleepSaturationMinutes'] >= MAX_CONTINUOUS_SLEEP_SATURATION_MINUTES:
        failures.append('continuous fatigue starvation')
    for key, allowance in [
        ('longestSleepSaturationMinutes', CONTINUOUS_ADDITIONAL_MINUTES_TOLERANCE),
        ('totalSleepSaturationResidentMinutes', TOTAL_ADDITIONAL_MINUTES_TOLERANCE),
    ]:
        limit = max(reference[key] * SATURATION_MULTIPLIER_TOLERANCE,
                    reference[key] + allowance)
        if result[key] > limit:
            failures.append(f'{key}={result[key]} exceeds baseline limit {limit:g}')
    result['baselineCommit'] = baseline['baseCommit']
    result['baseline'] = reference
    return result, failures


def main(arguments):
    baseline = json.loads(BASELINE_PATH.read_text())
    results, errors = [], []
    for argument in arguments:
        path = Path(argument)
        try:
            result, failures = compare_audit(path.read_text(), baseline)
            results.append(result)
            errors.extend(f'{path}: {failure}' for failure in failures)
        except (ValueError, KeyError) as error:
            errors.append(f'{path}: {error}')
    if not arguments:
        errors.append('provide balance-audit logs')
    print(json.dumps(results, indent=2))
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
