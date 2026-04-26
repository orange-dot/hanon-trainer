#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
import pathlib
from collections import Counter
from collections.abc import Iterable


RAW_TSV_HEADER = ("ms", "raw")
SCHEMA_VERSION = "pc4-live-midi-raw-tsv-v1"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read_quality_summary(path: pathlib.Path) -> dict[str, dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        return {row["take_id"]: row for row in csv.DictReader(handle, delimiter="\t")}


def read_midi_rows(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle, delimiter="\t"))
    return [row for row in rows if row.get("raw_bytes", "").strip()]


def raw_bytes(row: dict[str, str]) -> list[int]:
    return [int(part, 16) for part in row["raw_bytes"].split()]


def probe_kind(data: list[int]) -> str:
    status = data[0]
    if 0xF0 <= status <= 0xF7:
        return "sysex"
    if status >= 0xF8:
        return "other"
    status_class = status & 0xF0
    if status_class == 0x80:
        return "note_off"
    if status_class == 0x90:
        return "note_off" if len(data) >= 3 and data[2] == 0 else "note_on"
    if status_class == 0xA0:
        return "keypress"
    if status_class == 0xB0:
        return "controller"
    if status_class == 0xC0:
        return "pgmchange"
    if status_class == 0xD0:
        return "chanpress"
    if status_class == 0xE0:
        return "pitchbend"
    return "other"


def raw_type(data: list[int]) -> str:
    status = data[0]
    if 0xF0 <= status <= 0xF7:
        return "0xF0"
    if status >= 0xF8:
        return f"0x{status:02X}"
    return f"0x{status & 0xF0:02X}"


def event_ms(row: dict[str, str]) -> int:
    time_s = row.get("time_s", "")
    if time_s == "":
        return 0
    return round(float(time_s) * 1000.0)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_file_records(rows: Iterable[dict[str, str]], capture_root: pathlib.Path) -> list[dict[str, object]]:
    names = sorted({row["source_file"] for row in rows if row.get("source_file", "")})
    records: list[dict[str, object]] = []
    for name in names:
        path = capture_root / name
        require(path.exists(), f"missing source evidence file: {path}")
        records.append(
            {
                "file": name,
                "lab_relative_path": f"synthetic-pc4-capture/real-captures/{name}",
                "sha256": sha256_file(path),
                "size_bytes": path.stat().st_size,
            }
        )
    return records


def write_raw_tsv(path: pathlib.Path, rows: list[dict[str, str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle, delimiter="\t", lineterminator="\n")
        writer.writerow(RAW_TSV_HEADER)
        for row in rows:
            writer.writerow((event_ms(row), row["raw_bytes"]))


def write_meta(
    path: pathlib.Path,
    take_id: str,
    rows: list[dict[str, str]],
    quality: dict[str, str],
    capture_root: pathlib.Path,
) -> None:
    probe_kind_counts: Counter[str] = Counter()
    raw_type_counts: Counter[str] = Counter()
    source_layer_counts: Counter[str] = Counter()
    controller_counts: Counter[str] = Counter()
    channels: set[int] = set()
    ms_values: list[int] = []

    for row in rows:
        data = raw_bytes(row)
        probe_kind_counts[probe_kind(data)] += 1
        raw_type_counts[raw_type(data)] += 1
        source_layer_counts[row.get("source_layer", "unknown")] += 1
        ms_values.append(event_ms(row))
        if data[0] < 0xF0:
            channels.add((data[0] & 0x0F) + 1)
        if (data[0] & 0xF0) == 0xB0 and len(data) >= 2:
            controller_counts[str(data[1])] += 1

    meta = {
        "schema": SCHEMA_VERSION,
        "source_take_id": take_id,
        "source_root": "synthetic-pc4-capture/derived",
        "source_files": source_file_records(rows, capture_root),
        "event_count": len(rows),
        "duration_ms": max(ms_values) if ms_values else 0,
        "channels": sorted(channels),
        "source_layer_counts": dict(sorted(source_layer_counts.items())),
        "probe_kind_counts": dict(sorted(probe_kind_counts.items())),
        "raw_type_counts": dict(sorted(raw_type_counts.items())),
        "controller_counts": dict(sorted(controller_counts.items(), key=lambda item: int(item[0]))),
        "quality_class": quality.get("quality_class", ""),
        "keeper": quality.get("keeper", ""),
        "manifest_status": quality.get("manifest_status", ""),
        "duration_manifest_s": quality.get("duration_manifest_s", ""),
        "duration_midi_s": quality.get("duration_midi_s", ""),
        "export_note": "Sanitized MIDI-only fixture exported from local PC4 lab evidence. WAV, pcap, and manifest data lake files are intentionally excluded.",
    }
    path.write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def export_take(take_dir: pathlib.Path,
                out_dir: pathlib.Path,
                quality_rows: dict[str, dict[str, str]],
                capture_root: pathlib.Path) -> bool:
    take_id = take_dir.name
    rows = read_midi_rows(take_dir / "midi_events.tsv")
    if not rows:
        return False

    out_dir.mkdir(parents=True, exist_ok=True)
    write_raw_tsv(out_dir / f"{take_id}.raw.tsv", rows)
    write_meta(out_dir / f"{take_id}.meta.json",
               take_id,
               rows,
               quality_rows.get(take_id, {}),
               capture_root)
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--derived-root", required=True)
    parser.add_argument("--capture-root", required=True)
    parser.add_argument("--quality-summary", required=True)
    parser.add_argument("--out-dir", required=True)
    args = parser.parse_args()

    derived_root = pathlib.Path(args.derived_root)
    capture_root = pathlib.Path(args.capture_root)
    out_dir = pathlib.Path(args.out_dir)
    quality_rows = read_quality_summary(pathlib.Path(args.quality_summary))

    exported: list[str] = []
    skipped: list[str] = []
    for path in sorted(derived_root.glob("*/midi_events.tsv")):
        take_dir = path.parent
        if export_take(take_dir, out_dir, quality_rows, capture_root):
            exported.append(take_dir.name)
        else:
            skipped.append(take_dir.name)

    require("2026-04-26-pc4-tui-002" in exported, "230s pc4-tui-002 fixture must be exported")
    print(f"exported {len(exported)} PC4 live MIDI fixtures")
    for take_id in exported:
        print(f"  {take_id}")
    if skipped:
        print(f"skipped {len(skipped)} zero-event derived takes")
        for take_id in skipped:
            print(f"  {take_id}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
