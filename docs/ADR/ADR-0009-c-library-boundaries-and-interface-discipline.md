# ADR-0009: C Library Boundaries And Interface Discipline

## Status

Accepted

## Context

This project is a C project, not a C++ codebase.

The implementation needs a structure that fits C well:

- headers for interfaces
- translation units for implementation
- explicit lifetime and ownership rules
- small semantically connected libraries

Without that discipline, SDL2 rendering, MIDI capture, SQLite I/O, analysis, and
Codex integration would quickly collapse into one hard-to-test application
layer.

## Decision

Organize the implementation as small C libraries with explicit public headers.

Use:

- one public header per semantically connected library
- a C ABI-style public surface with named prototypes, C scalar types, opaque
  handles, and plain transfer records
- opaque handles for mutable services
- plain transfer structs for stable data exchange
- explicit status-code returns for fallible operations
- documented ownership rules for every created object and buffer
- Doxygen-style comments on public prototypes, including ownership, size, and
  precondition contracts
- project-owned public identifiers with the `ht_` prefix

Public typedef names must not end in `_t`. Public identifiers must not use
leading underscores and must not rely on reserved C or POSIX naming space.

Public headers must include the standard headers required by their visible
types and must compile independently as C11. Ordinary public calls should be
real function prototypes, not varargs or macro-only entry points.

## Consequences

- the implementation gains clear seams for testing and future refactoring
- integration-heavy code stays isolated from stable domain logic
- header dependencies can remain acyclic
- the docs and code can describe the same public surface cleanly
- identifier ownership remains reviewable instead of being enforced by habit
- C ABI assumptions stay visible at the project boundary
