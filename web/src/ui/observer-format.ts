export function formatDay(minuteValue: unknown): string {
  const minute = Math.max(0, Number(minuteValue) || 0);
  const day = Math.floor(minute / 1440) + 1;
  const clock = minute % 1440;
  const hour = String(Math.floor(clock / 60)).padStart(2, '0');
  const minuteText = String(clock % 60).padStart(2, '0');
  return `Day ${day} · ${hour}:${minuteText}`;
}

export function formatPercent(value: unknown): string {
  const normalized = Math.max(0, Math.min(1, Number(value) || 0));
  return `${Math.round(normalized * 100)}%`;
}
