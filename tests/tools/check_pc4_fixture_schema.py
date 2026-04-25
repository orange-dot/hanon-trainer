#!/usr/bin/env python3
import argparse
import dataclasses
import pathlib
import random
import struct
from collections.abc import Iterable, Sequence


TSV_HEADER = ("ms", "kind", "channel", "note", "velocity", "controller", "value", "raw_type")
DEFAULT_SEED = 20260425
DEFAULT_TICKS_PER_QUARTER = 1000
DEFAULT_BPM = 60


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


@dataclasses.dataclass(frozen=True)
class MidiEvent:
    ms: int
    channel: int
    raw: tuple[int, ...]

    @property
    def status_byte(self) -> int:
        return self.raw[0]

    @property
    def status_class(self) -> int:
        if self.status_byte < 0xF0:
            return self.status_byte & 0xF0
        return self.status_byte

    @property
    def raw_type(self) -> str:
        return f"0x{self.status_class:02X}"

    @property
    def kind(self) -> str:
        if self.status_class == 0x80:
            return "note_off"
        if self.status_class == 0x90:
            return "note_off" if self.raw[2] == 0 else "note_on"
        if self.status_class == 0xA0:
            return "keypress"
        if self.status_class == 0xB0:
            return "controller"
        if self.status_class == 0xC0:
            return "pgmchange"
        if self.status_class == 0xD0:
            return "chanpress"
        if self.status_class == 0xE0:
            return "pitchbend"
        if self.status_class == 0xF0:
            return "sysex"
        return "other"

    @property
    def note(self) -> int | None:
        if self.kind in {"note_on", "note_off", "keypress"}:
            return self.raw[1]
        return None

    @property
    def velocity(self) -> int | None:
        if self.kind in {"note_on", "note_off"}:
            return self.raw[2]
        return None

    @property
    def controller(self) -> int | None:
        if self.kind == "controller":
            return self.raw[1]
        return None

    @property
    def value(self) -> int | None:
        if self.kind in {"controller", "keypress"}:
            return self.raw[2]
        if self.kind in {"pgmchange", "chanpress"}:
            return self.raw[1]
        if self.kind == "pitchbend":
            return self.raw[1] | (self.raw[2] << 7)
        if self.kind == "sysex":
            return len(self.raw)
        return None


def status(channel: int, status_class: int) -> int:
    require(1 <= channel <= 16, "MIDI channel must be 1..16")
    return status_class | (channel - 1)


def channel_event(ms: int, channel: int, status_class: int, data: Iterable[int]) -> MidiEvent:
    event = MidiEvent(ms=ms, channel=channel, raw=(status(channel, status_class), *tuple(data)))
    validate_wire_shape(event)
    return event


def sysex_event(ms: int, data: Iterable[int]) -> MidiEvent:
    payload = tuple(data)
    require(payload, "SysEx payload must not be empty")
    event = MidiEvent(ms=ms, channel=0, raw=(0xF0, *payload, 0xF7))
    validate_wire_shape(event)
    return event


def validate_wire_shape(event: MidiEvent) -> None:
    expected_lengths = {
        0x80: 3,
        0x90: 3,
        0xA0: 3,
        0xB0: 3,
        0xC0: 2,
        0xD0: 2,
        0xE0: 3,
    }
    expected_len = expected_lengths.get(event.status_class)
    if expected_len is not None:
        require(
            len(event.raw) == expected_len,
            f"{event.raw_type} should have {expected_len} raw bytes",
        )
    if event.status_byte < 0xF0:
        wire_channel = (event.status_byte & 0x0F) + 1
        require(wire_channel == event.channel, "event channel must match raw status byte")
    if event.status_class == 0xF0:
        require(event.raw[0] == 0xF0, "SysEx should start with 0xF0")
        require(event.raw[-1] == 0xF7, "SysEx should end with 0xF7")
    for byte in event.raw:
        require(0 <= byte <= 0xFF, "raw byte outside 0..255")
    for byte in event.raw[1:]:
        if event.status_byte < 0xF0:
            require(byte <= 0x7F, "MIDI data byte outside 0..127")


def pitchbend_bytes(value: int) -> list[int]:
    require(0 <= value <= 0x3FFF, "pitchbend value must be 0..16383")
    return [value & 0x7F, (value >> 7) & 0x7F]


def random_pressure_envelope(rng: random.Random) -> list[int]:
    first_rise = sorted(rng.sample(range(8, 120), 5))
    fall = sorted(rng.sample(range(0, 115), 5), reverse=True)
    second_rise = sorted(rng.sample(range(10, 118), 4))
    return [*first_rise, 0x7F, *fall, 0, *second_rise, 0x7F, rng.randrange(24, 95), 0]


def generate_capture(seed: int) -> list[MidiEvent]:
    rng = random.Random(seed)
    events: list[MidiEvent] = []
    ms = 0

    def advance(min_delta: int = 4, max_delta: int = 24) -> int:
        nonlocal ms
        ms += rng.randrange(min_delta, max_delta + 1)
        return ms

    channel_a = 1
    channel_b = 2
    root = rng.choice([48, 50, 52, 53, 55, 57, 60])
    chord = [root, root + 4, root + 7]

    events.append(channel_event(advance(), channel_a, 0xC0, [rng.randrange(0, 128)]))
    events.append(channel_event(advance(), channel_a, 0xB0, [1, rng.randrange(32, 110)]))
    events.append(channel_event(advance(), channel_a, 0xB0, [64, 127]))
    events.append(channel_event(advance(), channel_a, 0xE0, pitchbend_bytes(rng.randrange(0, 16384))))

    for note in chord:
        events.append(channel_event(advance(1, 6), channel_a, 0x90, [note, rng.randrange(36, 105)]))

    events.append(channel_event(advance(2, 8), channel_a, 0xA0, [chord[1], rng.randrange(20, 100)]))
    for pressure in random_pressure_envelope(rng):
        events.append(channel_event(advance(6, 18), channel_a, 0xD0, [pressure]))

    for note in reversed(chord):
        events.append(channel_event(advance(1, 10), channel_a, 0x80, [note, rng.randrange(16, 80)]))

    events.append(channel_event(advance(), channel_a, 0x90, [root + 12, rng.randrange(24, 100)]))
    events.append(channel_event(advance(), channel_a, 0x90, [root + 12, 0]))
    events.append(channel_event(advance(), channel_b, 0x90, [root + 19, rng.randrange(40, 110)]))
    events.append(channel_event(advance(), channel_b, 0xD0, [rng.randrange(1, 128)]))
    events.append(channel_event(advance(), channel_b, 0x80, [root + 19, rng.randrange(8, 70)]))
    events.append(channel_event(advance(), channel_a, 0xB0, [64, 0]))
    events.append(sysex_event(advance(), [0x7E, 0x7F, 0x06, 0x02, rng.randrange(0, 128)]))

    return events


def tsv_rows(events: Sequence[MidiEvent]) -> list[list[str]]:
    rows = [list(TSV_HEADER)]
    for event in events:
        rows.append(
            [
                str(event.ms),
                event.kind,
                "" if event.channel == 0 else str(event.channel),
                "" if event.note is None else str(event.note),
                "" if event.velocity is None else str(event.velocity),
                "" if event.controller is None else str(event.controller),
                "" if event.value is None else str(event.value),
                event.raw_type,
            ]
        )
    return rows


def render_tsv(events: Sequence[MidiEvent]) -> str:
    return "\n".join("\t".join(row) for row in tsv_rows(events)) + "\n"


def encode_varlen(value: int) -> bytes:
    require(value >= 0, "MIDI delta ticks must not be negative")
    buffer = value & 0x7F
    value >>= 7
    while value:
        buffer <<= 8
        buffer |= (value & 0x7F) | 0x80
        value >>= 7

    encoded = bytearray()
    while True:
        encoded.append(buffer & 0xFF)
        if buffer & 0x80:
            buffer >>= 8
        else:
            break
    return bytes(encoded)


def ms_to_ticks(ms: int) -> int:
    return round(ms * DEFAULT_TICKS_PER_QUARTER * DEFAULT_BPM / 60_000)


def render_smf(events: Sequence[MidiEvent]) -> bytes:
    tempo_us_per_quarter = round(60_000_000 / DEFAULT_BPM)
    track = bytearray()
    track.extend(b"\x00\xFF\x51\x03")
    track.extend(tempo_us_per_quarter.to_bytes(3, "big"))

    previous_tick = 0
    for event in events:
        tick = ms_to_ticks(event.ms)
        track.extend(encode_varlen(tick - previous_tick))
        if event.status_class == 0xF0:
            payload = bytes(event.raw[1:])
            track.append(0xF0)
            track.extend(encode_varlen(len(payload)))
            track.extend(payload)
        else:
            track.extend(event.raw)
        previous_tick = tick

    end_tick = previous_tick + ms_to_ticks(15)
    track.extend(encode_varlen(end_tick - previous_tick))
    track.extend(b"\xFF\x2F\x00")

    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, DEFAULT_TICKS_PER_QUARTER)
    return header + b"MTrk" + struct.pack(">I", len(track)) + bytes(track)


def check_generated_outputs(events: Sequence[MidiEvent], tsv_text: str, smf: bytes) -> None:
    lines = tsv_text.splitlines()
    require(lines[0] == "\t".join(TSV_HEADER), "generated TSV header should match ht_midi_probe")
    require(len(lines) == len(events) + 1, "generated TSV should contain one row per event")
    for line_number, line in enumerate(lines[1:], start=2):
        require(len(line.split("\t")) == len(TSV_HEADER), f"line {line_number} should have 8 columns")

    require(smf[:4] == b"MThd", "generated SMF should start with MThd")
    require(struct.unpack(">I", smf[4:8])[0] == 6, "generated SMF header length should be 6")
    format_type, track_count, division = struct.unpack(">HHH", smf[8:14])
    require(format_type == 0, "generated SMF should use format 0")
    require(track_count == 1, "generated SMF should contain one track")
    require(division == DEFAULT_TICKS_PER_QUARTER, "generated SMF division should be 1000")
    require(smf[14:18] == b"MTrk", "generated SMF should contain one MTrk chunk")


def check_summary(events: Sequence[MidiEvent]) -> None:
    kind_counts: dict[str, int] = {}
    controllers: set[int] = set()
    channels: set[int] = set()
    raw_types: set[str] = set()
    note_on_velocity_zero_count = 0
    aftertouch_peak_count = 0

    for event in events:
        kind_counts[event.kind] = kind_counts.get(event.kind, 0) + 1
        raw_types.add(event.raw_type)
        if event.channel != 0:
            channels.add(event.channel)
        if event.kind == "controller" and event.controller is not None:
            controllers.add(event.controller)
        if event.status_class == 0x90 and event.raw[2] == 0:
            note_on_velocity_zero_count += 1
        if event.kind == "chanpress" and event.value == 0x7F:
            aftertouch_peak_count += 1

    require(len(events) >= 30, "synthetic capture should contain a non-trivial event stream")
    require(kind_counts.get("note_on", 0) >= 4, "synthetic capture should include note-on events")
    require(kind_counts.get("note_off", 0) >= 4, "synthetic capture should include note-off events")
    require(kind_counts.get("chanpress", 0) >= 10, "synthetic capture should include aftertouch")
    require(kind_counts.get("controller", 0) >= 3, "synthetic capture should include controllers")
    require(kind_counts.get("pitchbend", 0) >= 1, "synthetic capture should include pitchbend")
    require(kind_counts.get("pgmchange", 0) >= 1, "synthetic capture should include program change")
    require(kind_counts.get("keypress", 0) >= 1, "synthetic capture should include poly aftertouch")
    require(kind_counts.get("sysex", 0) >= 1, "synthetic capture should include SysEx")
    require(1 in controllers, "synthetic capture should include mod wheel CC1")
    require(64 in controllers, "synthetic capture should include sustain CC64")
    require(channels == {1, 2}, "synthetic capture should include two user MIDI channels")
    require(aftertouch_peak_count >= 2, "synthetic capture should include aftertouch peaks at 0x7F")
    require(note_on_velocity_zero_count >= 1, "synthetic capture should cover note-on velocity zero")
    for raw_type in {"0x80", "0x90", "0xA0", "0xB0", "0xC0", "0xD0", "0xE0", "0xF0"}:
        require(raw_type in raw_types, f"synthetic capture should include {raw_type}")


def write_outputs(out_dir: pathlib.Path, events: Sequence[MidiEvent], seed: int) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    tsv_text = render_tsv(events)
    smf = render_smf(events)
    check_generated_outputs(events, tsv_text, smf)
    check_summary(events)
    (out_dir / "pc4-synthetic-capture.tsv").write_text(tsv_text, encoding="utf-8")
    (out_dir / "pc4-synthetic-capture.mid").write_bytes(smf)
    (out_dir / "pc4-synthetic-capture.seed").write_text(f"{seed}\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    args = parser.parse_args()

    events = generate_capture(args.seed)
    write_outputs(pathlib.Path(args.out_dir), events, args.seed)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
