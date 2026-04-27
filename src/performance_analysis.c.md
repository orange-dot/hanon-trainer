# performance_analysis.c

## Purpose
Implements deterministic pitch and timing analysis over persisted MIDI events.

## Theory
Analysis compares timestamp-owned note-on groups to reviewed overlay steps and stores both aggregate and step-level evidence. This slice keeps the one-step-per-second timing model while allowing one step to expect multiple simultaneous pitches.

## Architecture Role
This source coordinates catalog, overlay, and SQLite boundaries without owning persistent state.

## Implementation Contract
It refuses variants without overlay coverage, assigns note-on events to half-open step windows, scores expected pitch groups with duplicate-safe matching, computes wrong/missed/extra/late/early metrics, counts each weak step once, and persists results.

## Ownership And Failure Modes
Temporary event buffers are module-owned during analysis. Unsupported coverage returns `HT_ERR_UNSUPPORTED`.

## Test Strategy
`tests/test_performance_analysis.c` covers perfect, wrong-note, missing-note, late-onset, and midpoint-ownership synthetic sessions. `tests/test_hanon_01c_pilot_analysis.c` covers production Hanon 01C pitch groups.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0007-post-session-pitch-and-timing-analysis.md
