# ADR-0007: Post-Session Pitch And Timing Analysis

## Status

Accepted

## Context

Once MIDI is captured, the app needs a first analysis target that is useful,
implementable, and consistent with manual-first overlays.

Broad pedagogical grading, expressive analysis, and fingering correctness would
materially increase annotation cost and complexity.

## Decision

Analyze completed takes after capture ends.

The first analysis scope is limited to:

- expected pitch-group matching
- missed and extra note detection
- onset timing deviation against overlay-backed step expectations

The first build does not attempt:

- real-time adaptive coaching
- expressive phrasing grades
- fingering correctness grades

## Consequences

- analysis remains deterministic and reviewable
- overlays only need enough detail for step order, pitch groups, and timing
  windows
- local review metrics can be shown before any AI advice is generated
