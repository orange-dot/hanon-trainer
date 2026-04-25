# performance_analysis.c

## Purpose
Implements deterministic synthetic pitch and timing analysis over persisted MIDI events.

## Theory
Analysis compares ordered note-on events to reviewed overlay steps and stores both aggregate and step-level evidence.

## Architecture Role
This source coordinates catalog, overlay, and SQLite boundaries without owning persistent state.

## Implementation Contract
It refuses variants without overlay coverage, computes wrong/missed/extra/late/early metrics, and persists results.

## Ownership And Failure Modes
Temporary event buffers are module-owned during analysis. Unsupported coverage returns `HT_ERR_UNSUPPORTED`.

## Test Strategy
`tests/test_performance_analysis.c` covers perfect, wrong-note, missing-note, and late-onset fixture sessions.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0007-post-session-pitch-and-timing-analysis.md
