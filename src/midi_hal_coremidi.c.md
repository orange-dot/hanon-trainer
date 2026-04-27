# midi_hal_coremidi.c

## Purpose

Implements the macOS CoreMIDI backend for the private MIDI HAL.

## Theory

CoreMIDI exposes MIDI sources as endpoints and delivers packets through an
input-port callback. The backend converts packet bytes into bounded HAL raw
events and buffers them for nonblocking polling.

## Architecture Role

This file keeps CoreMIDI and CoreFoundation types inside one private backend
translation unit. The public capture API and probe consume the shared
`ht_midi_hal_*` contract.

## Implementation Contract

Device identifiers use `coremidi:<unique-id>` when CoreMIDI provides a unique
id, otherwise `coremidi:index:<n>`. Event timestamps are monotonic nanoseconds
relative to connection start. SysEx payloads are bounded by the HAL raw
capacity.

## Ownership And Failure Modes

The backend owns the CoreMIDI client, input port, endpoint connection, mutex,
and bounded event queue. CoreMIDI failures return `HT_ERR_IO`; unknown endpoint
selectors return `HT_ERR_NOT_FOUND`.

## Test Strategy

macOS CI compiles and links this backend. Manual smoke uses `ht_midi_probe
--list` and timed capture from the listed `device_id`.

## Spec Links

- `docs/ARCHITECTURE.md`
- `docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md`
