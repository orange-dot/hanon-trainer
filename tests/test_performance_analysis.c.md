# test_performance_analysis.c

## Purpose
Verifies synthetic analysis over persisted session and MIDI events.

## Theory
The vertical slice should score deterministic fixture data before manual Hanon coordinates exist.

## Architecture Role
This test crosses catalog, overlay, SQLite, and analysis boundaries.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable, with helper functions local to the file.

## Ownership And Failure Modes
Each analysis case uses an in-memory database and closes all handles. Assertions cover expected metric outcomes.

## Test Strategy
It checks perfect, wrong-note, missing-note, and late-onset cases.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0007-post-session-pitch-and-timing-analysis.md
