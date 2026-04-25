# midi_capture.h

## Purpose
Declares the MIDI capture boundary for device listing, start/stop, and event polling.

## Theory
Capture is a platform integration concern. It emits events but does not own persistence, analysis, or UI state.

## Architecture Role
This header is a platform leaf consumed by future app/session orchestration.

## Implementation Contract
Empty polls return `HT_OK` with `out_has_event == false`. Buffer capacities are element counts.

## Ownership And Failure Modes
The capture handle is caller-owned after open. Live capture is unsupported in this slice and returns `HT_ERR_UNSUPPORTED`.

## Test Strategy
`tests/test_platform_boundaries.c` verifies ALSA-linked open/list/poll/start/stop behavior without live devices.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
