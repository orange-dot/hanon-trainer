# ht_midi_probe.c

## Purpose
Implements a standalone MIDI hardware probe for validating native backend visibility and event decoding.

## Theory
The probe exercises the same private MIDI HAL as the library capture path. It lists backend-owned device ids, waits for live events through the HAL, and delegates raw-byte interpretation to the private decoder. The raw replay path remains HAL-free so fixtures can be validated without hardware.

## Architecture Role
This tool is outside the public C library surface. It does not change `midi_capture.h`, does not write SQLite, and does not add a public header-graph dependency.

## Implementation Contract
The CLI supports `--help`, `--version`, `--list`, and timed capture from an explicit `--port` in `table` or `tsv` format. `--version` reports the selected MIDI backend and `--list` prints `device_id`, `backend`, and `name`. Its replay path accepts generated raw-byte TSV input and renders through the same decode/print path. It normalizes `NOTEON velocity=0` as `note_off`, decodes common Kurzweil events, reports user-facing channels as 1-indexed, writes known MIDI event classes as hex status classes in `raw_type`, keeps TSV stdout parseable, and never reads stdin.

## Ownership And Failure Modes
The tool owns the selected HAL handle and releases it before exit. CLI errors return `EX_USAGE`; MIDI runtime failures return `1` with a `midi:` prefix; clean capture returns `0` even with zero events.

## Test Strategy
CTest covers help/version, parser failures, backend-neutral list output, runtime error prefixes, and raw replay without requiring hardware. The fake backend build also captures the deterministic fake event stream. Synthetic and real-derived Kurzweil PC4 fixtures replay through the compiled probe to protect the decoder, TSV schema, and MIDI event-class coverage for parser consumers. Manual hardware acceptance uses `--list` followed by timed capture from the displayed device id.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
