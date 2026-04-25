# Backlog Refinement

This document records the review checklist that should be completed before the
first Hanon pilot analysis implementation sprint starts.

The target implementation is the Exercise 01 in C pilot described in
[Sprint 001](SPRINT-001-PILOT-TRAINER.md).

## Refinement Outcome

Backlog refinement should produce implementation tickets that are small enough
to build and review independently while preserving one vertical slice:

`source PDF -> pinned score asset -> overlay -> fixture MIDI events -> SQLite -> local analysis -> minimal viewer`

The refinement should avoid creating tickets for full corpus coverage, live
MIDI capture, real AI advice, or broad product polish before the pilot proves
the local analysis loop.

## Epic Breakdown

### Epic 0: Build Scaffold

Tickets must define:

- CMake as the build system
- C11 as the minimum C dialect
- C23 usage only behind explicit fallback headers
- clean compilation with `-Wall -Wextra -Wpedantic`
- directory layout matching `LOCAL_SPEC.md`
- C ABI-style public headers with named prototypes, opaque handles, plain
  records, `ht_status` returns, and no varargs for ordinary calls
- independent public headers that include their own visible standard types
- public headers with Doxygen-style ownership, size, and precondition comments
- fixture harness root: `tests/fixtures/hanon-01-c/`
- test command shape for running fixture analysis without real MIDI input

Acceptance threshold:

- an empty scaffold can build and run the fixture harness command in a no-op
  mode without adding generated corpus assets.

### Corpus Pilot

Tickets must define:

- exact source PDF: `corpus/source-pdfs/hanon-exercise-01-c.pdf`
- runtime score asset path: `corpus/runtime/assets/hanon-exercise-01-c.ppm`
- conversion command:
  `pdftoppm -r 300 -f 1 -l 1 -singlefile corpus/source-pdfs/hanon-exercise-01-c.pdf corpus/runtime/assets/hanon-exercise-01-c`
- expected asset dimensions: `2480 x 3508` px
- future catalog entry for `hanon-01-c`
- catalog fields for display asset dimensions

Acceptance threshold:

- the generated score asset exists, has the expected dimensions, and can be
  used as the stable overlay coordinate basis.

### Overlay Pilot

Tickets must define:

- bounded passage selected from Exercise 01 in C
- maximum scope: first 4 measures and no more than 8 overlay steps
- support-level promotion target: `pilot_analysis`
- strict `analysis_ready` remains unavailable until the full exercise is
  covered
- ASCII-only TSV sidecar rules
- manual transcription responsibility for expected pitch groups
- overlay reference dimensions matching the score asset dimensions
- coordinate validation rule for all score regions

Acceptance threshold:

- every overlay region is inside `2480 x 3508` bounds, every scored step has a
  non-empty pitch group, and every scored step has exactly one integer
  `timing_window_ms`.

### Analysis Pilot

Tickets must define:

- fixture MIDI event format under `tests/fixtures/hanon-01-c/`
- fixture ingestion into SQLite before analysis
- analyzer reads the practice session and MIDI events from SQLite
- analyzer stores aggregate and step-level results back to SQLite through
  explicit store APIs
- fixture event timestamps use session-relative nanoseconds
- perfect fixture
- wrong-note fixture
- missing-note fixture
- late-onset fixture
- aggregate analysis fields to persist or display

Acceptance threshold:

- the perfect fixture produces no pitch errors
- the wrong-note fixture reports a wrong or extra note
- the missing-note fixture reports a missed note
- the late-onset fixture reports timing deviation
- all analysis results are explainable from the overlay and fixture events
  alone

### Viewer Pilot

Tickets must define:

- minimum SDL viewer surface
- score asset loading from the pinned PPM path
- active overlay rectangle selection
- step navigation across at most 8 steps
- visible pilot variant identity
- missing-asset and missing-overlay degraded states

Acceptance threshold:

- the viewer renders the score asset and at least one overlay rectangle without
  requiring real MIDI input or real AI advice.

### AI Boundary

Tickets must define:

- adapter-level stub in `codex_cli_adapter`
- stubbed or unsupported generation status
- compact prompt payload fields that will eventually be used
- advice artifact behavior when the adapter is unavailable
- rule that local analysis must pass without AI availability

Acceptance threshold:

- requesting advice exercises the adapter-level stub path and does not fail or
  hide local analysis results.

## Required Acceptance Scenarios

The future implementation sprint should not be accepted without these scenarios:

- catalog lookup resolves `hanon-01-c`
- source PDF exists at the expected path
- score asset exists after offline generation
- score asset dimensions are exactly `2480 x 3508`
- overlay reference dimensions match the score asset
- overlay regions fall inside score asset bounds
- pilot support level is `pilot_analysis`, not strict `analysis_ready`
- fixture events are inserted into SQLite before analysis
- fixture event timestamps are session-relative nanoseconds
- perfect fixture produces no pitch errors
- wrong-note fixture reports a wrong or extra note
- missing-note fixture reports a missed note
- late-onset fixture reports timing deviation
- analysis results are stored back into SQLite
- step-level analysis results are stored back into SQLite
- viewer renders the score asset and at least one overlay rectangle
- advice path reports a stubbed or unavailable status without failing analysis

## Decisions To Keep Fixed

- The pilot variant is `hanon-01-c`.
- The first implementation is a pilot analysis proof, not a full trainer.
- PDF content is visual source material, not machine-readable music semantics.
- Overlay metadata is the authority for analysis expectations.
- Partial passage coverage uses `pilot_analysis`.
- Strict `analysis_ready` means the whole exercise can be scored.
- Fixture-based MIDI analysis is the first proof.
- Fixture data enters through SQLite before analysis runs.
- Real MIDI capture can follow after the fixture path is trusted.
- AI/Codex advice is explicit, adapter-level, and stub-only for the first pilot.

## Review Exit Criteria

Backlog refinement is complete when every implementation ticket can be assigned
without asking:

- which variant is in scope
- which source file is authoritative
- what support level the pilot uses
- what strict `analysis_ready` means
- what the overlay must contain
- which score asset dimensions define the coordinate basis
- how TSV sidecars encode pitch lists and notes
- where fixtures live
- whether fixture events go through SQLite
- what fixture behavior proves analysis
- what UI surface is sufficient
- where the AI stub is placed
- whether AI is required for acceptance
