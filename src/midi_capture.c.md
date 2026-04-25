# midi_capture.c

## Purpose
Implements the ALSA-linked MIDI capture boundary stub.

## Theory
The slice proves platform compile/link integration without pretending live device enumeration or capture exists.

## Architecture Role
This source is a platform integration leaf below future app orchestration.

## Implementation Contract
Open records ALSA library availability, listing returns zero devices, polling returns no event, and start returns unsupported.

## Ownership And Failure Modes
The capture handle is caller-owned. Invalid buffers or handles return invalid-argument status.

## Test Strategy
`tests/test_platform_boundaries.c` covers open/list/poll/start/stop without live hardware.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
