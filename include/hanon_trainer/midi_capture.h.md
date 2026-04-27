# midi_capture.h

## Purpose
Declares the MIDI capture boundary for device listing, start/stop, and event polling.

## Theory
Capture is a platform integration concern. It emits session-relative MIDI events from the selected native backend but does not own persistence, analysis, or UI state.

## Architecture Role
This header is the public MIDI ingress boundary consumed by app/session orchestration. OS APIs stay behind the private HAL in `src/`.

## Implementation Contract
Empty polls return `HT_OK` with `out_has_event == false`. Buffer capacities are element counts. Device identifiers are backend-owned strings such as `alsa:<client>:<port>`, `coremidi:<id>`, or `winmm:<index>`.

## Ownership And Failure Modes
The capture handle is caller-owned after open and supports one active input stream at a time. A stale or unavailable device id returns `HT_ERR_NOT_FOUND`; backend I/O failures return `HT_ERR_IO`.

## Test Strategy
`tests/test_platform_boundaries.c` verifies boundary behavior without requiring live devices. `tests/test_midi_hal_fake.c` verifies deterministic fake-device enumeration and capture mapping in the fake backend build.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
