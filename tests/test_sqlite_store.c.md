# test_sqlite_store.c

## Purpose
Verifies SQLite migration and persistence APIs.

## Theory
The database is the single local store for user state, takes, MIDI evidence, analysis, and advice artifacts.

## Architecture Role
This test guards the `sqlite_store` integration boundary before analysis and advice depend on it.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable.

## Ownership And Failure Modes
The in-memory database handle is closed. The reopen/foreign-key check uses a relative on-disk database path so CTest can run on Unix-like and Windows hosts. Buffer-too-small cases verify count reporting without ownership transfer.

## Test Strategy
It covers migration, load/save state, sessions, event loading, analysis loading, step loading, and advice storage.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0005-sqlite-local-store.md
