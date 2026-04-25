# Local Spec

This document fixes the local implementation contract for the first build.

It is intentionally concrete about C-facing interfaces, runtime configuration,
SQLite ownership, and the Codex CLI boundary.

## C Baseline

The first implementation pass targets C11 as the minimum dialect.

C23 features may be used only behind explicit fallback headers so the codebase
can still build on a C11-capable toolchain. Public headers should compile
cleanly with `-Wall -Wextra -Wpedantic`.

Public headers that use `bool` must include `<stdbool.h>` and remain valid C11.
Public headers that expose `size_t` must include `<stddef.h>`. Any other
standard type used in a public prototype or record must be provided by the
header itself, so each public header can be compiled independently.

The public surface is a C ABI-style API: named function prototypes in headers,
C scalar types, pointers to opaque structs for mutable services, and plain
`ht_` transfer records for data exchange. No public interface should depend on
language features outside C or macro-only call sites for ordinary operations.

## Directory And Naming Shape

Recommended project layout:

```text
include/hanon_trainer/
  ht_types.h
  content_catalog.h
  overlay_store.h
  session_core.h
  midi_capture.h
  sqlite_store.h
  performance_analysis.h
  codex_cli_adapter.h
  advice_orchestrator.h
  score_renderer.h
  guidance_renderer.h
src/
  *.c
assets/
corpus/
```

Naming rules:

- public types and functions use the `ht_` prefix
- one public header per semantically connected library
- mutable services are opaque handles
- transfer records are plain structs
- public typedef names must not end in `_t`
- public identifiers must not use leading underscores or C/POSIX-reserved names
- public function prototypes use Doxygen-style comments for ownership, size,
  and precondition contracts

## Common Types

The implementation should define at least these common surfaces in `ht_types.h`:

- `ht_status`
- `ht_support_level`
- `ht_capture_status`
- `ht_analysis_status`
- `ht_generation_status`
- `ht_midi_device_record`
- `ht_codex_result`
- `ht_advice_request_record`
- transfer records for variants, overlay steps, sessions, events, analysis, and
  advice

Recommended `ht_status` members:

- `HT_OK`
- `HT_ERR_INVALID_ARG`
- `HT_ERR_NOT_FOUND`
- `HT_ERR_IO`
- `HT_ERR_DB`
- `HT_ERR_UNSUPPORTED`
- `HT_ERR_TIMEOUT`
- `HT_ERR_EXTERNAL_TOOL`
- `HT_ERR_CORRUPT_DATA`
- `HT_ERR_BUFFER_TOO_SMALL`

Recommended `ht_support_level` members:

- `HT_SUPPORT_ASSET_ONLY`
- `HT_SUPPORT_GUIDE_ONLY`
- `HT_SUPPORT_PILOT_ANALYSIS`
- `HT_SUPPORT_ANALYSIS_READY`

Recommended `ht_generation_status` members:

- `HT_GENERATION_COMPLETE`
- `HT_GENERATION_TIMEOUT`
- `HT_GENERATION_FAILED`
- `HT_GENERATION_STUBBED`

`HT_SUPPORT_ANALYSIS_READY` is reserved for full exercise scoring coverage.
`HT_SUPPORT_PILOT_ANALYSIS` is for reviewed bounded passages only.

## Public API Contract

All public headers must make ownership and lifetime visible from the prototype
and its comment block.

Handle rules:

- `*_open` and `*_create` functions take `T** out_handle`.
- On success, the function stores a non-null handle and the caller owns it.
- On failure, the function stores `NULL` when `out_handle` is non-null.
- Handles are released only by the paired `*_close` or `*_destroy` function.
- `*_close` and `*_destroy` functions accept `NULL` and return `void`.

Record and buffer rules:

- Transfer records passed as `const*` are borrowed for the duration of the call.
- Output records are caller-owned and are fully initialized on `HT_OK`.
- On non-`HT_OK`, output records are zeroed unless that function documents an
  informative output such as a required element count.
- `capacity` values are element counts, not byte counts.
- `out_count` reports the number of available records when that can be known,
  even if the caller-provided buffer was too small.
- A buffer that is too small returns `HT_ERR_BUFFER_TOO_SMALL`; partial output
  is allowed only when the function comment says so.

String rules:

- `char const*` identifiers and paths are borrowed NUL-terminated strings.
- A module that needs a string after the call returns must copy it into
  module-owned storage.

## Required Public Headers

`content_catalog.h`

- `ht_status ht_catalog_open(ht_catalog** out_catalog, char const* corpus_root);`
- `void ht_catalog_close(ht_catalog* catalog);`
- `ht_status ht_catalog_lookup_variant(ht_catalog const* catalog, char const* variant_id, ht_variant_record* out_variant);`

`overlay_store.h`

- `ht_status ht_overlay_store_open(ht_overlay_store** out_store, char const* corpus_root);`
- `void ht_overlay_store_close(ht_overlay_store* store);`
- `ht_status ht_overlay_store_get_overlay(ht_overlay_store const* store, char const* overlay_id, ht_overlay_record* out_overlay);`
- `ht_status ht_overlay_store_get_step(ht_overlay_store const* store, char const* overlay_id, size_t step_index, ht_overlay_step_record* out_step);`

`session_core.h`

- `ht_status ht_session_create(ht_session** out_session);`
- `void ht_session_destroy(ht_session* session);`
- `ht_status ht_session_select_variant(ht_session* session, char const* variant_id);`
- `ht_status ht_session_set_target_tempo(ht_session* session, unsigned target_tempo);`
- `ht_status ht_session_arm_device(ht_session* session, char const* midi_device_id);`

`midi_capture.h`

- `ht_status ht_midi_capture_open(ht_midi_capture** out_capture);`
- `void ht_midi_capture_close(ht_midi_capture* capture);`
- `ht_status ht_midi_capture_list_devices(ht_midi_capture const* capture, ht_midi_device_record* out_devices, size_t capacity, size_t* out_count);`
- `ht_status ht_midi_capture_start(ht_midi_capture* capture, char const* device_id, char const* session_id);`
- `ht_status ht_midi_capture_poll_event(ht_midi_capture* capture, ht_midi_event_record* out_event, bool* out_has_event);`
- `ht_status ht_midi_capture_stop(ht_midi_capture* capture);`

`midi_capture` does not own persistence and does not depend on `ht_db`. The
app/session orchestration layer drains captured events and calls
`sqlite_store`.

`ht_midi_capture_poll_event` returns `HT_OK` with `*out_has_event == false`
when no event is currently available. An empty poll is not an error and must
not write a meaningful event record.

`ht_midi_capture_list_devices` treats `capacity` as an element count. It writes
at most `capacity` records, sets `out_count` to the number of available devices
when discoverable, and returns `HT_ERR_BUFFER_TOO_SMALL` if more devices exist
than fit in the provided buffer.

`sqlite_store.h`

- `ht_status ht_db_open(ht_db** out_db, char const* database_path);`
- `void ht_db_close(ht_db* db);`
- `ht_status ht_db_migrate(ht_db* db);`
- `ht_status ht_db_load_user_state(ht_db* db, ht_user_state_record* out_state);`
- `ht_status ht_db_save_user_state(ht_db* db, ht_user_state_record const* state);`
- `ht_status ht_db_insert_session(ht_db* db, ht_session_record const* session);`
- `ht_status ht_db_load_session(ht_db* db, char const* session_id, ht_session_record* out_session);`
- `ht_status ht_db_append_midi_event(ht_db* db, ht_midi_event_record const* event);`
- `ht_status ht_db_load_midi_events(ht_db* db, char const* session_id, ht_midi_event_record* out_events, size_t capacity, size_t* out_count);`
- `ht_status ht_db_store_analysis(ht_db* db, ht_analysis_record const* analysis);`
- `ht_status ht_db_load_analysis(ht_db* db, char const* session_id, ht_analysis_record* out_analysis);`
- `ht_status ht_db_store_analysis_step(ht_db* db, ht_analysis_step_record const* step);`
- `ht_status ht_db_load_analysis_steps(ht_db* db, char const* session_id, ht_analysis_step_record* out_steps, size_t capacity, size_t* out_count);`
- `ht_status ht_db_store_advice(ht_db* db, ht_advice_record const* advice);`

`ht_db_load_session`, `ht_db_load_midi_events`, and `ht_db_load_analysis` are
read-side helpers required by `performance_analysis` and `advice_orchestrator`.
They follow the same caller-owned output and count-reporting rules as the
original store API.

`ht_db_load_analysis_steps` writes at most `capacity` step records, sorted by
`step_index`. It sets `out_count` to the number of stored step results for the
session when that count is available. If the caller buffer is too small, it
returns `HT_ERR_BUFFER_TOO_SMALL` and may leave partial records in
`out_steps`.

`performance_analysis.h`

- `ht_status ht_analysis_run_session(ht_db* db, ht_catalog const* catalog, ht_overlay_store const* overlays, char const* session_id, ht_analysis_record* out_analysis);`

`codex_cli_adapter.h`

- `ht_status ht_codex_cli_open(ht_codex_cli** out_cli, char const* executable_path, char const* model_override, unsigned timeout_ms);`
- `void ht_codex_cli_close(ht_codex_cli* cli);`

`codex_cli_adapter` is an internal process adapter. It should not expose a
general public raw-prompt API for application code.

`advice_orchestrator.h`

- `ht_status ht_advice_build_request(ht_db* db, ht_catalog const* catalog, char const* session_id, ht_advice_request_record* out_request);`
- `ht_status ht_advice_generate_for_session(ht_db* db, ht_catalog const* catalog, ht_codex_cli* cli, char const* session_id, ht_advice_record* out_advice);`

Rendering headers may expose renderer-local init/draw/shutdown functions, but
they must not own persistence or analysis state.

## Runtime Configuration Contract

The runtime must expose explicit configuration for:

- `corpus_root`
- `database_path`
- `codex_executable_path`
- optional `codex_model_override`
- `codex_timeout_ms`
- preferred `midi_device_id`

These are public runtime knobs, not hidden implementation details.

## SQLite Contract

SQLite is the canonical local store for:

- user state
- practice sessions
- captured MIDI events
- analysis summaries
- step-level analysis results
- Codex advice artifacts

Recommended tables:

- `user_state`
- `practice_sessions`
- `midi_events`
- `analysis_results`
- `analysis_step_results`
- `advice_artifacts`

The corpus itself remains outside SQLite and is loaded read-only from the local
filesystem.

Fixture-based analysis tests use the same persistence path as future live MIDI
capture: fixtures insert a practice session and MIDI events into SQLite before
`performance_analysis` runs, and aggregate plus step-level analysis results are
stored back into SQLite.

MIDI event timestamps use session-relative nanoseconds. `event_ns` is measured
from the start of the capture session, not from an absolute host monotonic
clock. Event ordering is stable by `(session_id, sequence_no)`, while timing
analysis uses `event_ns`.

## Codex Prompt Contract

The public Codex advice boundary is a typed request record, not arbitrary prompt
text. `ht_advice_request_record` is built from:

- session id and variant id
- exercise number and title
- key name and target tempo
- support level
- aggregate pitch metrics
- aggregate timing metrics
- weak-step summaries
- short user note when present

The generated prompt consumed by the internal CLI adapter must be derived only
from that typed request. It must not receive the whole workspace, raw mirrored
pages, or hidden ambient context.

The generated advice artifact should preserve:

- prompt version
- command line used
- generation status
- returned markdown or plain text

## Degraded Modes

- If SQLite cannot open, the app must surface a persistence failure instead of
  pretending sessions are saved.
- If a variant lacks overlay expectations, the app may still render the score
  but must disable scored analysis.
- If Codex fails, the app must still show local metrics.
- If the Codex adapter is stubbed, the app must store a stubbed advice status
  without hiding local metrics.
- If no MIDI device is armed, the app remains usable as a score viewer and
  guide.
