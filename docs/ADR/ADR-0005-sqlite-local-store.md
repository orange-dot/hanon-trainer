# ADR-0005: SQLite Local Store

## Status

Accepted

## Context

The app no longer needs to persist only bookmarks and notes.

The first build now needs durable storage for:

- user preferences and notes
- practice sessions
- captured MIDI events
- aggregate and step-level post-session analysis output
- optional advice artifacts

A single ad hoc state file would turn query, migration, and review-panel loading
into scattered bespoke logic.

## Decision

Use SQLite as the canonical local store from the first implementation pass.

Keep mirrored corpus assets and overlay sidecars on disk as read-only input, but
persist runtime state and generated artifacts in SQLite.

## Consequences

- session history, MIDI capture, aggregate analysis, and step-level analysis
  results share one local queryable store
- migrations become explicit instead of implicit file-shape drift
- the persistence boundary becomes stable enough for the review panel and advice
  workflow
- the runtime still remains local-only and offline-first
