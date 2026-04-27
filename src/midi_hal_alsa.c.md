# midi_hal_alsa.c

## Purpose

Implements the Linux ALSA sequencer backend for the private MIDI HAL.

## Theory

ALSA exposes MIDI clients and ports through the sequencer API. The backend
enumerates readable source ports, subscribes a local input port, waits on ALSA
poll descriptors, and converts ALSA events back into raw MIDI bytes.

## Architecture Role

This file is the only ALSA implementation unit for MIDI capture. Public Hanon
Trainer headers and diagnostic tools consume the private `ht_midi_hal_*`
contract instead of ALSA types.

## Implementation Contract

Device identifiers use `alsa:<client>:<port>`. Event timestamps are
connection-relative monotonic nanoseconds sampled when events are received.
Channel voice messages are emitted as raw MIDI bytes; SysEx payloads are
bounded by the HAL raw capacity.

## Ownership And Failure Modes

The backend owns the ALSA sequencer handle, local input port, subscription, and
poll descriptors. It releases all of them on disconnect or close. Runtime ALSA
failures are returned as `HT_ERR_IO`; unknown or stale ports return
`HT_ERR_NOT_FOUND`.

## Test Strategy

Native Linux builds compile and link this backend. Hardware behavior is covered
by manual `ht_midi_probe --list` and timed capture smoke tests.

## Spec Links

- `docs/ARCHITECTURE.md`
- `docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md`
