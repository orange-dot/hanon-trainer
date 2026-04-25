# midi_decode.c

## Purpose
Decodes one raw MIDI event packet into the internal event shape used by local diagnostic tooling.

## Theory
Raw MIDI status bytes carry both the event class and, for channel voice messages, a zero-based channel nibble. The decoder normalizes those bytes into user-facing channels and event fields while keeping the original status class visible.

## Architecture Role
This is a private C module, not part of the public library ABI. It gives hardware diagnostics and generated fixture tests one shared decode path before live `midi_capture` is enabled.

## Implementation Contract
Known channel messages require their exact raw byte count and seven-bit data bytes. `NOTEON velocity=0` is normalized as `note_off`; SysEx reports its raw byte length as `value`; unsupported well-formed status bytes decode as `other`.

## Ownership And Failure Modes
Callers own input storage and output records. Invalid arguments return `HT_ERR_INVALID_ARG`; malformed or truncated known messages return `HT_ERR_CORRUPT_DATA`; successful decodes return `HT_OK`.

## Test Strategy
`tests/test_midi_decode.c` covers common channel voice classes, SysEx length handling, malformed raw bytes, and invalid arguments. The generated PC4 fixture also replays raw TSV through `ht_midi_probe`, which uses this module.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
