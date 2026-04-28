# midi_hal_fake.c

## Purpose

Provides the deterministic fake MIDI backend for platform-independent HAL tests.

## Theory

The fake backend behaves like a small in-memory MIDI input device. It enumerates
one stable fake port and emits a fixed stream of raw MIDI messages.

## Architecture Role

This source implements the private `ht_midi_hal_*` backend contract for test
builds. It is not part of the public library ABI.

## Implementation Contract

The backend lists `fake:keyboard`, rejects unknown device identifiers, starts a
single active connection, and emits monotonically increasing event timestamps.
The stream covers channel voice messages and bounded SysEx behavior.

## Ownership And Failure Modes

`ht_midi_hal_open` allocates the backend handle and `ht_midi_hal_close` releases
it. Invalid arguments return `HT_ERR_INVALID_ARG`; unknown ports return
`HT_ERR_NOT_FOUND`; allocation failure returns `HT_ERR_IO`.

## Test Strategy

Used by CTest through the `HT_MIDI_BACKEND=FAKE` build to verify capture and
probe behavior without live MIDI hardware.

## Spec Links

- `docs/ARCHITECTURE.md`
- `docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md`
