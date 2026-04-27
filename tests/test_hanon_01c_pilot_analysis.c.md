# test_hanon_01c_pilot_analysis.c

## Purpose
Verifies the production Hanon 01C pilot passage through SQLite-backed analysis.

## Theory
The first real analysis slice must prove that one overlay step can score a two-hand pitch group and that fixture TSV evidence enters through the same persisted event path as captured sessions.

## Architecture Role
This test crosses the production catalog, production overlay store, SQLite store, and `performance_analysis` boundary.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable. Fixture TSV parsing is local, portable C and avoids process-global tokenizers.

## Ownership And Failure Modes
Each fixture case owns an in-memory database and closes it before the next case. Bad fixture rows fail by assertion during test execution.

## Test Strategy
It checks perfect, wrong-note, missing-one-hand, and late-group cases against all eight production pilot steps.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0007-post-session-pitch-and-timing-analysis.md
