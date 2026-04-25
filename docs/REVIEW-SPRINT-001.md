# Review - Sprint 001 Resolved Architecture Review

This document records the resolved architecture review for the Sprint 001 pilot
analysis documentation plan.

The original review found that the project direction was sound, but that the
documentation needed tighter contracts before implementation tickets could be
written safely. The current documentation has been updated to close those
blocking gaps.

## Current Verdict

The documentation set is ready to publish as a public planning and architecture
pack for a future implementation sprint.

It remains documentation-only. It does not add C source files, build system
files, generated score assets, overlay sidecars, MIDI fixtures, SQLite schemas,
or runtime corpus output.

## Resolved Findings

### Partial Coverage Support Level

The pilot now targets `pilot_analysis`, not strict `analysis_ready`.

`analysis_ready` remains reserved for full exercise scoring coverage. Bounded,
manually reviewed passages use `pilot_analysis`. This keeps the catalog support
level honest and prevents a partial pilot from weakening the meaning of
`analysis_ready`.

Resolved in:

- `docs/CONTENT_PIPELINE.md`
- `docs/DATA_MODEL.md`
- `docs/SPRINT-001-PILOT-TRAINER.md`
- `docs/BACKLOG-REFINEMENT.md`
- `docs/OPEN_QUESTIONS.md`

### Pilot Analysis Naming

The sprint is named and scoped as a pilot analysis proof. It does not claim to
ship the full trainer loop or live MIDI capture.

The implementation slice proves:

- score asset loading
- bounded overlay loading
- fixture MIDI ingestion through SQLite
- deterministic pitch and timing analysis
- minimal score-first viewer behavior
- adapter-level AI degraded behavior

### SQLite Participation

Fixture data enters through SQLite before analysis runs.

The fixture harness inserts a practice session and MIDI events into SQLite.
`performance_analysis` reads from that persisted session and stores aggregate
plus step-level results back into SQLite.

### Score Asset Coordinate Basis

The pilot pins the source PDF, conversion command, output path, and expected
`2480 x 3508` coordinate basis. Overlay records store reference dimensions, and
loaders must reject overlays whose dimensions or regions do not match the
display asset.

### Build Scaffold

The first implementation sprint begins with an explicit build scaffold epic.
That epic fixes CMake, C11 minimum, warning posture, directory layout, public
header posture, and fixture harness location before feature work begins.

### Overlay Sidecar Format

The first implementation pass uses ASCII-only TSV sidecars.

The TSV contract now names:

- required header row
- literal tab separators
- literal newline records
- no tabs or newlines in `step_note`
- space-separated decimal MIDI note numbers in `expected_pitches`
- non-empty `expected_pitches` for scored steps
- ASCII-only content

### AI Stub Placement

AI advice remains stub-only for the first implementation sprint, and the stub
is adapter-level. `codex_cli_adapter` is compiled and called, but returns a
stubbed or unsupported result. `advice_orchestrator` must preserve local
analysis output and persist the advice artifact status.

## Additional Boundary Decisions

The resolved review also locks several C-facing boundaries before public
implementation starts.

### MIDI Capture And Persistence

`midi_capture` owns device enumeration and capture mechanics only. It emits or
polls `ht_midi_event_record` values and does not depend on SQLite.

The UI/session orchestration layer drains captured events and calls
`sqlite_store`. This keeps the compile-time graph acyclic and separates volatile
device I/O from persistence.

Captured MIDI `event_ns` values are session-relative nanoseconds from capture
start. Wall-clock session timestamps remain on the practice session record.

### Step-Level Analysis Results

Aggregate and step-level analysis results are both first-class persisted data.

The public store contract includes explicit APIs for step result storage and
loading, and `analysis_step_results` remains a required SQLite table.

### Typed Codex Advice Boundary

Application code should not call a public raw-prompt Codex API.

The public advice boundary is a typed `ht_advice_request_record` built from
session identity, variant identity, exercise metadata, aggregate metrics,
weak-step summaries, and an optional short user note. Any raw prompt text passed
to the local Codex CLI is internal adapter detail derived from that typed
request.

### Modern C API Posture

The final documentation pass treats the project as a C library-style codebase.
The public surface is specified as C ABI-style headers with `ht_` identifiers,
opaque handles for mutable services, plain transfer records, explicit
`ht_status` returns, and Doxygen-style ownership, size, and precondition
comments.

Public typedef names avoid the reserved `_t` suffix. Public headers must
include the standard headers required by their visible types, including
`<stddef.h>` for `size_t` and `<stdbool.h>` for `bool`.

The docs now specify no-event MIDI polling and capacity/`out_count` behavior
for step-result loading before implementation begins.

## Remaining Implementation Watchpoints

These are not documentation blockers, but they should be checked during the
first implementation sprint:

- `midi_capture_poll_event` must honor the documented `HT_OK` plus
  `out_has_event == false` empty-poll behavior.
- Step-result load APIs must honor the documented capacity, partial-output, and
  `out_count` behavior.
- The advice request builder must be the only place that translates database and
  catalog facts into the Codex prompt payload.
- Fixture MIDI files must use session-relative timestamps so timing tests remain
  deterministic.

## Closing

The architecture now preserves the intended boundaries:

- stable domain libraries are separated from volatile integrations
- public interfaces are explicit and typed
- the library graph remains acyclic
- corpus assets are read-only at runtime
- SQLite owns mutable local state
- Codex advice is explicit, optional, and bounded by a typed request

The remaining work is implementation, not architectural triage.
