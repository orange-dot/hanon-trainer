# ht_types.h

## Purpose
Defines common status codes, support levels, opaque handles, and plain transfer records used across the library.

## Theory
Shared records keep module boundaries explicit without exposing mutable service internals. Names stay project-owned with `ht_` and avoid reserved `_t` suffixes.

## Architecture Role
This is the root public header in the acyclic C dependency graph.

## Implementation Contract
The header includes standard headers for all visible types and compiles independently as C11.

## Ownership And Failure Modes
Records are caller-owned data carriers. String storage inside records is fixed-capacity and copied by modules that retain values.

## Test Strategy
Header independence, identifier discipline, and runtime tests all include this contract directly or transitively.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md
