#!/usr/bin/env python3
import argparse
import dataclasses
import pathlib
import re
import struct
from collections.abc import Sequence


TRACE_RE = re.compile(
    r"^midi trace: ch=(?P<channel>\d+) raw=\[(?P<raw>[0-9A-Fa-f ]+)\](?: (?P<text>.*))?$"
)
TSV_HEADER = ("ms", "kind", "channel", "note", "velocity", "controller", "value", "raw_type")
DEFAULT_CADENCE_MS = 15
DEFAULT_TICKS_PER_QUARTER = 1000
DEFAULT_BPM = 60


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


@dataclasses.dataclass(frozen=True)
class MidiEvent:
    line_no: int
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
        return None


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
            f"line {event.line_no}: {event.raw_type} should have {expected_len} raw bytes",
        )
    if event.status_byte < 0xF0:
        wire_channel = (event.status_byte & 0x0F) + 1
        require(
            wire_channel == event.channel,
            f"line {event.line_no}: trace channel does not match raw status byte",
        )
    for byte in event.raw:
        require(0 <= byte <= 0xFF, f"line {event.line_no}: raw byte outside 0..255")
    for byte in event.raw[1:]:
        if event.status_byte < 0xF0:
            require(byte <= 0x7F, f"line {event.line_no}: MIDI data byte outside 0..127")


def parse_trace(path: pathlib.Path) -> list[MidiEvent]:
    events: list[MidiEvent] = []
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        match = TRACE_RE.match(line)
        require(match is not None, f"line {line_no}: cannot parse source trace")
        raw = tuple(int(part, 16) for part in match.group("raw").split())
        require(raw, f"line {line_no}: raw byte list is empty")
        event = MidiEvent(line_no=line_no, channel=int(match.group("channel")), raw=raw)
        validate_wire_shape(event)
        events.append(event)
    require(events, "source trace should contain MIDI events")
    return events


def tsv_rows(events: Sequence[MidiEvent]) -> list[list[str]]:
    rows = [list(TSV_HEADER)]
    for index, event in enumerate(events):
        rows.append(
            [
                str(index * DEFAULT_CADENCE_MS),
                event.kind,
                str(event.channel),
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
    for index, event in enumerate(events):
        require(event.status_byte < 0xF0, "PC4 SMF fixture should contain channel messages only")
        tick = ms_to_ticks(index * DEFAULT_CADENCE_MS)
        track.extend(encode_varlen(tick - previous_tick))
        track.extend(event.raw)
        previous_tick = tick

    end_tick = previous_tick + ms_to_ticks(DEFAULT_CADENCE_MS)
    track.extend(encode_varlen(end_tick - previous_tick))
    track.extend(b"\xFF\x2F\x00")

    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, DEFAULT_TICKS_PER_QUARTER)
    return header + b"MTrk" + struct.pack(">I", len(track)) + bytes(track)


def check_summary(events: Sequence[MidiEvent]) -> None:
    kind_counts: dict[str, int] = {}
    channels: set[int] = set()
    active_notes: set[tuple[int, int]] = set()
    orphan_note_off_count = 0
    aftertouch_peak_count = 0

    for event in events:
        kind_counts[event.kind] = kind_counts.get(event.kind, 0) + 1
        channels.add(event.channel)
        if event.kind == "chanpress" and event.value == 0x7F:
            aftertouch_peak_count += 1
        if event.note is None:
            continue
        note_key = (event.channel, event.note)
        if event.kind == "note_on":
            active_notes.add(note_key)
        elif event.kind == "note_off":
            if note_key in active_notes:
                active_notes.remove(note_key)
            else:
                orphan_note_off_count += 1

    require(len(events) == 50, "PC4 fixture should contain exactly 50 events")
    require(kind_counts.get("chanpress") == 35, "PC4 fixture should cover 35 channel-pressure events")
    require(kind_counts.get("note_on") == 7, "PC4 fixture should contain 7 note-on events")
    require(kind_counts.get("note_off") == 8, "PC4 fixture should contain 8 note-off events")
    require(channels == {1}, "PC4 fixture should stay on user MIDI channel 1")
    require(aftertouch_peak_count == 3, "PC4 fixture should have 3 aftertouch peaks at 0x7F")
    require(orphan_note_off_count >= 1, "PC4 fixture should preserve the mid-stream orphan note-off")


def check_fixture(fixture_dir: pathlib.Path) -> None:
    events = parse_trace(fixture_dir / "source-mamut-trace.txt")
    actual_tsv = (fixture_dir / "pc4-aftertouch-arpeggio.tsv").read_text(encoding="utf-8")
    actual_smf = (fixture_dir / "pc4-aftertouch-arpeggio.mid").read_bytes()
    generated_tsv = render_tsv(events)
    generated_smf = render_smf(events)

    require(actual_tsv == generated_tsv, "PC4 TSV fixture should match source-trace regeneration")
    require(actual_smf == generated_smf, "PC4 SMF fixture should match source-trace regeneration")
    require(len(generated_smf) == 198, "PC4 SMF fixture should remain 198 bytes")
    require(generated_smf[:4] == b"MThd", "SMF should start with MThd")
    require(struct.unpack(">I", generated_smf[4:8])[0] == 6, "SMF header length should be 6")
    format_type, track_count, division = struct.unpack(">HHH", generated_smf[8:14])
    require(format_type == 0, "SMF should use format 0")
    require(track_count == 1, "SMF should contain one track")
    require(division == DEFAULT_TICKS_PER_QUARTER, "SMF division should be 1000 ticks per quarter")
    require(generated_smf[14:18] == b"MTrk", "SMF should contain an MTrk chunk after header")
    require(struct.unpack(">I", generated_smf[18:22])[0] == 176, "SMF track length should remain 176")
    check_summary(events)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture-dir", required=True)
    args = parser.parse_args()

    fixture_dir = pathlib.Path(args.fixture_dir)
    required_files = [
        "README.md",
        "pc4-aftertouch-arpeggio.mid",
        "pc4-aftertouch-arpeggio.tsv",
        "source-mamut-trace.txt",
    ]
    for name in required_files:
        require((fixture_dir / name).is_file(), f"missing PC4 fixture file: {name}")

    check_fixture(fixture_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
