# test_midi_hal_fake.c

## Purpose
Verifies the deterministic fake MIDI backend and the public capture adapter built on top of it.

## Theory
A cross-platform MIDI boundary needs tests that do not depend on connected hardware or a host driver. The fake backend provides a stable source port, a fixed event stream, and a bounded SysEx overflow case so HAL and capture contracts can be tested in CTest.

## Architecture Role
This test is private to the `HT_MIDI_BACKEND=FAKE` build. It exercises `src/midi_hal.h` directly and also checks that `midi_capture.h` maps HAL events into the public event record shape.

## Implementation Contract
The fake backend lists exactly one default input, `fake:keyboard`, emits nine events, preserves session-relative timestamps, reports oversized SysEx with `raw_truncated`, and maps note-on, note-off, and control-change events through `ht_midi_capture_poll_event`.

## Ownership And Failure Modes
The test owns every opened HAL and capture handle and closes them before returning. Invalid wait and missing-port cases are expected status-code failures, not process failures.

## Test Strategy
CTest runs this file only when CMake is configured with `-DHT_MIDI_BACKEND=FAKE`. The CLI probe test separately validates fake backend list and capture output through the executable boundary.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
