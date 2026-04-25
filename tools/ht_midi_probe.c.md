# ht_midi_probe.c

## Purpose
Implements a standalone ALSA MIDI hardware probe for validating real keyboard input before the library capture path is enabled.

## Theory
The probe uses ALSA sequencer APIs directly so it can list source ports and decode structured MIDI events without shelling out to `aseqdump`.

## Architecture Role
This tool is outside the public C library surface. It does not change `midi_capture.h`, does not write SQLite, and does not add a header-graph dependency.

## Implementation Contract
The CLI supports `--help`, `--version`, `--list`, and timed capture from an explicit `--port` in `table` or `tsv` format. It normalizes `NOTEON velocity=0` as `note_off`, decodes common Kurzweil events, reports user-facing channels as 1-indexed, writes known MIDI event classes as hex status classes in `raw_type`, keeps TSV stdout parseable, and never reads stdin.

## Ownership And Failure Modes
The tool owns the ALSA sequencer handle, local input port, subscription, and poll descriptors for the process lifetime. CLI errors return `EX_USAGE`; ALSA runtime failures return `1`; clean capture returns `0` even with zero events.

## Test Strategy
CTest covers help/version and parser failures without requiring hardware. `tests/fixtures/synthetic-pc4-capture` protects a screenshot-derived Kurzweil PC4 channel-pressure fixture for parser consumers. Manual hardware acceptance uses `--list` followed by timed capture from the displayed Kurzweil port.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
