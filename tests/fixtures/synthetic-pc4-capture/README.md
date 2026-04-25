# Synthetic PC4 raw capture

A synthetic MIDI capture fixture derived from a screenshot of a real
Kurzweil PC4 play session that was **not saved as a log file**. The
screenshot was the only surviving artifact of that session (window
title `mamut-sint-sw-remote-up-21-4 : mamut-standalon`, 2026-04-25).

This fixture exists to give the `hanon-trainer` `ht_midi_probe` decode
path (see `../../../tools/ht_midi_probe.c.md`) a small but
representative input that exercises the Kurzweil-typical
**channel-pressure aftertouch envelope**, since that signal class was
flagged as a coverage gap (B1) in the probe plan review.

It is **not** a recording. It is **not** ground truth for performance
timing. Use it only to validate parser decode paths and round-trip
fidelity.

---

## Files

| File | Role |
|---|---|
| `source-mamut-trace.txt` | Verbatim transcription from the screenshot, one event per line, in `mamut-sint-sw` logger format. The raw bytes (`raw=[..]`) are the authoritative source — semantic decode is informational only. |
| `pc4-aftertouch-arpeggio.tsv` | Same 50 events in the `ht_midi_probe --format tsv` schema (see §Schema). Synthetic timestamps. |
| `pc4-aftertouch-arpeggio.mid` | Same 50 events as a Standard MIDI File, format 0, single track, 1000 ticks/quarter at 60 BPM (so 1 tick = 1 ms). 198 bytes total. Plays in any DAW. |
| `README.md` | This file. |

---

## Origin

The screenshot showed 50 consecutive events from the `mamut-standalon`
window's MIDI trace pane. The user was playing a Kurzweil PC4 connected
to the host running `mamut-sint-sw`. The session itself was never
written to disk; the screenshot is the only surviving artifact.

The trace **starts mid-stream**: the first event is `note off note=72`
with no preceding `note on` for note 72 in the visible window. Real
note-ons for that note happened earlier in the session and are lost.

---

## What's in the trace

50 events on channel 1:

- **35 channel pressure (aftertouch)**, status `0xD0` — three full
  envelopes (rise → peak `0x7F` → fall to `0x00`), exactly the shape a
  weighted-action Kurzweil emits during sustained playing.
- **7 note-on**, status `0x90` — two 3-note chord clusters
  (`62 67 72` then `64 69 74`, roughly Dm-shape transposed up a step)
  plus one isolated note that opens the visible window.
- **8 note-off**, status `0x80` — release events. **Note-off velocity
  is preserved** in the raw bytes (third byte) but the original logger
  prints only the note number; the TSV and SMF preserve the release
  velocity from the raw bytes.

What is **not** present (and therefore not represented in the fixture):

- pitchbend (`0xE0`)
- mod wheel (`0xB0 01 ..`)
- sustain pedal (`0xB0 40 ..`)
- program change (`0xC0`)
- SysEx (`0xF0 .. 0xF7`)
- poly aftertouch (`0xA0`)
- any second MIDI channel

If a future fixture needs to cover these, capture a fresh trace with
the relevant gestures (move the wheel, press the pedal, change a
program) — do not synthesize them on top of this one.

---

## Synthesis rules (what is real vs. what is invented)

**Real (from the screenshot):**

- Event order
- Status bytes (`0xD0`, `0x90`, `0x80`) and channel (1)
- Note numbers
- Velocity / pressure / release-velocity raw bytes
- The fact that this is a single-channel, single-instrument session

**Invented (synthetic):**

- **Timestamps.** Each event is stamped at `ms = index × 15`, so the
  whole 50-event sequence spans 735 ms. Real PC4 timing varies by
  event class:
  - aftertouch: ~10–20 ms apart on continuous pressure
  - chord notes: within ~5 ms of each other
  - melodic notes: 100–500 ms apart
  The uniform 15 ms cadence is **wrong as performance data** but
  reproducible and easy to reason about. Do not use the timestamps to
  judge timing accuracy of any consumer.
- **Tempo / BPM in the SMF.** 60 BPM with 1000 ticks/quarter is chosen
  only so that 1 tick = 1 ms — making the SMF round-trip exactly with
  the TSV `ms` column. It does not reflect any musical tempo of the
  original session.
- **Track name and program.** No track-name meta event and no program
  change are emitted. The receiving DAW will play the events on its
  default channel-1 instrument.

---

## Schema (TSV)

The TSV matches the `ht_midi_probe --format tsv` schema proposed in
the plan review:

```
ms<TAB>kind<TAB>channel<TAB>note<TAB>velocity<TAB>controller<TAB>value<TAB>raw_type
```

| Column | Type | Meaning |
|---|---|---|
| `ms` | int ≥ 0 | session-relative monotonic milliseconds from first event |
| `kind` | enum | one of `note_on`, `note_off`, `chanpress`, `controller`, `pitchbend`, `pgmchange`, `keypress`, `other` |
| `channel` | int 1..16 | User-facing MIDI channel. ALSA stores this internally as a 0-indexed nibble; this fixture and `ht_midi_probe` print it as 1-indexed. |
| `note` | int 0..127 | populated for `note_on`, `note_off`, `keypress` |
| `velocity` | int 0..127 | populated for `note_on`, `note_off` (release velocity), `keypress` |
| `controller` | int 0..127 | populated for `controller` only |
| `value` | int 0..16383 | populated for `controller`, `chanpress`, `pgmchange`, `pitchbend` |
| `raw_type` | hex | upper nibble of the status byte, lower nibble zeroed (e.g. `0xD0`) |

Empty cells are **literal empty strings** between tabs (no `-`, no `null`).

The `mamut-sint-sw` logger prints `velocity` and `pressure` as
floating-point in [0..1]; this fixture **stores them as raw 7-bit
integers (0..127)** so consumers do not have to round-trip through
floating point. The two are interchangeable via `raw / 127`.

---

## Channel encoding caveat

The trace says `ch=1`, and the raw status byte is `0xD0` (not `0xD1`).
That means the logger uses **1-indexed user channels** while the wire
format is the standard 0-indexed nibble. The TSV follows the logger:
`channel=1` corresponds to wire-byte `0xD0` / `0x80` / `0x90`. The SMF
uses the wire-byte form directly.

---

## Limitations as a fixture

1. **Mid-stream start.** The first event is a note-off with no
   preceding note-on; consumers must tolerate orphan note-off events.
2. **No timing fidelity.** See "Synthesis rules" above.
3. **Single channel only.** Multi-channel parsing paths are not
   exercised.
4. **No control change / pitchbend / program change / SysEx.** Coverage
   for those event classes must come from a separate fixture.
5. **Asymmetric note on/off counts** (7 on, 8 off) because of (1).
6. **Low-resolution screenshot transcription.** If a digit was
   misread from the screenshot the only correction path is to capture
   a new trace from a live session — there is no original log file to
   re-read.

---

## Reproducing / extending

If `mamut-sint-sw` writes a real `.log` of the next session, prefer
that file over this synthetic one. The screenshot-derived data is a
stopgap.

To extend coverage, capture a fresh session with the gestures
mentioned in "What's not present" and add a second fixture file rather
than editing this one.
