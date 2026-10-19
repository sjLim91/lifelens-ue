# Readable memory attribution in long-run audits

The raw per-resident memory measurements added in #801 are now summarized by checkpoint in summary.json, summary.md, Actions logs and the job summary.

The report uses the last row per resident/day. It reports entries, UTF-8 text payload, exact duplicates within each resident, provenance counts, age buckets and largest primary-tag contributors. It includes dead residents still reported by Core. Repeated daily/checkpoint rows do not inflate totals. Existing artifacts without this evidence are explicitly unavailable; partial coverage is marked. Text payload is not heap or snapshot size, and tag/source totals do not prove a causal behavior change.

The runner, Core parameters, invariant gates, determinism comparison, saved bytes and memory retention policy are unchanged. This is read-only reporting; no memory is removed.

Validation: previous seven audit invariant regressions plus last-row aggregation, legacy/partial evidence and checkpoint-separated end-to-end report publication.

The first 1000-day report exposed a diagnostic cardinality problem: unique fact:<id> tags consumed the 64-category budget and hid most social memories in other tags. The report now groups numeric fact IDs by their actual MemorySource into social_fact:direct/told/inferred. Original memory tags, full-record duplicate comparisons and saved data remain untouched. Regression covers 100 distinct facts, source separation, category sums and unchanged originals.
