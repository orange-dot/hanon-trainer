# PC4 Live MIDI Fixtures

This directory contains sanitized MIDI-only fixtures exported from the local
`synthetic-pc4-capture` lab evidence area. The committed files are intentionally
small replay inputs for `ht_midi_probe --replay-raw-tsv`; they are not the
complete capture archive.

Committed contents:

- `*.raw.tsv`: `ms<TAB>raw` event streams replayed by `ht_midi_probe`
- `*.meta.json`: source hashes, event counts, event-kind counts, and quality
  labels copied from the local lab evidence

Excluded contents:

- WAV recordings
- pcap files
- full capture manifests and data-lake metadata
- zero-event diagnostic takes

The fixture set currently contains 12 non-empty derived takes with 9,945 total
MIDI events. The largest required fixture is
`2026-04-26-pc4-tui-002.raw.tsv`, a 230 second clean keeper with 2,913 events.
That take includes notes, controllers, pitchbend, channel aftertouch, and
program changes.

## Source

The source evidence lives outside this repository under the lab root:

- `synthetic-pc4-capture/derived/*/midi_events.tsv`
- `synthetic-pc4-capture/derived/capture_quality_summary.tsv`
- `synthetic-pc4-capture/real-captures/*`

Each metadata file records the source take id, source evidence filename,
SHA-256, size, quality class, and decoded MIDI event counts.

## Regeneration

From the `hanon-trainer` repository root, regenerate the fixtures with:

```sh
python3 tests/tools/export_pc4_live_midi_fixtures.py \
  --derived-root /home/dev/work-base-20260421/synthetic-pc4-capture/derived \
  --capture-root /home/dev/work-base-20260421/synthetic-pc4-capture/real-captures \
  --quality-summary /home/dev/work-base-20260421/synthetic-pc4-capture/derived/capture_quality_summary.tsv \
  --out-dir tests/fixtures/pc4-live-midi
```

Then validate them against the compiled probe:

```sh
python3 tests/tools/check_pc4_live_midi_fixtures.py \
  --fixtures tests/fixtures/pc4-live-midi \
  --probe build/ht_midi_probe \
  --verbose
```
