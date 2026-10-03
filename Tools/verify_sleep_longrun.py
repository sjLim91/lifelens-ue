#!/usr/bin/env python3
"""Reject the multi-day fatigue starvation regression from balance-audit logs."""
import json
from pathlib import Path
import re
import sys

# Acceptance guard for the explicitly prohibited tens-of-thousands-minute
# saturation, not a simulation rule or gameplay threshold.
MAX_CONTINUOUS_SLEEP_SATURATION_MINUTES = 10_000
results = []
for argument in sys.argv[1:]:
    path = Path(argument)
    content = path.read_text()
    completion = re.search(r'AUDIT_COMPLETE seed=(\d+) days=(\d+)', content)
    if not completion:
        raise SystemExit(f'{path}: incomplete audit')
    seed, days = map(int, completion.groups())
    if days < 365:
        raise SystemExit(f'{path}: need at least 365 days')
    rows = [dict(re.findall(r'(\w+)=([^ ]+)', line))
            for line in content.splitlines()
            if line.startswith('RESIDENT ') and f'day={days} ' in line]
    if not rows:
        raise SystemExit(f'{path}: missing final resident metrics')
    result = {
        'seed': seed, 'days': days,
        'longestSleepSaturationMinutes': max(int(row['sleepLongestSat']) for row in rows),
        'totalSleepSaturationResidentMinutes': sum(int(row['sleepSatMin']) for row in rows),
        'livingResidents': sum(int(row['alive']) for row in rows),
    }
    results.append(result)
    if result['longestSleepSaturationMinutes'] >= MAX_CONTINUOUS_SLEEP_SATURATION_MINUTES:
        raise SystemExit(f'{path}: fatigue starvation: {result}')
if not results:
    raise SystemExit('provide balance-audit logs')
print(json.dumps(results, indent=2))
