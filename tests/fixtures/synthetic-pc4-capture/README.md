# Synthetic PC4 raw capture

This directory documents the CI-generated Kurzweil PC4-like MIDI fixture used
by `ht_pc4_fixture_schema`.

The synthetic capture has no checked-in raw source trace. CTest generates the
event stream from a deterministic pseudo-random seed and writes the raw input,
decoded TSV, and SMF files under the CMake build directory:

```text
build/generated-fixtures/synthetic-pc4-capture/
  pc4-synthetic-capture.raw.tsv
  pc4-synthetic-capture.tsv
  pc4-synthetic-capture.mid
  pc4-synthetic-capture.seed
```

The fixed CI seed makes failures reproducible while still exercising generated
data rather than a committed TSV or MIDI file. The raw TSV is then replayed
through the compiled `ht_midi_probe`, so the checked behavior depends on the C
decoder instead of Python-only expectations.

CI runs the generator with `--verbose`, which prints the seed, artifact paths,
event counts, covered status classes, sample raw rows, sample decoded rows, and
the compiled probe replay result.

## Generated Coverage

The generator emits a PC4-like raw MIDI capture with:

- note-on and note-off events, including `NOTEON velocity=0` as note-off input
- channel-pressure aftertouch envelopes with `0x7F` peaks
- poly aftertouch / key pressure
- mod wheel (`CC1`) and sustain pedal (`CC64`)
- pitchbend
- program change
- SysEx
- two user-facing MIDI channels

The generated timestamps are synthetic session-relative milliseconds. They are
valid for parser and schema coverage, not performance timing analysis.

## TSV Schema

The raw replay input is:

```text
ms<TAB>raw
```

The `raw` column contains uppercase hexadecimal MIDI bytes separated by spaces.

The generated TSV matches `ht_midi_probe --format tsv`:

```text
ms<TAB>kind<TAB>channel<TAB>note<TAB>velocity<TAB>controller<TAB>value<TAB>raw_type
```

Empty cells are literal empty strings between tabs. User-facing MIDI channels
are printed as `1..16`; raw status bytes use the standard 0-indexed channel
nibble. Known event classes use hex status classes in `raw_type`, such as
`0x90`, `0xD0`, and `0xF0`.

## SMF Shape

The generated MIDI file is Standard MIDI File format 0 with one track,
1000 ticks per quarter, and 60 BPM. That timing basis keeps one tick equal to
one millisecond for the generated fixture.
