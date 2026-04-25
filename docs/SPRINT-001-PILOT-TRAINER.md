# Sprint 001: Pilot Analysis Documentation Plan

This sprint is a planning and review sprint only.

It documents the first implementation slice for a future runnable pilot
analysis proof. It must not add C source files, build system files, generated
score assets, overlay sidecars, MIDI fixtures, SQLite schemas, or runtime
corpus output.

## Goal

Prepare a decision-complete backlog for a first implementation sprint centered
on one pilot variant and one bounded passage:

- source PDF: `corpus/source-pdfs/hanon-exercise-01-c.pdf`
- future `exercise_id`: `hanon-01`
- future `variant_id`: `hanon-01-c`
- future support target: `pilot_analysis`
- strict `analysis_ready` remains reserved for full exercise coverage

The future implementation should prove one narrow analysis loop:

1. load a normalized score asset derived from the source PDF
2. load a manual overlay for a bounded pilot passage
3. insert fixture MIDI events into SQLite
4. run deterministic post-session pitch and timing analysis from SQLite
5. show a minimal score-first viewer for the score and active overlay
6. keep AI advice behind an explicit adapter-level stub boundary

The sprint does not attempt full Hanon corpus support or a live MIDI trainer.

## Intended Implementation Slice

### Epic 0: Build Scaffold

The first implementation sprint must start by creating the scaffold required by
the other epics.

Decisions fixed for that scaffold:

- build system: CMake
- required dialect: C11 minimum
- optional dialect posture: C23 features may be used only behind explicit
  fallback headers
- warning posture: compile cleanly with `-Wall -Wextra -Wpedantic`
- C ABI posture: public headers expose named prototypes, C scalar types,
  opaque handles, plain transfer records, and `ht_status` returns
- independent headers: each public header includes the standard headers needed
  by its visible types, including `<stddef.h>` and `<stdbool.h>` where needed
- public header posture: Doxygen-style comments on every public prototype,
  including ownership, size, and precondition notes
- fixture location: `tests/fixtures/hanon-01-c/`

The scaffold is not accepted until the empty public header layout matches
`LOCAL_SPEC.md`, public headers compile independently as C11, and the fixture
harness can be invoked without real MIDI input.

### Corpus Pilot

The future implementation should promote only `hanon-exercise-01-c.pdf` into
the pilot runtime corpus.

The planned runtime asset is image-first and must pin the conversion command:

```sh
pdftoppm -r 300 -f 1 -l 1 -singlefile corpus/source-pdfs/hanon-exercise-01-c.pdf corpus/runtime/assets/hanon-exercise-01-c
```

The expected output path is:

```text
corpus/runtime/assets/hanon-exercise-01-c.ppm
```

The expected coordinate basis is A4 page 1 at 300 DPI:

- width: `2480` px
- height: `3508` px

The catalog entry must store the display asset path and dimensions. Overlay
records must store matching reference dimensions, and the loader must reject an
overlay whose reference dimensions do not match the asset or whose regions fall
outside asset bounds.

The source PDF remains the visual authority. Musical semantics do not come from
the PDF itself; they come from app-owned overlay and expectation metadata.

### Overlay Pilot

The first overlay describes only a bounded passage from Exercise 01 in C, not
the full exercise. Because coverage is partial, the pilot uses
`pilot_analysis`; it must not claim strict `analysis_ready`.

The selected passage must be no larger than:

- first 4 measures of the visible score, or a smaller reviewer-approved range
- 8 overlay steps

The first implementation pass uses ASCII-only TSV sidecars. The TSV rules are:

- one header row is required
- fields are separated by literal tab characters
- records are separated by literal newline characters
- `step_note` must not contain tabs or newlines
- `expected_pitches` is a space-separated list of decimal MIDI note numbers
- `expected_pitches` must be non-empty for scored steps
- file content is ASCII only

The sidecar must capture at least:

- `step_index`
- `page_index`
- score region as `x`, `y`, `w`, `h`
- `hand_scope`
- `expected_pitches`
- `timing_window_ms`
- short `step_note`

Every expected pitch group must be manually verified against the visible score
before the pilot can be promoted to `pilot_analysis`.

### Analysis Pilot

The first analysis proof uses deterministic MIDI fixtures instead of a physical
MIDI keyboard.

Fixture events enter through SQLite: the fixture harness inserts a practice
session and session-relative MIDI events into the local database, then
`performance_analysis` loads that session from SQLite and stores aggregate and
step-level results back to SQLite. This preserves the future
`Capture -> app/session orchestration -> DB -> Analysis -> DB` path while
keeping live capture out of acceptance.

The analysis target is intentionally narrow:

- pitch group matching
- missed note detection
- extra or wrong note detection
- onset timing deviation against overlay-backed timing windows

Every scored step has exactly one integer `timing_window_ms`; no nested timing
windows or expressive timing grades are part of the pilot.

Real-time coaching, expressive scoring, fingering correctness scoring, live
MIDI capture, and audio waveform analysis remain out of scope.

### Viewer Pilot

The future UI target is a minimal SDL viewer, not the full product shell.

The viewer only needs to prove:

- score image rendering
- active overlay rectangle rendering
- pilot variant identity
- basic step navigation across at most 8 steps
- explicit missing-asset and missing-overlay states

The full browser rail, practice sidebar, review panel, and polished guidance UI
remain later work.

### AI Boundary

AI advice remains stub-only for the first implementation sprint.

The stub is adapter-level: `codex_cli_adapter` is compiled and called, but the
pilot adapter returns an unsupported/stubbed result. `advice_orchestrator`
must preserve local analysis output and store an advice artifact with a
stubbed generation status if advice is requested.

No real Codex invocation is part of acceptance.

## Non-Scope

The following must not be pulled into the first implementation backlog:

- full corpus normalization for all 220 mirrored PDFs
- strict `analysis_ready` promotion for `hanon-01-c`
- public redistribution workflow for source-derived assets
- notation re-engraving
- real MIDI capture as an acceptance blocker
- in-app overlay authoring
- live coaching while capture is active
- real Codex advice generation
- microphone or audio analysis

## Review Gates

The backlog is ready for implementation only when review confirms:

- `hanon-01-c` is the only pilot variant
- `pilot_analysis` is accepted as a partial-coverage support level
- strict `analysis_ready` still means full scored exercise coverage
- the pilot passage boundaries are explicit and within the 4-measure/8-step cap
- the score asset conversion command and `2480 x 3508` coordinate basis are
  accepted
- the ASCII-only TSV sidecar rules are accepted
- fixture events enter through SQLite before analysis runs
- the fixture scenarios are accepted
- the adapter-level AI stub is accepted
- broader corpus support remains deferred

## Acceptance For This Documentation Sprint

This documentation sprint is complete when:

- this plan exists in `docs/`
- backlog refinement criteria exist in `docs/BACKLOG-REFINEMENT.md`
- the resolved architecture review in `docs/REVIEW-SPRINT-001.md` agrees with
  this plan
- supporting spec docs agree on `pilot_analysis`, strict `analysis_ready`,
  pinned asset dimensions, SQLite fixture flow, TSV rules, and AI stub
  placement
- no implementation files or generated assets are added
