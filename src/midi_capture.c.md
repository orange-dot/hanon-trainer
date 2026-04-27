# midi_capture.c

## Purpose
Implements the public MIDI capture boundary on top of the private platform HAL.

## Theory
The public library should expose stable capture records and status codes while platform-specific MIDI APIs remain private implementation details. The HAL supplies raw MIDI bytes and session-relative timestamps; this adapter validates handles, owns session id and sequence number state, and maps decoded events into the public record shape.

## Architecture Role
This source sits between `midi_capture.h` and `src/midi_hal.h`. It keeps OS headers out of the public ABI and keeps persistence out of capture.

## Implementation Contract
Open allocates a capture handle and backend HAL handle. Listing delegates to the selected backend. Start accepts one active stream, copies the borrowed session id, and resets sequence numbers. Poll is nonblocking, returns empty polls as `HT_OK`, decodes raw MIDI through `midi_decode`, maps note-on/note-off/controller events to public kinds, and reports other valid MIDI events as `HT_MIDI_EVENT_OTHER`.

## Ownership And Failure Modes
The capture handle is caller-owned. Close disconnects an active backend before freeing storage. Invalid buffers or handles return invalid-argument status; stale device ids return not-found; backend failures propagate as I/O errors.

## Test Strategy
`tests/test_platform_boundaries.c` covers open/list/poll/start/stop without requiring live hardware. `tests/test_midi_hal_fake.c` checks deterministic event mapping in the fake backend build.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
