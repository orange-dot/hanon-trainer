# sqlite_store.h

## Purpose
Declares the SQLite persistence boundary for local state, sessions, events, analysis, and advice artifacts.

## Theory
SQLite is the canonical local store for user-generated evidence and results, while corpus files remain read-only on disk.

## Architecture Role
This integration boundary is consumed by performance analysis and advice orchestration.

## Implementation Contract
All operations return `ht_status`. Load APIs use caller-owned buffers and report available counts when known.

## Ownership And Failure Modes
The database handle owns the SQLite connection. Database failures map to `HT_ERR_DB`; buffer shortages return `HT_ERR_BUFFER_TOO_SMALL`.

## Test Strategy
`tests/test_sqlite_store.c` covers migration, state, sessions, events, analysis, step results, and advice persistence.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0005-sqlite-local-store.md
