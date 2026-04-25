# performance_analysis.h

## Purpose
Declares post-session analysis from persisted MIDI events to aggregate and step-level results.

## Theory
Analysis is deterministic local computation over stored evidence and reviewed overlay expectations.

## Architecture Role
This header coordinates `sqlite_store`, `content_catalog`, and `overlay_store` without owning long-lived state.

## Implementation Contract
The function loads session facts and events from SQLite, resolves the catalog overlay, stores results, and optionally returns the aggregate record.

## Ownership And Failure Modes
All handles are borrowed. Missing overlay coverage returns unsupported rather than inventing scoring data.

## Test Strategy
`tests/test_performance_analysis.c` covers perfect, wrong-note, missing-note, and late-onset synthetic cases.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0007-post-session-pitch-and-timing-analysis.md
