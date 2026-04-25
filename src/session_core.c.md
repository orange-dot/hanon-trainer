# session_core.c

## Purpose
Implements the in-memory session handle and simple setter functions.

## Theory
Session selection and user choices belong in mutable app state, separate from persistence and capture.

## Architecture Role
This source backs the `session_core.h` boundary.

## Implementation Contract
It copies borrowed ids into fixed-capacity handle storage and validates target tempo.

## Ownership And Failure Modes
The caller owns the session handle. Oversized strings return buffer status and do not escape borrowed storage.

## Test Strategy
The interface is covered by build and header tests; behavior will expand with UI orchestration tests.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/DATA_MODEL.md
