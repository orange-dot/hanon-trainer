# session_core.h

## Purpose
Declares the in-memory session selection and target-state boundary.

## Theory
Session state is mutable application state, so it is represented by an opaque handle and explicit setter functions.

## Architecture Role
This boundary is independent of persistence and platform capture. Future UI orchestration can persist selected state through `sqlite_store`.

## Implementation Contract
Create/destroy and setters follow the common handle and borrowed-string rules.

## Ownership And Failure Modes
The caller owns the session handle. Invalid handles, empty tempos, and oversized ids are rejected.

## Test Strategy
Header and build tests cover the interface shape; future UI tests will exercise session transitions.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/DATA_MODEL.md
