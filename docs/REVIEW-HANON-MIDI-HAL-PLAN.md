# Review - Hanon Trainer Cross-OS MIDI HAL Plan

This review evaluates the proposed cross-OS MIDI HAL plan for Hanon Trainer.
It is reviewed against the current implementation in:

- `include/hanon_trainer/midi_capture.h`
- `include/hanon_trainer/ht_types.h`
- `src/midi_capture.c`
- `src/midi_decode.c`
- `src/midi_decode.h`
- `tools/ht_midi_probe.c`
- `CMakeLists.txt`
- `.github/workflows/ci.yml`
- `docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md`
- `docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md`

Modern C Library lens used:

- Chapter 6: opaque pointer boundaries, private structure layout, initialized
  pointer state, and transfer records.
- Chapter 7: complete prototypes, explicit input/output contracts, and
  consistent status-return behavior.
- Chapter 9: readable local style, project-owned naming, and minimal global
  namespace exposure.

## Verdict

The plan is conditionally acceptable.

The architectural direction is right: keep `midi_capture.h` as the stable public
boundary, keep `midi_decode` platform-neutral, push ALSA/CoreMIDI/WinMM into
private backends, and make `ht_midi_probe` consume the same private HAL instead
of carrying its own ALSA implementation.

Before implementation, the plan needs tighter contracts in four areas:

- exact private HAL ownership and timing semantics
- cross-platform waiting without leaking POSIX polling into the tool
- realistic device ID stability wording
- build/test layout for native backends plus deterministic fake backend tests

## What The Plan Gets Right

- It preserves the public `ht_midi_capture` opaque handle and the plain
  `ht_midi_device_record` / `ht_midi_event_record` transfer records.
- It keeps OS types out of public headers and out of `midi_decode`.
- It removes the duplicate ALSA event-decoding path from `ht_midi_probe`, which
  is the right way to avoid divergent live-capture behavior.
- It keeps raw replay as a platform-neutral test and fixture path.
- It names the right CI proof: cross-OS build and replay, not live hardware.
- It correctly treats program change, channel pressure, key pressure, pitchbend,
  and sysex as inputs that must be tested even though the public event enum only
  exposes note on, note off, control change, and other.

## Blocking Improvements

### B1. Define "no public API change" as no type/prototype change

`include/hanon_trainer/midi_capture.h` currently says live capture is
unsupported and that device listing does not enumerate live ALSA devices. The
HAL plan cannot keep those comments unchanged.

Proposed improvement:

- Keep public types, function names, arguments, and return types unchanged.
- Update public comments to describe real device listing, supported start, empty
  poll behavior, session-relative `event_ns`, and idempotent stop.
- Update companion docs such as `include/hanon_trainer/midi_capture.h.md`,
  `src/midi_capture.c.md`, `tools/ht_midi_probe.c.md`, and
  `tests/test_platform_boundaries.c.md` so they no longer describe an ALSA-only
  stub.

### B2. Use project-owned private external names

The private header will be included by multiple translation units, so its
functions have external linkage even though the header is not installed.
Chapter 9's namespace guidance still applies.

Proposed improvement:

- Prefer `ht_midi_hal_` for functions/types declared in `src/midi_hal.h`.
- Reserve unprefixed `midi_hal_` only for `static` helpers inside one `.c` file.
- Keep all backend-only helpers `static`.
- Do not expose `_t` typedef names.

Recommended private names:

```c
typedef struct ht_midi_hal ht_midi_hal;
typedef struct ht_midi_hal_event ht_midi_hal_event;

ht_status ht_midi_hal_open(ht_midi_hal** out_hal);
void ht_midi_hal_close(ht_midi_hal* hal);
ht_status ht_midi_hal_list_ports(ht_midi_hal* hal,
                                 ht_midi_device_record* out_ports,
                                 size_t capacity,
                                 size_t* out_count);
ht_status ht_midi_hal_connect(ht_midi_hal* hal, char const* device_id);
ht_status ht_midi_hal_disconnect(ht_midi_hal* hal);
ht_status ht_midi_hal_poll_event(ht_midi_hal* hal,
                                 ht_midi_hal_event* out_event,
                                 bool* out_has_event);
ht_status ht_midi_hal_wait_event(ht_midi_hal* hal, int timeout_ms);
char const* ht_midi_hal_backend_name(void);
```

### B3. Add a private wait contract

The proposed HAL API has only `poll_event`. That is enough for
`ht_midi_capture_poll_event`, but it is not enough for `ht_midi_probe` without
either busy-spinning or reintroducing OS-specific polling in the tool.

Current `ht_midi_probe.c` directly includes POSIX headers such as `poll.h`,
`sysexits.h`, and `time.h`. Windows will not accept that shape.

Proposed improvement:

- Add `ht_midi_hal_wait_event(hal, timeout_ms)` or make `poll_event` accept a
  timeout.
- Keep `ht_midi_capture_poll_event` public and nonblocking.
- Let `ht_midi_probe` implement duration handling with HAL wait plus monotonic
  elapsed time, not with ALSA descriptors or POSIX `poll`.
- Replace `sysexits.h` with a local `HT_EX_USAGE 64` constant or a CMake
  feature check.

### B4. Pin HAL event shape explicitly

"Raw MIDI bytes" is underspecified because the public record only stores
`status_byte`, `data_1`, and `data_2`.

Proposed improvement:

- Define `ht_midi_hal_event` with `event_ns`, `raw_bytes`, and
  `raw_byte_count`.
- Set an explicit raw capacity. At minimum it must handle 3-byte channel voice
  messages; if sysex payloads are not carried, say so directly.
- For sysex, either:
  - carry only `0xF0` plus byte count as metadata, matching the current decoder
    intent, or
  - carry bounded sysex bytes and return `HT_ERR_BUFFER_TOO_SMALL` or
    `HT_ERR_CORRUPT_DATA` for over-capacity payloads.
- Document that public `ht_midi_event_record` maps program change, pressure,
  pitchbend, sysex, realtime, and unknown messages to
  `HT_MIDI_EVENT_OTHER` while preserving status/data bytes where available.

Suggested private event:

```c
#define HT_MIDI_HAL_RAW_CAPACITY 3u

struct ht_midi_hal_event {
    int64_t event_ns;
    unsigned char raw_bytes[HT_MIDI_HAL_RAW_CAPACITY];
    size_t raw_byte_count;
};
```

If full sysex capture is in scope, do not use capacity 3. Add a larger private
capacity and a truncation/error policy.

### B5. Define timestamp ownership once

The plan says HAL emits session-relative timestamps and `midi_capture_start`
also resets timing state. That can work, but only if the ownership is exact.

Proposed improvement:

- `ht_midi_hal_connect` establishes the backend capture epoch.
- Every HAL event returns `event_ns` relative to that connection.
- `ht_midi_capture_start` stores `session_id`, resets `sequence_no`, calls
  `ht_midi_hal_connect`, and copies HAL `event_ns` directly into the public
  record.
- Backends may use native event timestamps when reliable. Otherwise they sample
  a monotonic clock when the event is received.
- Timestamps must be nonnegative and should be nondecreasing per connection.
- `ht_midi_probe` prints milliseconds as `event_ns / 1000000`.

### B6. Be honest about device ID stability

The plan asks for a stable `device_id`, but ALSA `client:port` and WinMM device
indexes are not durable identifiers across all reboots and hotplug events.

Proposed improvement:

- Define `device_id` as a backend-qualified selector stable for the current
  enumeration-to-connect workflow, not as a permanent hardware UUID.
- Use backend prefixes: `alsa:client:port`, `coremidi:unique-id`, and
  `winmm:index` or `winmm:name-hash:index`.
- Document that a previously stored `last_midi_device_id` can become stale and
  `ht_midi_capture_start` then returns `HT_ERR_NOT_FOUND`.
- Make `is_default` "at most one true, possibly none"; do not synthesize a
  default unless the backend has a meaningful one.

### B7. Specify active-state behavior

`stop` is idempotent, but `start` while already active is not defined.

Proposed improvement:

- Pick one behavior and test it.
- Recommended without adding a public status enum: `ht_midi_capture_start`
  returns `HT_ERR_INVALID_ARG` when capture is already active, and the caller
  must call `ht_midi_capture_stop` first.
- `ht_midi_capture_close` must disconnect before freeing.
- On failed `start`, the capture object remains idle and does not keep a
  partially connected backend.

### B8. Validate fixed-size public string fields

`session_id` and `device_id` are copied into fixed-size public records.

Proposed improvement:

- Reject empty `session_id` and empty `device_id`.
- Reject values that do not fit in the destination buffer with
  `HT_ERR_BUFFER_TOO_SMALL`.
- Define these as preconditions in the public header comments.
- Keep `out_*` arguments initialized to neutral values on all status-return
  paths where the pointer itself is valid.

### B9. Split native backend compile proof from fake backend behavior tests

The fake backend test path is necessary, but it should not replace native
backend build coverage.

Proposed improvement:

- Add a CMake option such as `HT_MIDI_BACKEND=AUTO|ALSA|COREMIDI|WINMM|FAKE`.
- Default `AUTO` selects the native backend for the platform.
- In CI, build once with `AUTO` to prove native compile/link.
- In CI, build or test once with `FAKE` to run deterministic device and event
  behavior tests.
- Do not require live MIDI hardware in CI.

The fake backend should provide:

- deterministic port list
- no-event behavior before `start`
- note on/off, control change, program change, key pressure, channel pressure,
  pitchbend, sysex/other, and empty poll
- monotonically increasing `event_ns`
- an optional overflow or malformed-event path if the HAL queue is bounded

### B10. Treat CMake cross-OS as wider than MIDI

Removing `if(NOT UNIX) FATAL_ERROR` is necessary, but not sufficient. The
current root CMake also assumes `PkgConfig`, SDL2, SDL2_ttf, SQLite, ALSA,
POSIX feature macros, and Unix-ish tool behavior.

Proposed improvement:

- Make MIDI backend selection platform-aware in its own target block.
- Apply `_DEFAULT_SOURCE` and `_POSIX_C_SOURCE=200809L` only to Unix-like
  targets that need them.
- Replace `pkg_check_modules(ALSA REQUIRED alsa)` with Linux-only ALSA lookup.
- Link macOS with `CoreMIDI` and `CoreFoundation`.
- Link Windows with `winmm`.
- Decide how SQLite and SDL2 are found on macOS and Windows before expanding
  the CI matrix. Use platform packages, CMake package configs, or an explicit
  reduced CI target set.
- Avoid making `hanon_trainer` link every MIDI backend dependency. It should
  link exactly the selected backend implementation.

## Proposed HAL Contract

The implementation should make `src/midi_capture.c` the only adapter from
private HAL records to public capture records.

Recommended contract:

- `ht_midi_hal_open` allocates backend state and sets `*out_hal = NULL` before
  validation.
- `ht_midi_hal_close(NULL)` is allowed.
- `ht_midi_hal_list_ports` follows the existing capacity contract:
  `capacity > 0` requires `out_ports != NULL`; `out_count` is required; output
  count is set on success.
- `ht_midi_hal_connect` opens a single active input stream for one listed
  `device_id`.
- `ht_midi_hal_disconnect` is idempotent for a non-null HAL handle.
- `ht_midi_hal_poll_event` is nonblocking and returns `HT_OK` plus
  `out_has_event == false` for empty queues.
- `ht_midi_hal_wait_event` blocks up to `timeout_ms` and returns `HT_OK` when
  the wait completed, even if no event arrived.
- `ht_midi_hal_backend_name` returns static storage: `alsa`, `coremidi`,
  `winmm`, `fake`, or `none`.

`src/midi_hal.h` should include only standard C headers needed for visible
types and `hanon_trainer/ht_types.h`. It must not include ALSA, CoreMIDI, or
Windows headers.

## Proposed Capture Mapping

`ht_midi_capture_poll_event` should:

- call `ht_midi_hal_poll_event`
- decode raw bytes through `ht_midi_decode_raw_event`
- copy the current `session_id`
- increment `sequence_no` only when a public event is emitted
- set `event_ns` from the HAL event
- copy `status_byte`, `data_1`, and `data_2` from raw bytes when present
- map decoded kinds as:
  - `HT_MIDI_DECODED_NOTE_ON` -> `HT_MIDI_EVENT_NOTE_ON`
  - `HT_MIDI_DECODED_NOTE_OFF` -> `HT_MIDI_EVENT_NOTE_OFF`
  - `HT_MIDI_DECODED_CONTROLLER` -> `HT_MIDI_EVENT_CONTROL_CHANGE`
  - all other decoded kinds -> `HT_MIDI_EVENT_OTHER`

This keeps the public enum stable while still preserving raw status/data bytes
for later analysis or diagnostics.

## Proposed Probe Changes

`tools/ht_midi_probe.c` should keep the same CLI, but its platform boundary
should shrink.

Proposed behavior:

- `--version` prints:

```text
hanon-trainer <version>
MIDI backend: <backend>
```

- `--list` opens HAL, lists ports, and prints the existing
  `port\tclient\tname`-style output or a documented compatible replacement.
- `--port` connects through HAL, waits through HAL, decodes with
  `midi_decode`, and prints the existing table/TSV event shape.
- `--replay-raw-tsv` does not open HAL and does not require hardware.
- Runtime errors begin with `midi:`. Backend detail can follow, for example:
  `midi: cannot connect (alsa): <detail>`.

The plan should decide whether `--list` output keeps the literal `port`,
`client`, and `name` columns on every OS or moves to backend-neutral columns
such as `device_id`, `backend`, and `name`. Keeping the current header is less
disruptive, but backend-neutral columns are cleaner for CoreMIDI and WinMM.

## Proposed Test Updates

Required C tests:

- Update `tests/test_platform_boundaries.c` so it no longer asserts
  `device_count == 0`.
- Add invalid argument coverage for list capacity/null combinations.
- Add empty poll coverage before start and after stop.
- Add start-on-active behavior coverage.
- Add fake backend capture mapping coverage for all MIDI classes listed above.

Required Python/CLI tests:

- Update `tests/tools/check_midi_probe_cli.py` to look for
  `MIDI backend:` instead of `ALSA`.
- Require `midi:` for runtime errors.
- Keep raw replay tests on every OS.
- Add fake backend `--list` and `--port` tests when configured with
  `HT_MIDI_BACKEND=FAKE`.

Required CI:

- Ubuntu native backend build and replay.
- macOS native backend build and replay.
- Windows native backend build and replay.
- Fake backend deterministic behavior, either as a second build or a dedicated
  test target.

## Implementation Order

Recommended order:

1. Add `src/midi_hal.h` with the exact private contract and a fake backend.
2. Move probe replay-only decode paths away from ALSA dependencies.
3. Refactor `src/midi_capture.c` to use the fake HAL and add mapping tests.
4. Add ALSA backend and make Linux native build pass.
5. Refactor `tools/ht_midi_probe.c` to use HAL for list/capture.
6. Add CoreMIDI backend and macOS CI.
7. Add WinMM backend and Windows CI.
8. Update public comments and companion docs.

This order gives deterministic tests before the native backends and prevents
the cross-OS work from being blocked by live hardware.

## Residual Risks

- CoreMIDI and WinMM timestamp semantics differ from ALSA; each backend needs a
  short note in code comments explaining its timestamp source.
- WinMM capture is callback-based, so the backend probably needs a small
  bounded queue. The overflow behavior must return a status or emit a clear
  diagnostic path.
- "Stable device ID" can become misleading if the UI persists it as a permanent
  device identity. Document stale IDs and test `HT_ERR_NOT_FOUND`.
- Full sysex capture is not represented by the current public event record. If
  sysex payloads matter later, that is a separate public API discussion.

## Acceptance Checklist

- Public MIDI capture prototypes unchanged.
- Public MIDI capture comments updated.
- No OS MIDI headers included outside backend `.c` files.
- `midi_decode` stays platform-neutral.
- `ht_midi_probe --replay-raw-tsv` works without opening HAL.
- Runtime errors use `midi:` prefix.
- `--version` reports `MIDI backend: <name>`.
- Native backend builds on Linux, macOS, and Windows.
- Fake backend behavior tests pass without hardware.
- Companion docs no longer claim ALSA-only stub behavior.
