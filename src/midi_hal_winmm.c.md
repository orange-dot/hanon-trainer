# midi_hal_winmm.c

## Purpose

Implements the Windows WinMM backend for the private MIDI HAL.

## Theory

WinMM exposes MIDI input devices by index and delivers short MIDI messages and
SysEx buffers through callbacks. The backend normalizes callback data into
bounded raw MIDI HAL events.

## Architecture Role

This file contains all Windows MIDI API usage for Hanon Trainer. Public capture
code and the probe use the private `ht_midi_hal_*` contract instead of WinMM
types.

## Implementation Contract

Device identifiers use `winmm:<index>`. Event timestamps are monotonic
nanoseconds relative to connection start. Short messages keep their raw status
and data bytes; SysEx uses a bounded HAL buffer.

## Ownership And Failure Modes

The backend owns the WinMM input handle, callback event, critical section, and
SysEx buffer header. Unknown device selectors return `HT_ERR_NOT_FOUND`; WinMM
runtime failures return `HT_ERR_IO`.

## Test Strategy

Windows CI compiles and links this backend. Manual smoke uses `ht_midi_probe
--list` and timed capture from a listed `winmm:<index>` device id.

## Spec Links

- `docs/ARCHITECTURE.md`
- `docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md`
