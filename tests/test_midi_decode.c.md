# test_midi_decode.c

## Purpose
Verifies the private raw MIDI decoder used by diagnostic tooling.

## Theory
A small unit executable can cover event-class semantics directly, without ALSA hardware or generated fixture files.

## Architecture Role
This test guards the private decode module that sits below `ht_midi_probe` and before the future live capture implementation.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable. It treats the raw input byte packet as the authority and checks normalized fields.

## Ownership And Failure Modes
All test storage is stack-owned. Assertions fail on incorrect decode status, field normalization, or error classification.

## Test Strategy
It covers note-on, note-off, `NOTEON velocity=0`, controller, pitchbend, program change, channel pressure, key pressure, SysEx length, malformed raw bytes, and invalid arguments.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
