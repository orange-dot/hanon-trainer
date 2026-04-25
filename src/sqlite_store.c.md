# sqlite_store.c

## Purpose
Implements SQLite migration and persistence APIs.

## Theory
The local database owns user-generated state and evidence, while corpus and overlay files stay read-only.

## Architecture Role
This source backs `sqlite_store.h` and is consumed by analysis and advice orchestration.

## Implementation Contract
It creates the documented tables, uses prepared statements for values, and implements count-aware buffer loading.

## Ownership And Failure Modes
The database handle owns `sqlite3*`. SQLite errors map to `HT_ERR_DB`; caller buffers remain caller-owned.

## Test Strategy
`tests/test_sqlite_store.c` covers schema migration and all public persistence functions used in the slice.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0005-sqlite-local-store.md
